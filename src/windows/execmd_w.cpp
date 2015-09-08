
#include "execmd.h"

#include <windows.h>
#include <iostream>
#include <string>

#include "debuglog.h"

using namespace std;

static void printError(const string& text)
{
    DWORD err = GetLastError();
    LOGERR(("%s : err: %d\n", text.c_str(), err));
}

class ExecCmd::Internal {
public:
    Internal()
        : advise(0), provide(0), timeoutMs(1000),
          hOutputRead(NULL), hInputWrite(NULL) {
    }

    std::vector<std::string>   m_env;
    ExecCmdAdvise   *advise;
    ExecCmdProvide  *provide;
    bool             killRequest;
    int              timeoutMs;
    string           stderrFile;
    // Subprocess id
    HANDLE           hOutputRead;
    HANDLE           hInputWrite;
    OVERLAPPED       oOutputRead; // Do these need resource control?
    OVERLAPPED       oInputWrite;
    PROCESS_INFORMATION piProcInfo;

    // Reset internal state indicators. Any resources should have been
    // previously freed
    void reset() {
        killRequest = false;
        hOutputRead = NULL;
        hInputWrite = NULL;
        memset(&oOutputRead, 0, sizeof(oOutputRead));
        memset(&oInputWrite, 0, sizeof(oInputWrite));
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    }
    void releaseResources() {
        if (piProcInfo.hProcess)
            CloseHandle(piProcInfo.hProcess);
        if (piProcInfo.hThread)
            CloseHandle(piProcInfo.hThread);
        if (hOutputRead)
            CloseHandle(hOutputRead);
        if (hInputWrite)
            CloseHandle(hInputWrite);
        if (oOutputRead.hEvent)
            CloseHandle(oOutputRead.hEvent);
        if (oInputWrite.hEvent)
            CloseHandle(oInputWrite.hEvent);
        reset();
    }
    bool PreparePipes(bool has_input, HANDLE *hChildInput,
                      bool has_output, HANDLE *hChildOutput,
                      HANDLE *hChildError);
};

ExecCmd::ExecCmd()
{
    m = new Internal();
    if (m) {
        m->reset();
    }
}
ExecCmd::~ExecCmd()
{
    if (m)
        m->releaseResources();
}

bool ExecCmd::which(const string& cmd, string& exe, const char* path)
{
    return false;
}

void ExecCmd::setAdvise(ExecCmdAdvise *adv)
{
    m->advise = adv;
}
void ExecCmd::setProvide(ExecCmdProvide *p)
{
    m->provide = p;
}
void ExecCmd::setTimeout(int mS)
{
    if (mS > 30) {
        m->timeoutMs = mS;
    }
}
void ExecCmd::setStderr(const std::string& stderrFile)
{
    m->stderrFile = stderrFile;
}
pid_t ExecCmd::getChildPid()
{
    return m->piProcInfo.dwProcessId;
}
void ExecCmd::setKill()
{
    m->killRequest = true;
}
void ExecCmd::zapChild()
{
    setKill();
    (void)wait();
}
void ExecCmd::putenv(const string &envassign)
{
    m->m_env.push_back(envassign);
}
void ExecCmd::putenv(const string &name, const string& value)
{
    string ea = name + "=" + value;
    putenv(ea);
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
    hOutputRead = NULL;
    hInputWrite = NULL;
    memset(&oOutputRead, 0, sizeof(oOutputRead));
    memset(&oInputWrite, 0, sizeof(oInputWrite));

    // manual reset event
    oOutputRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (oOutputRead.hEvent == INVALID_HANDLE_VALUE) {
        LOGERR(("ExecCmd::PreparePipes: CreateEvent failed\n"));
        goto errout;
    }

    // manual reset event
    oInputWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (oInputWrite.hEvent == INVALID_HANDLE_VALUE) {
        LOGERR(("ExecCmd::PreparePipes: CreateEvent failed\n"));
        goto errout;
    }

    SECURITY_ATTRIBUTES sa;
    // Set up the security attributes struct.
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE; // this is the critical bit
    sa.lpSecurityDescriptor = NULL;

    if (has_output) {
        // src for this code: https://www.daniweb.com/software-development/cpp/threads/295780/using-named-pipes-with-asynchronous-io-redirection-to-winapi
        // ONLY IMPORTANT CHANGE
        // set inheritance flag to TRUE in CreateProcess
        // you need this for the client to inherit the handles
       
        // Create the child output named pipe.
        // This creates a inheritable, one-way handle for the server to read
        hOutputReadTmp = CreateNamedPipe(
            TEXT("\\\\.\\pipe\\outstreamPipe"),
            PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
            PIPE_WAIT,
            1, 4096, 4096, 0, &sa);
        if (hOutputReadTmp == INVALID_HANDLE_VALUE) {
            printError("preparePipes: CreateNamedPipe(out tmp)");
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
        if (hOutputWrite == INVALID_HANDLE_VALUE) {
            printError("preparePipes: CreateFile(outputWrite)");
            goto errout;
        }

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
                             GetCurrentProcess(), &hOutputRead,
                             0, FALSE, // Make it uninheritable.
                             DUPLICATE_SAME_ACCESS)) {
            printError("preparePipes: DuplicateHandle(readtmp->outputread)");
            goto errout;
        }
        // now we kill the original, inheritable server-side handle.  That
        // will keep the pipe open, but keep the client program from
        // getting it. Child-proofing.  Close inheritable copies of the
        // handles you do not want to be inherited.
        if (!CloseHandle(hOutputReadTmp)) {
            printError("preparePipes: CloseHandle(readtmp)");
            goto errout;
        }
        hOutputReadTmp = NULL;
    } else {
        // Not using child output. Let the child have our standard output.
        HANDLE hstd = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hstd == INVALID_HANDLE_VALUE) {
            printError("preparePipes: GetStdHandle(stdout)");
            goto errout;
        }
        if (!DuplicateHandle(GetCurrentProcess(), hstd,
                             GetCurrentProcess(), &hOutputWrite,
                             0, TRUE, 
                             DUPLICATE_SAME_ACCESS)) {
            printError("preparePipes: DuplicateHandle(stdout)");
            goto errout;
        }
    }

    // Stderr: output to file or inherit. We don't support the file thing
    // for the moment
    if (false && !stderrFile.empty()) {
        // Open the file set up the child handle: TBD
    } else {
        // Let the child inherit our standard input
        HANDLE hstd = GetStdHandle(STD_ERROR_HANDLE);
        if (hstd == INVALID_HANDLE_VALUE) {
            printError("preparePipes: GetStdHandle(stderr)");
            goto errout;
        }
        if (!DuplicateHandle(GetCurrentProcess(), hstd,
                             GetCurrentProcess(), &hErrorWrite,
                             0, TRUE,
                             DUPLICATE_SAME_ACCESS)) {
            printError("preparePipes: DuplicateHandle(stderr)");
            goto errout;
        }
    }

    if (has_input) {
        // now same procedure for input pipe

        HANDLE hInputWriteTmp = CreateNamedPipe(
            TEXT("\\\\.\\pipe\\instreamPipe"),
            PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, 4096, 4096, 0, &sa);
        if (hInputWriteTmp == INVALID_HANDLE_VALUE) {
            printError("preparePipes: CreateNamedPipe(inputWTmp)");
            goto errout;
        }

        hInputRead = CreateFile(
            TEXT("\\\\.\\pipe\\instreamPipe"),
            FILE_READ_DATA | SYNCHRONIZE,
            0, &sa, OPEN_EXISTING, // very important flag!
            FILE_ATTRIBUTE_NORMAL, 0 // no template file for OPEN_EXISTING
            );
        if (hInputRead == INVALID_HANDLE_VALUE) {
            printError("preparePipes: CreateFile(inputRead)");
            goto errout;
        }
        
        if (!DuplicateHandle(GetCurrentProcess(), hInputWriteTmp,
                             GetCurrentProcess(), &hInputWrite,
                             0, FALSE, // Make it uninheritable.
                             DUPLICATE_SAME_ACCESS)) {
            printError("preparePipes: DuplicateHandle(inputWTmp->inputW)");
            goto errout;
        }
        if (!CloseHandle(hInputWriteTmp)) {
            printError("preparePipes: CloseHandle(inputWTmp)");
            goto errout;
        }
        hInputWriteTmp = NULL;
    } else {
        // Let the child inherit our standard input
        HANDLE hstd = GetStdHandle(STD_INPUT_HANDLE);
        if (hstd == INVALID_HANDLE_VALUE) {
            printError("preparePipes: GetStdHandle(stdin)");
            goto errout;
        }
        if (!DuplicateHandle(GetCurrentProcess(), hstd,
                             GetCurrentProcess(), &hInputRead,
                             0, TRUE,
                             DUPLICATE_SAME_ACCESS)) {
            printError("preparePipes: DuplicateHandle(stdin)");
            goto errout;
        }
    }

    *hChildInput = hInputRead;
    *hChildOutput = hOutputWrite;
    *hChildError = hErrorWrite;
    return true;

errout:
    releaseResources();
    if (hOutputWrite)
        CloseHandle(hOutputWrite);
    if (hInputRead)
        CloseHandle(hInputRead);
    if (hErrorWrite)
        CloseHandle(hErrorWrite);
    if (hInputWriteTmp)
        CloseHandle(hInputWriteTmp);
    if (hOutputReadTmp)
        CloseHandle(hOutputReadTmp);
    return false;
}

/**
   Append the given argument to a command line such that
   CommandLineToArgvW will return the argument string unchanged.
   Arguments in a command line should be separated by spaces; this
   function does not add these spaces. The caller must append spaces 
   between calls.
    
   @param arg Supplies the argument to encode.
   @param cmdLine Supplies the command line to which we append
   the encoded argument string.
   @param force Supplies an indication of whether we should quote
   the argument even if it does not contain any characters that 
   would ordinarily require quoting.
*/
static void argQuote(const string& arg, string& cmdLine, bool force = false)
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
        argQuote(*it, cmdline);
    }
    return cmdline;
}


// Create a child process 
int ExecCmd::startExec(const string &cmd, const vector<string>& args,
                       bool has_input, bool has_output)
{
    bool ret = false;
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

    HANDLE hInputRead;
    HANDLE hOutputWrite;
    HANDLE hErrorWrite;
    if (!m->PreparePipes(has_input, &hInputRead, has_output, 
                         &hOutputWrite, &hErrorWrite)) {
        LOGERR(("ExecCmd::startExec: preparePipes failed\n"));
        m->releaseResources();
        return false;
    }

    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE;

    // Set up members of the PROCESS_INFORMATION structure. 
    ZeroMemory(&m->piProcInfo, sizeof(PROCESS_INFORMATION));

    // Set up members of the STARTUPINFO structure. 
    // This structure specifies the STDIN and STDOUT handles for redirection.
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
    siStartInfo.hStdOutput = hOutputWrite;
    siStartInfo.hStdInput = hInputRead;
    siStartInfo.hStdError = hErrorWrite;

    // Create the child process. 
    // Need a writable buffer for the command line, for some reason.
    LPSTR buf = (LPSTR)malloc(cmdline.size() + 1);
    memcpy(buf, cmdline.c_str(), cmdline.size());
    buf[cmdline.size()] = 0;
    bSuccess = CreateProcess(NULL,
                             buf, // command line 
                             NULL,          // process security attributes 
                             NULL,          // primary thread security attrs 
                             TRUE,         // handles are inherited 
                             0,             // creation flags 
                             NULL,          // use parent's environment 
                             NULL,          // use parent's current directory 
                             &siStartInfo,  // STARTUPINFO pointer 
                             &m->piProcInfo);  // receives PROCESS_INFORMATION 
    if (!bSuccess) {
        printError("ExecCmd::doexec: CreateProcess");
    } else {
        ret = true;
    }
    free(buf);
    // Close child-side handles else we'll never see eofs
    if (!CloseHandle(hOutputWrite))
        printError("CloseHandle");
    if (!CloseHandle(hInputRead))
        printError("CloseHandle");
    if (!CloseHandle(hErrorWrite)) 
        printError("CloseHandle");

    if (!ret)
        m->releaseResources();

    return ret;
}

enum WaitResult {
    Ok, Quit, Timeout
};

static WaitResult Wait(HANDLE hdl, int timeout) 
{
    //HANDLE hdls[2] = { hdl, eQuit };
    HANDLE hdls[1] = { hdl};
    LOGDEB0(("ExecCmd::Wait()\n"));
    DWORD res = WaitForMultipleObjects(1, hdls, FALSE, timeout);
    if (res == WAIT_OBJECT_0) {
        LOGDEB0(("ExecCmd::Wait: returning Ok\n"));
        return Ok;
    } else if (res == (WAIT_OBJECT_0 + 1)) {
        LOGDEB0(("ExecCmd::Wait: returning Quit\n"));
        return Quit;
    } else if (res == WAIT_TIMEOUT) {
        LOGDEB0(("ExecCmd::Wait: returning Timeout\n"));
        return Timeout;
    }
    printError("Wait: WaitForMultipleObjects: unknown, returning Timout\n");
    return Timeout;
}


// Send data to the child.
int ExecCmd::send(const string& data)
{
    DWORD dwWritten;
    BOOL bSuccess = false;

    bSuccess = WriteFile(m->hInputWrite, data.c_str(), (DWORD)data.size(), 
                         NULL, &m->oInputWrite);
    DWORD err = GetLastError();

    // TODO: some more decision, either the operation completes immediately
    // and we get success, or it is started (which is indicated by no success)
    // and ERROR_IO_PENDING
    // in the first case bytes read/written parameter can be used directly
    if (!bSuccess && err != ERROR_IO_PENDING) {
        return -1;
    }

    WaitResult waitRes = Wait(m->oInputWrite.hEvent, 5000);
    if (waitRes == Ok) {
        if (!GetOverlappedResult(m->hInputWrite, 
                                 &m->oInputWrite, &dwWritten, TRUE)) {
            printError("GetOverlappedResult");
            return -1;
        }
    } else if (waitRes == Quit) {
        if (!CancelIo(m->hInputWrite)) {
            printError("CancelIo");
        }
        return -1;
    } else if (waitRes == Timeout) {
        if (!CancelIo(m->hInputWrite)) {
            printError("CancelIo");
        }
        return -1;
    }
    return dwWritten;
}

#ifndef MIN
#define MIN(A,B) ((A)<(B)?(A):(B))
#endif

// Read output from the child process's pipe for STDOUT
// and write to cout in this programme
// Stop when there is no more data. 
// @arg cnt count to read, -1 means read to end of data.
int ExecCmd::receive(string& data, int cnt)
{
    int totread = 0;
    LOGDEB(("ExecCmd::receive: cnt %d\n", cnt));
    while (true) {
        const int BUFSIZE = 8192;
        CHAR chBuf[BUFSIZE];
        int toread = cnt > 0 ? MIN(cnt - totread, BUFSIZE) : BUFSIZE;
        BOOL bSuccess = ReadFile(m->hOutputRead, chBuf, toread,
				            NULL, &m->oOutputRead);
        DWORD err = GetLastError();
        LOGDEB1(("receive: ReadFile: success %d err %d\n",
                 int(bSuccess), int(err)));
        if (!bSuccess && err != ERROR_IO_PENDING) {
            LOGERR(("ExecCmd::receive: ReadFile error: %d\n", int(err)));
            break;
        }

        WaitResult waitRes = Wait(m->oOutputRead.hEvent, 1000);
        if (waitRes == Ok) {
            DWORD dwRead;
            if (!GetOverlappedResult(m->hOutputRead, &m->oOutputRead,
                                     &dwRead, TRUE)) {
                printError("GetOverlappedResult");
                return -1;
            }
            totread += dwRead;
            data.append(chBuf, dwRead);
            LOGDEB1(("ExecCmd::receive: got %d bytes\n", int(dwRead)));
        } else if (waitRes == Quit) {
            if (!CancelIo(m->hOutputRead)) {
                printError("CancelIo");
            }
            break;
        } else if (waitRes == Timeout) {
            // We only want to cancel if m_advise says so here. Is the io still
            // valid at this point ?
            if (!CancelIo(m->hOutputRead)) {
                printError("CancelIo");
            }
            break;
        }
    }
    return totread;
}

int ExecCmd::getline(std::string& data)
{
    LOGERR(("ExecCmd::getline not implemented\n"));
    return -1;
}

int ExecCmd::wait() 
{
    // Wait until child process exits.
    WaitForSingleObject(m->piProcInfo.hProcess, INFINITE);

    // Get exit code
    DWORD exit_code = 0;
    GetExitCodeProcess(m->piProcInfo.hProcess, &exit_code);

    // Release all resources
    m->releaseResources();
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
        send(*input);
    } else if (output) {
        receive(*output);
    }
    return wait();
}
