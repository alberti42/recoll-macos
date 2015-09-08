
#include "execmd.h"

#include <windows.h>
#include <iostream>
#include <string>

using namespace std;

class ExecCmd::Internal {
public:
    Internal()
        : m_advise(0), m_provide(0), m_timeoutMs(1000),
          m_pid(-1), m_hOutputRead(NULL), m_hInputWrite(NULL) {
    }

    std::vector<std::string>   m_env;
    ExecCmdAdvise   *m_advise;
    ExecCmdProvide  *m_provide;
    bool             m_killRequest;
    int              m_timeoutMs;
    string           m_stderrFile;
    // Subprocess id
    pid_t            m_pid;
    HANDLE           m_hOutputRead;
    HANDLE           m_hInputWrite;
    OVERLAPPED       m_oOutputRead; // Do these need resource control?
    OVERLAPPED       m_oInputWrite;
    PROCESS_INFORMATION m_piProcInfo;

    // Reset internal state indicators. Any resources should have been
    // previously freed
    void reset() {
        m_killRequest = false;
        m_hOutputRead = NULL;
        m_hInputWrite = NULL;
        memset(&m_oOutputRead, 0, sizeof(m_oOutputRead));
        memset(&m_oInputWrite, 0, sizeof(m_oInputWrite));
        m_pid = -1;
    }
};

ExecCmd::ExecCmd()
{
    m = new Internal();
    if (m) {
        m->reset();
    }
}

void ExecCmd::setAdvise(ExecCmdAdvise *adv)
{
    m->m_advise = adv;
}
void ExecCmd::setProvide(ExecCmdProvide *p)
{
    m->m_provide = p;
}
void ExecCmd::setTimeout(int mS)
{
    if (mS > 30) {
        m->m_timeoutMs = mS;
    }
}
void ExecCmd::setStderr(const std::string& stderrFile)
{
    m->m_stderrFile = stderrFile;
}
pid_t ExecCmd::getChildPid()
{
    return m->m_pid;
}
void ExecCmd::setKill()
{
    m->m_killRequest = true;
}
void ExecCmd::zapChild()
{
    setKill();
    (void)wait();
}


bool ExecCmd::Internal::PreparePipes(bool has_input,HANDLE *hChildInput, 
                                     bool has_output, HANDLE *hChildOutput, 
                                     HANDLE *hChildError)
{
    HANDLE hInputWriteTmp = NULL;
    HANDLE hOutputReadTmp = NULL;
    HANDLE hOutputWrite = NULL;
    HANDLE hErrorWrite = NULL;
    HANDLE hInputRead = NULL;
    m_hOutputRead = NULL;
    m_hInputWrite = NULL;
    memset(&m_oOutputRead, 0, sizeof(m_oOutputRead));
    memset(&m_oInputWrite, 0, sizeof(m_oInputWrite));

    // manual reset event
    m_oOutputRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_oOutputRead.hEvent == INVALID_HANDLE_VALUE) {
        goto errout;
    }

    // manual reset event
    m_oInputWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_oInputWrite.hEvent == INVALID_HANDLE_VALUE) {
        goto errout;
    }

    if (has_output) {
        // src for this code: https://www.daniweb.com/software-development/cpp/threads/295780/using-named-pipes-with-asynchronous-io-redirection-to-winapi
        // ONLY IMPORTANT CHANGE
        // set inheritance flag to TRUE in CreateProcess
        // you need this for the client to inherit the handles

        SECURITY_ATTRIBUTES sa;
        // Set up the security attributes struct.
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE; // this is the critical bit
        sa.lpSecurityDescriptor = NULL;

        // Create the child output named pipe.
        // This creates a inheritable, one-way handle for the server to read
        hOutputReadTmp = CreateNamedPipe(
            TEXT("\\\\.\\pipe\\outstreamPipe"),
            PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, 4096, 4096, 0, &sa);
        if (hOutputReadTmp == INVALID_HANDLE_VALUE) {
            goto errout;
        }


        // However, the client can not use an inbound server handle.
        // Client needs a write-only, outgoing handle.
        // So, you create another handle to the same named pipe, only
        // write-only.  Again, must be created with the inheritable
        // attribute, and the options are important.
        // use CreateFile to open a new handle to the existing pipe...
        hOutputWrite = CreateFile(
            TEXT("\\\\.\\pipe\\outstreamPipe"),
            FILE_WRITE_DATA | SYNCHRONIZE,
            0, &sa, OPEN_EXISTING, // very important flag!
            FILE_ATTRIBUTE_NORMAL, 0 // no template file for OPEN_EXISTING
            );


        // All is well?  not quite. Our main server-side handle was
        // created shareable.
        // That means the client will receive it, and we have a
        // problem because pipe termination conditions rely on knowing
        // when the last handle closes.
        // So, the only answer is to create another one, just for the server...
        // Create new output read handle and the input write
        // handles. Set the Inheritance property to FALSE. Otherwise,
        // the child inherits the properties and, as a result,
        // non-closeable handles to the pipes are created.
        if (!DuplicateHandle(GetCurrentProcess(), hOutputReadTmp,
                             GetCurrentProcess(), &m_hOutputRead,
                             0, FALSE, // Make it uninheritable.
                             DUPLICATE_SAME_ACCESS)) {
            goto errout;
        }
        // now we kill the original, inheritable server-side handle.  That
        // will keep the pipe open, but keep the client program from
        // getting it. Child-proofing.  Close inheritable copies of the
        // handles you do not want to be inherited.
        if (!CloseHandle(hOutputReadTmp)) {
            goto errout;
        }
        hOutputReadTmp = NULL;
    }


#if 0 // Nope, can't do this, we certainly don't want to mix stdout and stderr
    // Have to take the give stderr output file instead
    if (!m->m_stderrFile.empty()) {
        // Open the file and make the handle inheritable
    } else {
        // Duplicate our own stderr. Does passing NULL work for this ?
    }

    // we're going to give the same pipe for stderr, so duplicate for that:
    // Create a duplicate of the output write handle for the std error
    // write handle. This is necessary in case the child application
    // closes one of its std output handles.
    if (!DuplicateHandle(GetCurrentProcess(), hOutputWrite,
                         GetCurrentProcess(), &hErrorWrite,
                         0, TRUE, DUPLICATE_SAME_ACCESS)) {
        goto errout;
    }
#endif

    if (has_input) {
        // now same procedure for input pipe

        HANDLE hInputWriteTmp = CreateNamedPipe(
            TEXT("\\\\.\\pipe\\instreamPipe"),
            PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, 4096, 4096, 0, &sa);
        if (hInputWriteTmp == INVALID_HANDLE_VALUE) {
            goto errout;
        }

        hInputRead = CreateFile(
            TEXT("\\\\.\\pipe\\instreamPipe"),
            FILE_READ_DATA | SYNCHRONIZE,
            0, &sa, OPEN_EXISTING, // very important flag!
            FILE_ATTRIBUTE_NORMAL, 0 // no template file for OPEN_EXISTING
            );

        if (!DuplicateHandle(GetCurrentProcess(), hInputWriteTmp,
                             GetCurrentProcess(), &m_hInputWrite,
                             0, FALSE, // Make it uninheritable.
                             DUPLICATE_SAME_ACCESS)) {
            goto errout;
        }
        if (!CloseHandle(hInputWriteTmp)) {
            goto errout;
        }
        hInputWriteTmp = NULL;
    }

    *hChildInput = hInputRead;
    *hChildOutput = hOutputWrite;
    *hChildError = hErrorWrite;
    return true;

errout:
    if (hInputWriteTmp)
        CloseHandle(hInputWriteTmp);
    if (hOutputReadTmp)
        CloseHandle(hOutputReadTmp);
    if (m_hOutputRead)
        CloseHandle(m_hOutputRead);
    if (m_hInputWrite)
        CloseHandle(m_hInputWrite);
    if (m_oOutputRead.hEvent)
        CloseHandle(m_oOutputRead.hEvent);
    if (m_oInputWrite.hEvent)
        CloseHandle(m_oInputWrite.hEvent);
    if (hOutputWrite)
        CloseHandle(hOutputWrite);
    if (hInputRead)
        CloseHandle(hInputRead);
    if (hErrorWrite)
        CloseHandle(hErrorWrite);
}

/**
    This routine appends the given argument to a command line such
    that CommandLineToArgvW will return the argument string unchanged.
    Arguments in a command line should be separated by spaces; this
    function does not add these spaces. The caller must append spaces 
    between calls.
    
    @param arg Supplies the argument to encode.

    @param cmdLine Supplies the command line to which we append
      the encoded argument string.

    @force Supplies an indication of whether we should quote
           the argument even if it does not contain any characters that 
           would ordinarily require quoting.
*/
static void argQuote(const string& arg,
                     string& cmdLine,
                     bool force = false)
{
    // Don't quote unless we actually need to do so
    if (!force && !arg.empty() && 
        arg.find_first_of(" \t\n\v\"") == arg.npos) {
        cmdLine.append(arg);
    } else {
        cmdLine.push_back ('"');
        
        for (auto It = arg.begin () ; ; ++It) {
            unsigned NumberBackslashes = 0;
        
            while (It != arg.end () && *It == '\\') {
                ++It;
                ++NumberBackslashes;
            }
        
            if (It == arg.end()) {
                // Escape all backslashes, but let the terminating
                // double quotation mark we add below be interpreted
                // as a metacharacter.
                cmdLine.append (NumberBackslashes * 2, '\\');
                break;
            } else if (*It == L'"') {
                // Escape all backslashes and the following
                // double quotation mark.
                cmdLine.append (NumberBackslashes * 2 + 1, '\\');
                cmdLine.push_back (*It);
            } else {
                // Backslashes aren't special here.
                cmdLine.append (NumberBackslashes, '\\');
                cmdLine.push_back (*It);
            }
        }
        cmdLine.push_back ('"');
    }
}

static string argvToCmdLine(const string& cmd, const vector<string>& args)
{
    string cmdline;
    argQuote(cmd, cmdline);
    for (auto it = args.begin(); it != args.end(); it++) {
        cmdline.append(" ");
        argQuote(cmd, cmdline);
    }
    return cmdline;
}


// Create a child process 
int ExecCmd::startExec(const string &cmd, const vector<string>& args,
		       bool has_input, bool has_output)
{
    { // Debug and logging
	string command = cmd + " ";
	for (vector<string>::const_iterator it = args.begin();
             it != args.end(); it++) {
	    command += "{" + *it + "} ";
	}
	LOGDEB(("ExecCmd::startExec: (%d|%d) %s\n", 
		has_input, has_output, command.c_str()));
    }

    string cmdline = argvToCmdLine(cmd, args);

    // The resource manager ensures resources are freed if we return early
    //ExecCmdRsrc e(this->m);

    HANDLE hInputRead;
    HANDLE hOutputWrite;
    HANDLE hErrorWrite;
    if (!m->PreparePipes(has_input, &hInputRead, has_output, 
                         &hOutputWrite, &hErrorWrite)) {
        return false;
    }

    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE;

    // Set up members of the PROCESS_INFORMATION structure. 

    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // Set up members of the STARTUPINFO structure. 
    // This structure specifies the STDIN and STDOUT handles for redirection.

    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
    siStartInfo.hStdOutput = hOutputWrite;
    siStartInfo.hStdInput = hInputRead;
    siStartInfo.hStdError = hErrorWrite;

    // Create the child process. 
    bSuccess = CreateProcess(NULL,
                             cmdline.c_str(), // command line 
                             NULL,          // process security attributes 
                             NULL,          // primary thread security attrs 
                             TRUE,         // handles are inherited 
                             0,             // creation flags 
                             NULL,          // use parent's environment 
                             NULL,          // use parent's current directory 
                             &siStartInfo,  // STARTUPINFO pointer 
                             &piProcInfo);  // receives PROCESS_INFORMATION 
    if (!bSuccess) {
        return false;
    }
    m->pid = piProcInfo.dwProcessId;
    return true;
}



// Read from a file and write its contents to the pipe for the child's
// STDIN.  Stop when there is no more data.
int ExecCmd::send(const string& data)
{
    DWORD dwRead, dwWritten;
    BOOL bSuccess = false;

    bSuccess = WriteFile(m->m_hInputWrite, data.c_str(), data.size(), 
                         NULL, &m->m_oInputWrite);
    DWORD err = GetLastError();

    // TODO: some more decision, either the operation completes immediately
    // and we get success, or it is started (which is indicated by no success)
    // and ERROR_IO_PENDING
    // in the first case bytes read/written parameter can be used directly
    if (!bSuccess && err != ERROR_IO_PENDING) {
        return;
    }

    WaitResult waitRes = Wait(oInputWrite.hEvent, 5000);
    if (waitRes == Ok) {
        if (!GetOverlappedResult(m->m_hInputWrite, 
                                 &m->m_oInputWrite, &dwWritten, TRUE)) {
            ErrorExit("GetOverlappedResult");
        }
    } else if (waitRes == Quit) {
        if (!CancelIo(hInputWrite)) 
            ErrorExit("CancelIo");
        return;
    } else if (waitRes == Timeout) {
        if (!CancelIo(hInputWrite)) 
            ErrorExit("CancelIo");
        return;
    }
}

// Read output from the child process's pipe for STDOUT
// and write to cout in this programme
// Stop when there is no more data. 
// @arg cnt count to read, -1 means read to end of data.
int ExecCmd::receive(const string& data, int cnt)
{
    DWORD dwRead;
    const int BUFSIZE = 8192;
    CHAR chBuf[BUFSIZE];
    BOOL bSuccess = FALSE;
    int totread = 0;

    while (true) {
        int toread = cnt > 0 ? MIN(cnt - totread, BUFSIZE) : BUFSIZE;
        bSuccess = ReadFile(m->m_hOutputRead, chBuf, toread, 
                            NULL, &m->m_oOutputRead);
        DWORD err = GetLastError();
        if (!bSuccess && err != ERROR_IO_PENDING) 
            break;

        WaitResult waitRes = Wait(oOutputRead.hEvent, 5000);
        if (waitRes == Ok) {
            if (!GetOverlappedResult(hOutputRead, &oOutputRead, &dwRead, TRUE)) {
                ErrorExit("GetOverlappedResult");
            }
            totread += dwRead;
            data.append(buf, dwRead);
        } else if (waitRes == Quit) {
            if (!CancelIo(hOutputRead)) 
                ErrorExit("CancelIo");
            break;
        } else if (waitRes == Timeout) {
            if (!CancelIo(hOutputRead)) 
                ErrorExit("CancelIo");
            break;
        }
    }
    return totread;
}

int ExecCmd::wait() 
{
    // Wait until child process exits.
    WaitForSingleObject(m->m_piProcInfo.hProcess, INFINITE);

    // Get exit code
    DWORD exit_code = 0;
    GetExitCodeProcess(m->m_piProcInfo.hProcess, &exit_code);

    // Close handles to the child process and its primary thread.
    CloseHandle(m->m_piProcInfo.hProcess);
    CloseHandle(m->m_piProcInfo.hThread);
    return (int)exit_code;
}


int ExecCmd::doexec(const string &cmd, const vector<string>& args,
		    const string *input, string *output)
{
    if (input && output) {
        LOGERR(("ExecCmd::doexec: can't do both input and output\n"));
        return -1;
    }
    if (startExec(cmd, args, input != 0, output != 0) < 0) {
	return -1;
    }
    if (input) {
        receive(*input);
    } else if (output) {
        send(*output);
    }
    return wait();
}
