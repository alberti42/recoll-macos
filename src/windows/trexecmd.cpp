#include "autoconfig.h"

#include "execmd.h"

#include <stdio.h>
#include <stdlib.h>
#include "safeunistd.h"
#include <string.h>

#include <string>
#include <iostream>
#include <sstream>
#include <vector>

#include "debuglog.h"
#include "cancelcheck.h"
#include "execmd.h"
#include "smallut.h"

using namespace std;

// Testing the rclexecm protocol outside of recoll. Here we use the
// rcldoc.py filter, you can try with rclaudio too, adjust the file
// arg accordingly. This simplified driver only really works with
// single-doc files (else it extracts only the first doc, usually the
// empty self-doc).
bool exercise_mhexecm(const string& cmdstr, const string& mimetype, 
                      vector<string>& files)
{
	if (files.empty())
		return false;

    ExecCmd cmd;
	vector<string> myparams; 
	// Hack for windows: the command is always "Python somescript"
	myparams.push_back(files[0]);
	files.erase(files.begin());

    if (cmd.startExec(cmdstr, myparams, 1, 1) < 0) {
        cerr << "startExec " << cmdstr << " failed. Missing command?\n";
        return false;
    }

    for (vector<string>::const_iterator it = files.begin();
         it != files.end(); it++) {
        // Build request message
        ostringstream obuf;
        obuf << "Filename: " << (*it).length() << "\n" << (*it);
        obuf << "Mimetype: " << mimetype.length() << "\n" << mimetype;
        // Bogus parameter should be skipped by filter
        obuf << "BogusParam: " << string("bogus").length() << "\n" << "bogus";
        obuf << "\n";
        cerr << "SENDING: [" << obuf.str() << "]\n";
        // Send it 
        if (cmd.send(obuf.str()) < 0) {
            // The real code calls zapchild here, but we don't need it as
            // this will be handled by ~ExecCmd
            //cmd.zapChild();
            cerr << "send error\n";
            return false;
        }

        // Read answer
        for (int loop=0;;loop++) {
            string name, data;

            // Code from mh_execm.cpp: readDataElement
            string ibuf;
            // Read name and length
            if (cmd.getline(ibuf) <= 0) {
                cerr << "getline error\n";
                return false;
            }
            // Empty line (end of message)
            if (!ibuf.compare("\n")) {
                cerr << "Got empty line\n";
                name.clear();
                break;
            }

            // Filters will sometimes abort before entering the real
            // protocol, ie if a module can't be loaded. Check the
            // special filter error first word:
            if (ibuf.find("RECFILTERROR ") == 0) {
                cerr << "Got RECFILTERROR\n";
                return false;
            }

            // We're expecting something like Name: len\n
            vector<string> tokens;
            stringToTokens(ibuf, tokens);
            if (tokens.size() != 2) {
                cerr << "bad line in filter output: [" << ibuf << "]\n";
                return false;
            }
            vector<string>::iterator it = tokens.begin();
            name = *it++;
            string& slen = *it;
            int len;
            if (sscanf(slen.c_str(), "%d", &len) != 1) {
                cerr << "bad line in filter output (no len): [" << 
                    ibuf << "]\n";
                return false;
            }
            // Read element data
            data.erase();
            if (len > 0 && cmd.receive(data, len) != len) {
                cerr << "MHExecMultiple: expected " << len << 
                    " bytes of data, got " << data.length() << endl;
                return false;
            }

            // Empty element: end of message
            if (name.empty())
                break;
            cerr << "Got name: [" << name << "] data [" << data << "]\n";
        }
    }
    return true;
}

static char *thisprog;
static char usage [] =
                                                           "trexecmd [-c -r -i -o] cmd [arg1 arg2 ...]\n" 
                                                           "   -c : test cancellation (ie: trexecmd -c sleep 1000)\n"
                                                           "   -r : run reexec. Must be separate option.\n"
                                                           "   -i : command takes input\n"
                                                           "   -o : command produces output\n"
                                                           "    If -i is set, we send /etc/group contents to whatever command is run\n"
                                                           "    If -o is set, we print whatever comes out\n"
                                                           "trexecmd -m <filter> <mimetype> <file> [file ...]: test execm:\n"
                                                           "     <filter> should be the path to an execm filter\n"
                                                           "     <mimetype> the type of the file parameters\n"
                                                           "trexecmd -w cmd : do the 'which' thing\n"
                                                           ;

static void Usage(FILE *fp = stderr)
{
    fprintf(fp, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_i     0x4
#define OPT_w     0x8
#define OPT_c     0x10
#define OPT_r     0x20
#define OPT_m     0x40
#define OPT_o     0x80

// Data sink for data coming out of the command. We also use it to set
// a cancellation after a moment.
class MEAdv : public ExecCmdAdvise {
public:
    void newData(int cnt) {
        if (op_flags & OPT_c) {
            static int  callcnt;
            if (callcnt++ == 5) {
                // Just sets the cancellation flag
                CancelCheck::instance().setCancel();
                // Would be called from somewhere else and throws an
                // exception. We call it here for simplicity
				cerr << "newData: should throw !\n";
                CancelCheck::instance().checkCancel();
            }
        }
        //cerr << "newData(" << cnt << ")" << endl;
    }
};

// Data provider, used if the -i flag is set
class MEPv : public ExecCmdProvide {
public:
    string *m_input;
    int  m_cnt;
    MEPv(string *i) 
        : m_input(i), m_cnt(0) {
    }
    ~MEPv() {
    }
    void newData() {
        if (m_cnt++ < 10) {
            char num[30];
            sprintf(num, "%d", m_cnt);
            *m_input = string("This is an input chunk ") + string(num) +
                string("\n");
        } else {
            m_input->erase();
        }
    }
    void reset() {
        m_cnt = 0;
    }
};



ReExec reexec;
int main(int argc, char *argv[])
{
#if 0
    reexec.init(argc, argv);

    if (0) {
        // Disabled: For testing reexec arg handling
        vector<string> newargs;
        newargs.push_back("newarg");
        newargs.push_back("newarg1");
        newargs.push_back("newarg2");
        newargs.push_back("newarg3");
        newargs.push_back("newarg4");
        reexec.insertArgs(newargs, 2);
    }
#endif

    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
        (*argv)++;
        if (!(**argv))
            /* Cas du "adb - core" */
            Usage();
        while (**argv)
            switch (*(*argv)++) {
            case 'c':   op_flags |= OPT_c; break;
            case 'r':   op_flags |= OPT_r; break;
            case 'w':   op_flags |= OPT_w; break;
            case 'm':   op_flags |= OPT_m; break;
            case 'i':   op_flags |= OPT_i; break;
            case 'o':   op_flags |= OPT_o; break;
            case'h': 
                for (int i = 0; i < 10; i++) {
                    cout << "MESSAGE " << i << " FROM TREXECMD\n";
                    cout.flush();
                    //sleep(1);
                }
                return 0;
            default: Usage();   break;
            }
    b1: argc--; argv++;
    }

    if (argc < 1)
        Usage();

    string arg1 = *argv++; argc--;
    vector<string> l;
    while (argc > 0) {
        l.push_back(*argv++); argc--;
    }

    DebugLog::getdbl()->setloglevel(DEBDEB1);
    DebugLog::setfilename("stderr");
#if 0
    signal(SIGPIPE, SIG_IGN);
#endif

    if (op_flags & OPT_w) {
        // Test "which" method
        string path;
        if (ExecCmd::which(arg1, path)) {
            cout << path << endl;
            return 0;
        } 
        return 1;
    } else if (op_flags & OPT_m) {
        if (l.size() < 2)
            Usage();
        string mimetype = l[0];
        l.erase(l.begin());
        return exercise_mhexecm(arg1, mimetype, l) ? 0 : 1;
    } else {
        // Default: execute command line arguments
        ExecCmd mexec;

        // Set callback to be called whenever there is new data
        // available and at a periodic interval, to check for
        // cancellation
        MEAdv adv;
        mexec.setAdvise(&adv);
        //mexec.setTimeout(5);
#ifdef LATER
        // Stderr output goes there
        mexec.setStderr("/tmp/trexecStderr");
#endif
        // A few environment variables. Check with trexecmd env
        mexec.putenv("TESTVARIABLE1=TESTVALUE1");
        mexec.putenv("TESTVARIABLE2=TESTVALUE2");
        mexec.putenv("TESTVARIABLE3=TESTVALUE3");

        string input, output;
        MEPv  pv(&input);
        
        string *ip = 0;
        if (op_flags  & OPT_i) {
            ip = &input;
            mexec.setProvide(&pv);
        }
        string *op = 0;
        if (op_flags & OPT_o) {
            op = &output;
        }

        int status = -1;
        for (int i = 0; i < 10; i++) {
            output.clear();
            pv.reset();
            try {
                status = mexec.doexec(arg1, l, ip, op);
            } catch (CancelExcept) {
                cerr << "CANCELLED" << endl;
            }
            //fprintf(stderr, "Status: 0x%x\n", status);
            if (op_flags & OPT_o) {
                //cout << "data received: [" << output << "]\n";
                cerr << "iter " << i << " status " <<
                    status << " bytes received " << output.size() << endl;
            }
            if (status)
                break;
        }
        return status >> 8;
    }
}
