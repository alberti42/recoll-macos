#ifndef lint
static char rcsid[] = "@(#$Id: execmd.cpp,v 1.9 2005-11-23 10:17:35 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
#ifndef TEST_EXECMD
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#ifdef PUTENV_ARG_NOT_CONST
#include <string.h>
#endif
#include <list>
#include <string>
#include <sstream>
#include <iostream>

#include "execmd.h"
#include "pathut.h"
#include "debuglog.h"

using namespace std;
#define MAX(A,B) (A>B?A:B)

void  ExecCmd::putenv(const string &ea)
{
    env.push_back(ea);
}

int ExecCmd::doexec(const string &cmd, const list<string>& args,
		const string *input, string *output)
{
    { // Debug and logging
	string command = cmd + " ";
	for (list<string>::const_iterator it = args.begin();it != args.end();
	     it++) {
	    command += "{" + *it + "} ";
	}
	LOGDEB(("ExecCmd::doexec: %s\n", command.c_str()));
    }

    int pipein[2]; // subproc input
    int pipeout[2]; // subproc output
    pipein[0] = pipein[1] = pipeout[0] = pipeout[1] = -1;

    if (input && pipe(pipein) < 0) {
	LOGERR(("ExecCmd::doexec: pipe(2) failed. errno %d\n", errno));
	return -1;
    }
    if (output && pipe(pipeout) < 0) {
	LOGERR(("ExecCmd::doexec: pipe(2) failed. errno %d\n", errno));
	close(pipein[0]);
	close(pipein[1]);
	return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
	LOGERR(("ExecCmd::doexec: fork(2) failed. errno %d\n", errno));
	return -1;
    }

    if (pid) {
	if (input) {
	    close(pipein[0]);
	    pipein[0] = -1;
	}
	if (output) {
	    close(pipeout[1]);
	    pipeout[1] = -1;
	}
	fd_set readfds, writefds;
	if (input || output) {
	    if (input)
		fcntl(pipein[1], F_SETFL, O_NONBLOCK);
	    if (output)	
		fcntl(pipeout[0], F_SETFL, O_NONBLOCK);
	    int nwritten = 0;
	    int nfds = MAX(pipein[1], pipeout[0]) + 1;
	    for(;nfds > 0;) {
		FD_ZERO(&writefds);
		FD_ZERO(&readfds);
		if (pipein[1] >= 0)
		    FD_SET(pipein[1], &writefds);
		if (pipeout[0] >= 0)
		    FD_SET(pipeout[0], &readfds);
		nfds = MAX(pipein[1], pipeout[0]) + 1;
		//struct timeval to; to.tv_sec = 1;to.tv_usec=0;
		//cerr << "pipein[1] "<< pipein[1] << " pipeout[0] " << 
		//pipeout[0] << " nfds " << nfds << endl;
		if (select(nfds, &readfds, &writefds, 0, 0) <= 0) {
		    LOGERR(("ExecCmd::doexec: select(2) failed. errno %d\n", 
			    errno));
		    break;
		}
		if (pipein[1] >= 0 && FD_ISSET(pipein[1], &writefds)) {
		    int n = write(pipein[1], input->c_str()+nwritten, 
				  input->length() - nwritten);
		    if (n < 0) {
			LOGERR(("ExecCmd::doexec: write(2) failed. errno %d\n",
				errno));
			goto out;
		    }
		    nwritten += n;
		    if (nwritten == (int)input->length()) {
			// cerr << "Closing output" << endl;
			close(pipein[1]);
			pipein[1] = -1;
		    }
		}
		if (pipeout[0] > 0 && FD_ISSET(pipeout[0], &readfds)) {
		    char buf[1024];
		    int n = read(pipeout[0], buf, 1024);
		    if (n == 0) {
			goto out;
		    } else if (n < 0) {
			LOGERR(("ExecCmd::doexec: read(2) failed. errno %d\n",
				errno));
			goto out;
		    } else if (n > 0) {
			// cerr << "READ: " << n << endl;
			output->append(buf, n);
		    }
		}
	    }

	}
    out:
	int status;
	pid = waitpid(pid, &status, 0);
	if (pipein[0] >= 0)
	    close(pipein[0]);
	if (pipein[1] >= 0)
	    close(pipein[1]);
	if (pipeout[0] >= 0)
	    close(pipeout[0]);
	if (pipeout[1] >= 0)
	    close(pipeout[1]);
	LOGDEB1(("ExecCmd::doexec: father got status 0x%x\n", status));
	return status;
    } else {
	if (input) {
	    close(pipein[1]);
	    pipein[1] = -1;
	    if (pipein[0] != 0) {
		dup2(pipein[0], 0);
		close(pipein[0]);
		pipein[0] = -1;
	    }
	}
	if (output) {
	    close(pipeout[0]);
	    pipeout[0] = -1;
	    if (pipeout[1] != 1) {
		if (dup2(pipeout[1], 1) < 0) {
		    LOGERR(("ExecCmd::doexec: dup2(2) failed. errno %d\n",
			    errno));
		}
		if (close(pipeout[1]) < 0) {
		    LOGERR(("ExecCmd::doexec: close(2) failed. errno %d\n",
			    errno));
		}
		pipeout[1] = -1;
	    }
	}

	// Count args
	list<string>::const_iterator it;
	int i = 0;
	for (it = args.begin(); it != args.end(); it++) i++;
	// Allocate arg vector (2 more for arg0 + final 0)
	typedef const char *Ccharp;
	Ccharp *argv;
	argv = (Ccharp *)malloc((i+2) * sizeof(char *));
	if (argv == 0) {
	    LOGERR(("ExecCmd::doexec: malloc() failed. errno %d\n",
		    errno));
	    exit(1);
	}
	
	// Fill up argv
	argv[0] = path_getsimple(cmd).c_str();
	i = 1;
	for (it = args.begin(); it != args.end(); it++) {
	    argv[i++] = it->c_str();
	}
	argv[i] = 0;

#if 0
	{int i = 0;cerr << "cmd: " << cmd << endl << "ARGS: " << endl; 
	    while (argv[i]) cerr << argv[i++] << endl;}
#endif

	for (it = env.begin(); it != env.end(); it++) {
#ifdef PUTENV_ARG_NOT_CONST
	    ::putenv(strdup(it->c_str()));
#else
	    ::putenv(it->c_str());
#endif
	}
	
	execvp(cmd.c_str(), (char *const*)argv);
	// Hu ho
	LOGERR(("ExecCmd::doexec: execvp(%s) failed. errno %d\n", cmd.c_str(),
		errno));
	_Exit(128);
    }
}

#else // TEST
#include <stdio.h>
#include <string>
#include <iostream>
#include <list>
#include "debuglog.h"
using namespace std;

#include "execmd.h"

const char *data = "Une ligne de donnees\n";

int main(int argc, const char **argv)
{
    DebugLog::getdbl()->setloglevel(DEBDEB1);
    DebugLog::setfilename("stderr");
    if (argc < 2) {
	cerr << "Usage: execmd cmd arg1 arg2 ..." << endl;
	exit(1);
    }
    const string cmd = argv[1];
    list<string> l;
    for (int i = 2; i < argc; i++) {
	l.push_back(argv[i]);
    }
    ExecCmd mexec;
    string input, output;
    input = data;
    string *ip = 0;
    //ip = &input;
    mexec.putenv("TESTVARIABLE1=TESTVALUE1");
    mexec.putenv("TESTVARIABLE2=TESTVALUE2");
    mexec.putenv("TESTVARIABLE3=TESTVALUE3");

    int status = mexec.doexec(cmd, l, ip, &output);

    fprintf(stderr, "Status: 0x%x\n", status);
    cout << "Output:" << output << endl;
    exit (status >> 8);
}
#endif // TEST
