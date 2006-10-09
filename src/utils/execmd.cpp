#ifndef lint
static char rcsid[] = "@(#$Id: execmd.cpp,v 1.18 2006-10-09 16:37:08 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef TEST_EXECMD
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
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

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

#define MAX(A,B) ((A) > (B) ? (A) : (B))

void  ExecCmd::putenv(const string &ea)
{
    m_env.push_back(ea);
}

/** A resource manager to ensure that execcmd cleans up if an exception is 
 * raised in the callback */
class ExecCmdRsrc {
public:
    // Pipe for data going to the command
    int pipein[2];
    // Pipe for data coming out
    int pipeout[2];
    pid_t pid;
    ExecCmdRsrc() {
	reset();
    }
    void reset() {
	pipein[0] = pipein[1] = pipeout[0] = pipeout[1] = -1;
	pid = -1;
    }
    ~ExecCmdRsrc() {
	int status;
	if (pid > 0) {
	    LOGDEB(("ExecCmd: killing cmd\n"));
	    if (kill(pid, SIGTERM) == 0) {
		for (int i = 0; i < 3; i++) {
		    (void)waitpid(pid, &status, WNOHANG);
		    if (kill(pid, 0) != 0)
			break;
		    sleep(1);
		    if (i == 2) {
			LOGDEB(("ExecCmd: killing (KILL) cmd\n"));
			kill(pid, SIGKILL);
		    }
		}
	    }
	}
	if (pipein[0] >= 0)
	    close(pipein[0]);
	if (pipein[1] >= 0)
	    close(pipein[1]);
	if (pipeout[0] >= 0)
	    close(pipeout[0]);
	if (pipeout[1] >= 0)
	    close(pipeout[1]);
    }
};

	
int ExecCmd::doexec(const string &cmd, const list<string>& args,
		const string *inputstring, string *output)
{
    { // Debug and logging
	string command = cmd + " ";
	for (list<string>::const_iterator it = args.begin();it != args.end();
	     it++) {
	    command += "{" + *it + "} ";
	}
	LOGDEB(("ExecCmd::doexec: %s\n", command.c_str()));
    }
    const char *input = inputstring ? inputstring->data() : 0;
    unsigned int inputlen = inputstring ? inputstring->length() : 0;

    ExecCmdRsrc e;

    if (input && pipe(e.pipein) < 0) {
	LOGERR(("ExecCmd::doexec: pipe(2) failed. errno %d\n", errno));
	return -1;
    }
    if (output && pipe(e.pipeout) < 0) {
	LOGERR(("ExecCmd::doexec: pipe(2) failed. errno %d\n", errno));
	return -1;
    }

    e.pid = fork();
    if (e.pid < 0) {
	LOGERR(("ExecCmd::doexec: fork(2) failed. errno %d\n", errno));
	return -1;
    }

    if (e.pid) {
	// Father process
	if (input) {
	    close(e.pipein[0]);
	    e.pipein[0] = -1;
	    fcntl(e.pipein[1], F_SETFL, O_NONBLOCK);
	}
	if (output) {
	    close(e.pipeout[1]);
	    e.pipeout[1] = -1;
	    fcntl(e.pipeout[0], F_SETFL, O_NONBLOCK);
	}

	if (input || output) {
	    unsigned int nwritten = 0;
	    int nfds = MAX(e.pipein[1], e.pipeout[0]) + 1;
	    fd_set readfds, writefds;
	    struct timeval tv;
	    tv.tv_sec = m_timeoutMs / 1000;
	    tv.tv_usec = 1000 * (m_timeoutMs % 1000);
	    for(; nfds > 0;) {
		if (m_cancelRequest)
		    break;

		FD_ZERO(&writefds);
		FD_ZERO(&readfds);
		if (e.pipein[1] >= 0)
		    FD_SET(e.pipein[1], &writefds);
		if (e.pipeout[0] >= 0)
		    FD_SET(e.pipeout[0], &readfds);
		nfds = MAX(e.pipein[1], e.pipeout[0]) + 1;
		//struct timeval to; to.tv_sec = 1;to.tv_usec=0;
		//cerr << "e.pipein[1] "<< e.pipein[1] << " e.pipeout[0] " << 
		//e.pipeout[0] << " nfds " << nfds << endl;
		int ss;
		if ((ss = select(nfds, &readfds, &writefds, 0, &tv)) <= 0) {
		    if (ss == 0) {
			// Timeout, is ok.
			if (m_advise)
			    m_advise->newData(0);
			continue;
		    }
		    LOGERR(("ExecCmd::doexec: select(2) failed. errno %d\n", 
			    errno));
		    break;
		}
		if (e.pipein[1] >= 0 && FD_ISSET(e.pipein[1], &writefds)) {
		    int n = write(e.pipein[1], input + nwritten, 
				  inputlen - nwritten);
		    if (n < 0) {
			LOGERR(("ExecCmd::doexec: write(2) failed. errno %d\n",
				errno));
			goto out;
		    }
		    nwritten += n;
		    if (nwritten == inputlen) {
			// cerr << "Closing output" << endl;
			close(e.pipein[1]);
			e.pipein[1] = -1;
		    }
		}
		if (e.pipeout[0] > 0 && FD_ISSET(e.pipeout[0], &readfds)) {
		    char buf[8192];
		    int n = read(e.pipeout[0], buf, 8192);
		    if (n == 0) {
			goto out;
		    } else if (n < 0) {
			LOGERR(("ExecCmd::doexec: read(2) failed. errno %d\n",
				errno));
			goto out;
		    } else if (n > 0) {
			// cerr << "READ: " << n << endl;
			output->append(buf, n);
			if (m_advise)
			    m_advise->newData(n);
		    }
		}
	    }
	}

    out:
	int status = -1;
	if (!m_cancelRequest) {
	    (void)waitpid(e.pid, &status, 0);
	    e.pid = -1;
	}
	LOGDEB1(("ExecCmd::doexec: father got status 0x%x\n", status));
	return status;

    } else {
	// In child process. Set up pipes, environment, and exec command

	if (input) {
	    close(e.pipein[1]);
	    e.pipein[1] = -1;
	    if (e.pipein[0] != 0) {
		dup2(e.pipein[0], 0);
		close(e.pipein[0]);
		e.pipein[0] = -1;
	    }
	}
	if (output) {
	    close(e.pipeout[0]);
	    e.pipeout[0] = -1;
	    if (e.pipeout[1] != 1) {
		if (dup2(e.pipeout[1], 1) < 0) {
		    LOGERR(("ExecCmd::doexec: dup2(2) failed. errno %d\n",
			    errno));
		}
		if (close(e.pipeout[1]) < 0) {
		    LOGERR(("ExecCmd::doexec: close(2) failed. errno %d\n",
			    errno));
		}
		e.pipeout[1] = -1;
	    }
	}
	// Do we need to redirect stderr ?
	if (!m_stderrFile.empty()) {
	    int fd = open(m_stderrFile.c_str(), O_WRONLY|O_CREAT
#ifdef O_APPEND
			  |O_APPEND
#endif
			  , 0600);
	    if (fd < 0) {
		close(2);
	    } else {
		if (fd != 2) {
		    dup2(fd, 2);
		}
		lseek(2, 0, 2);
	    }
	}
	e.reset();

	// Allocate arg vector (2 more for arg0 + final 0)
	typedef const char *Ccharp;
	Ccharp *argv;
	argv = (Ccharp *)malloc((args.size()+2) * sizeof(char *));
	if (argv == 0) {
	    LOGERR(("ExecCmd::doexec: malloc() failed. errno %d\n",
		    errno));
	    exit(1);
	}
	
	// Fill up argv
	argv[0] = path_getsimple(cmd).c_str();
	int i = 1;
	list<string>::const_iterator it;
	for (it = args.begin(); it != args.end(); it++) {
	    argv[i++] = it->c_str();
	}
	argv[i] = 0;

#if 0
	{int i = 0;cerr << "cmd: " << cmd << endl << "ARGS: " << endl; 
	    while (argv[i]) cerr << argv[i++] << endl;}
#endif

	for (it = m_env.begin(); it != m_env.end(); it++) {
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
	_exit(128);
    }
    /* This cant be reached: to make cc happy */
    return -1;
}

#else // TEST
#include <stdio.h>
#include <string>
#include <iostream>
#include <list>
#include "debuglog.h"
#include "cancelcheck.h"

using namespace std;

#include "execmd.h"

const char *data = "Une ligne de donnees\n";
class MEAdv : public ExecCmdAdvise {
public:
    ExecCmd *cmd;
    void newData(int cnt) {
	cerr << "newData(" << cnt << ")" << endl;
	//	CancelCheck::instance().setCancel();
	//	CancelCheck::instance().checkCancel();
	//	cmd->setCancel();
    }
};

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
    MEAdv adv;
    adv.cmd = &mexec;
    mexec.setAdvise(&adv);
    mexec.setTimeout(500);
    mexec.setStderr("/tmp/trexecStderr");

    string input, output;
    input = data;
    string *ip = 0;
    ip = &input;
    mexec.putenv("TESTVARIABLE1=TESTVALUE1");
    mexec.putenv("TESTVARIABLE2=TESTVALUE2");
    mexec.putenv("TESTVARIABLE3=TESTVALUE3");

    int status = -1;
    try {
	status = mexec.doexec(cmd, l, ip, &output);
    } catch (CancelExcept) {
	cerr << "CANCELED" << endl;
    }

    fprintf(stderr, "Status: 0x%x\n", status);
    cout << "Output:" << output << endl;
    exit (status >> 8);
}
#endif // TEST
