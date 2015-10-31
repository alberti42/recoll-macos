#include "autoconfig.h"

#include "execmd.h"

#include <string>
#include <iostream>
#include <sstream>
#include UNORDERED_MAP_INCLUDE

#include "debuglog.h"
#include "safesysstat.h"
#include "safeunistd.h"
#include "safewindows.h"
#include <psapi.h>
#include "smallut.h"
#include "pathut.h"

using namespace std;

//////////////////////////////////////////////
// Helper routines

static void printError(const string& text)
{
    DWORD err = GetLastError();
    LOGERR(("%s : err: %d\n", text.c_str(), err));
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

// Merge the father environment with the variable specified in m_env
static char *mergeEnvironment(const STD_UNORDERED_MAP<string, string>& addenv)
{
    // Parse existing environment.
    char *envir = GetEnvironmentStrings();
    char *cp0 = envir;
    STD_UNORDERED_MAP<string, string> envirmap;

    string name, value;
    for (char *cp1 = cp0;;cp1++) {
        if (*cp1 == '=') {
            name = string(cp0, cp1 - cp0);
            cp0 = cp1 + 1;
        } else if (*cp1 == 0) {
            value = string(cp0, cp1 - cp0);
            envirmap[name] = value;
            LOGDEB1(("mergeEnvir: [%s] = [%s]\n", name.c_str(), value.c_str()));
            cp0 = cp1 + 1;
            if (*cp0 == 0)
                break;
        }
    }

    FreeEnvironmentStrings(envir);

    // Merge our values
    for (auto it = addenv.begin(); it != addenv.end(); it++) {
        envirmap[it->first] = it->second;
    }

    // Create environment block
    size_t sz = 0;
    for (auto it = envirmap.begin(); it != envirmap.end(); it++) {
        sz += it->first.size() + it->second.size() + 2; // =, 0
    }
    sz++; // final 0
    char *nenvir = (char *)malloc(sz);
    if (nenvir == 0)
        return nenvir;
    char *cp = nenvir;
    for (auto it = envirmap.begin(); it != envirmap.end(); it++) {
        memcpy(cp, it->first.c_str(), it->first.size());
        cp += it->first.size();
        *cp++ = '=';
        memcpy(cp, it->second.c_str(), it->second.size());
        cp += it->second.size();
        *cp++ = 0;
    }
    // Final double-zero
    *cp++ = 0;
    
    return nenvir;
}

static bool is_exe(const string& path)
{
    struct stat st;
    if (access(path.c_str(), X_OK) == 0 && stat(path.c_str(), &st) == 0 &&
        S_ISREG(st.st_mode)) {
        return true;
    }
    return false;
}

// In mt programs the static vector computations below needs a call
// from main before going mt. This is done by rclinit and saves having
// to take a lock on every call

// Try appending executable suffixes to the base name and see if we
// have something
static bool is_exe_base(const string& path)
{
    static vector<string> exts;
    if (exts.empty()) {
        const char *ep = getenv("PATHEXT");
        if (!ep || !*ep) {
            ep = ".com;.exe;.bat;.cmd";
        }
        string eps(ep);
        trimstring(eps, ";");
        stringToTokens(eps, exts, ";");
    }

    if (is_exe(path))
        return true;
    for (auto it = exts.begin(); it != exts.end(); it++) {
        if (is_exe(path + *it))
            return true;
    }
    return false;
}

// Parse a PATH spec into a vector<string>
static void make_path_vec(const char *ep, vector<string>& vec)
{
    if (ep && *ep) {
        string eps(ep);
        trimstring(eps, ";");
        stringToTokens(eps, vec, ";");
    }
    vec.insert(vec.begin(), ".\\");
}

static std::string pipeUniqueName(std::string nClass, std::string prefix)
{
    std::stringstream uName;

    long currCnt;
    // PID + multi-thread-protected static counter to be unique
    {
        static long cnt = 0;
        currCnt = InterlockedIncrement(&cnt);
    }
    DWORD pid = GetCurrentProcessId();

    // naming convention
    uName << "\\\\.\\" << nClass << "\\";
    uName << "pid-" << pid << "-cnt-" << currCnt << "-";
    uName << prefix;

    return uName.str();
}

enum WaitResult {
    Ok, Quit, Timeout
};

static WaitResult Wait(HANDLE hdl, int timeout) 
{
    //HANDLE hdls[2] = { hdl, eQuit };
    HANDLE hdls[1] = { hdl};
    LOGDEB1(("Wait()\n"));
    DWORD res = WaitForMultipleObjects(1, hdls, FALSE, timeout);
    if (res == WAIT_OBJECT_0) {
        LOGDEB1(("Wait: returning Ok\n"));
        return Ok;
    } else if (res == (WAIT_OBJECT_0 + 1)) {
        LOGDEB0(("Wait: returning Quit\n"));
        return Quit;
    } else if (res == WAIT_TIMEOUT) {
        LOGDEB0(("Wait: returning Timeout\n"));
        return Timeout;
    }
    printError("Wait: WaitForMultipleObjects: unknown, returning Timout\n");
    return Timeout;
}

static int getVMMBytes(HANDLE hProcess)
{
    PROCESS_MEMORY_COUNTERS pmc;
    const int MB = 1024 * 1024;
    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        LOGDEB2(("ExecCmd: getVMMBytes paged Kbs %d non paged %d Kbs\n",
                 int(pmc.QuotaPagedPoolUsage/1024),
                 int(pmc.QuotaNonPagedPoolUsage/1024)));
        return int(pmc.QuotaPagedPoolUsage /MB +
                   pmc.QuotaNonPagedPoolUsage / MB);
    }
    return -1;
}

////////////////////////////////////////////////////////
// ExecCmd:

class ExecCmd::Internal {
public:
    Internal(int flags)
        : m_advise(0), m_provide(0), m_timeoutMs(1000), m_rlimit_as_mbytes(0),
          m_flags(flags) {
        reset();
    }

    STD_UNORDERED_MAP<string, string>   m_env;
    ExecCmdAdvise   *m_advise;
    ExecCmdProvide  *m_provide;
    int              m_timeoutMs;
    int m_rlimit_as_mbytes;
    int m_flags;
    
    // We need buffered I/O for getline. The Unix version uses netcon's
    string m_buf;       // Buffer. Only used when doing getline()s
    size_t m_bufoffs;    // Pointer to current 1st byte of useful data
    bool             m_killRequest;
    string           m_stderrFile;
    HANDLE           m_hOutputRead;
    HANDLE           m_hInputWrite;
    OVERLAPPED       m_oOutputRead;
    OVERLAPPED       m_oInputWrite;
    PROCESS_INFORMATION m_piProcInfo;

    
    // Reset internal state indicators. Any resources should have been
    // previously freed. 
    void reset() {
        m_buf.resize(0);
        m_bufoffs = 0;
        m_stderrFile.erase();
        m_killRequest = false;
        m_hOutputRead = NULL;
        m_hInputWrite = NULL;
        memset(&m_oOutputRead, 0, sizeof(m_oOutputRead));
        memset(&m_oInputWrite, 0, sizeof(m_oInputWrite));
        ZeroMemory(&m_piProcInfo, sizeof(PROCESS_INFORMATION));
    }
    bool preparePipes(bool has_input, HANDLE *hChildInput,
                      bool has_output, HANDLE *hChildOutput,
                      HANDLE *hChildError);
    bool tooBig();
};

// Sending a break to a subprocess is only possible if we share the
// same console We temporarily ignore breaks, attach to the Console,
// send the break, and restore normal break processing
static bool sendIntr(int pid)
{
    LOGDEB(("execmd_w: sendIntr -> %d\n", pid));
    bool needDetach = false;
    if (GetConsoleWindow() == NULL) {
        needDetach = true;
        LOGDEB(("execmd_w: sendIntr attaching console\n"));
        if (!AttachConsole((unsigned int) pid)) {
            int err = GetLastError();
            LOGERR(("execmd_w: sendIntr: AttachConsole failed: %d\n", err));
            return false;
        }
    }

#if 0
    // Would need to do this for sending to all processes on this console
    // Disable Ctrl-C handling for our program
    if (!SetConsoleCtrlHandler(NULL, true)) {
        int err = GetLastError();
        LOGERR(("execmd_w:sendIntr:SetCons.Ctl.Hndlr.(NULL, true) failed: %d\n",
                err));
        return false;
    }
#endif

    // Note: things don't work with CTRL_C (process not advised, sometimes
    // stays apparently blocked), no idea why
    bool ret = GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pid);
    if (!ret) {
        int err = GetLastError();
        LOGERR(("execmd_w:sendIntr:Gen.Cons.CtrlEvent failed: %d\n", err));
    }

#if 0
    // Restore Ctrl-C handling for our program
    SetConsoleCtrlHandler(NULL, false);
#endif

    if (needDetach) {
        LOGDEB(("execmd_w: sendIntr detaching console\n"));
        if (!FreeConsole()) {
            int err = GetLastError();
            LOGERR(("execmd_w: sendIntr: FreeConsole failed: %d\n", err));
        }
    }

    return ret;
}

// ExecCmd resource releaser class. Using a separate object makes it
// easier that resources are released under all circumstances,
// esp. exceptions
class ExecCmdRsrc {
public:
    ExecCmdRsrc(ExecCmd::Internal *parent) 
        : m_parent(parent), m_active(true) {
    }
    void inactivate() {
        m_active = false;
    }
    ~ExecCmdRsrc() {
        if (!m_active || !m_parent)
            return;
        LOGDEB1(("~ExecCmdRsrc: working. mypid: %d\n", (int)getpid()));
        if (m_parent->m_hOutputRead)
            CloseHandle(m_parent->m_hOutputRead);
        if (m_parent->m_hInputWrite)
            CloseHandle(m_parent->m_hInputWrite);
        if (m_parent->m_oOutputRead.hEvent)
            CloseHandle(m_parent->m_oOutputRead.hEvent);
        if (m_parent->m_oInputWrite.hEvent)
            CloseHandle(m_parent->m_oInputWrite.hEvent);

        if (m_parent->m_piProcInfo.hProcess) {
            BOOL bSuccess = sendIntr(m_parent->m_piProcInfo.dwProcessId);
            if (bSuccess) {
                // Give it a chance, then terminate
                for (int i = 0; i < 3; i++) {
                    WaitResult res = Wait(m_parent->m_piProcInfo.hProcess,
                                          i == 0 ? 5 : (i == 1 ? 100 : 2000));
                    switch (res) {
                    case Ok:
                    case Quit:
                        goto breakloop;
                    case Timeout:
                        if (i == 2) {
                            TerminateProcess(m_parent->m_piProcInfo.hProcess,
                                             0xffff);
                        }
                    }
                }
            } else {
                TerminateProcess(m_parent->m_piProcInfo.hProcess,
                                 0xffff);
            }
        breakloop:
            CloseHandle(m_parent->m_piProcInfo.hProcess);
        }
        if (m_parent->m_piProcInfo.hThread)
            CloseHandle(m_parent->m_piProcInfo.hThread);
        m_parent->reset();
    }
private:
    ExecCmd::Internal *m_parent;
    bool    m_active;
};

ExecCmd::ExecCmd(int flags)
{
    m = new Internal(flags);
    if (m) {
        m->reset();
    }
}

ExecCmd::~ExecCmd()
{
    if (m) {
        ExecCmdRsrc(this->m);
        delete m;
    }
}

// This does nothing under windows, but defining it avoids ifdefs in multiple
// places
void ExecCmd::useVfork(bool)
{
}

bool ExecCmd::which(const string& cmd, string& exe, const char* path)
{
    static vector<string> s_pathelts;
    vector<string> pathelts;
    vector<string> *pep;

    if (path) {
        make_path_vec(path, pathelts);
        pep = &pathelts;
    } else {
        if (s_pathelts.empty()) {
            const char *ep = getenv("PATH");
            make_path_vec(ep, s_pathelts);
        }
        pep = &s_pathelts;
    }

    if (path_isabsolute(cmd)) {
        if (is_exe_base(cmd)) {
            exe = cmd;
            return true;
        }
        exe.clear();
        return false;
    }

    for (auto it = pep->begin(); it != pep->end(); it++) {
        exe = path_cat(*it, cmd);
        if (is_exe_base(exe)) {
            return true;
        }
    }
    exe.clear();
    return false;
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
    return m->m_piProcInfo.dwProcessId;
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

bool ExecCmd::requestChildExit()
{
    if (m->m_piProcInfo.hProcess) {
        return sendIntr(m->m_piProcInfo.dwProcessId);
    }
    return false;
}

void ExecCmd::putenv(const string &envassign)
{
    vector<string> v;
    stringToTokens(envassign, v, "=");
    if (v.size() == 2) {
        m->m_env[v[0]] = v[1];
    }
}
void ExecCmd::putenv(const string &name, const string& value)
{
    m->m_env[name] = value;
}

void ExecCmd::setrlimit_as(int mbytes)
{
    m->m_rlimit_as_mbytes = mbytes;
}

bool ExecCmd::Internal::tooBig()
{
    if (m_rlimit_as_mbytes <= 0)
        return false;
    int mbytes = getVMMBytes(m_piProcInfo.hProcess);
    if (mbytes > m_rlimit_as_mbytes) {
        LOGINFO(("ExecCmd:: process mbytes %d > set limit %d\n",
                 mbytes, m_rlimit_as_mbytes));
        m_killRequest = true;
        return true;
    }
    return false;
}

bool ExecCmd::Internal::preparePipes(bool has_input,HANDLE *hChildInput, 
                                     bool has_output, HANDLE *hChildOutput, 
                                     HANDLE *hChildError)
{
    // Note: our caller is responsible for ensuring that we start with
    // a clean state, and for freeing class resources in case of
    // error. We just manage the local ones.
    HANDLE hOutputWrite = NULL;
    HANDLE hErrorWrite = NULL;
    HANDLE hInputRead = NULL;

    // manual reset event
    m_oOutputRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_oOutputRead.hEvent == INVALID_HANDLE_VALUE) {
        LOGERR(("ExecCmd::preparePipes: CreateEvent failed\n"));
        goto errout;
    }

    // manual reset event
    m_oInputWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_oInputWrite.hEvent == INVALID_HANDLE_VALUE) {
        LOGERR(("ExecCmd::preparePipes: CreateEvent failed\n"));
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
        // This creates a non-inheritable, one-way handle for the server to read
        string pipeName = pipeUniqueName("pipe", "output");
        sa.bInheritHandle = FALSE; 
        m_hOutputRead = CreateNamedPipeA(
            pipeName.c_str(),
            PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
            PIPE_WAIT,
            1, 4096, 4096, 0, &sa);
        if (m_hOutputRead == INVALID_HANDLE_VALUE) {
            printError("preparePipes: CreateNamedPipe(outputR)");
            goto errout;
        }

        // However, the client can not use an inbound server handle.
        // Client needs a write-only, outgoing handle.
        // So, you create another handle to the same named pipe, only
        // write-only.  Again, must be created with the inheritable
        // attribute, and the options are important.
        // use CreateFile to open a new handle to the existing pipe...
        sa.bInheritHandle = TRUE; 
        hOutputWrite = CreateFile(
            pipeName.c_str(),
            FILE_WRITE_DATA | SYNCHRONIZE,
            0, &sa, OPEN_EXISTING, // very important flag!
            FILE_ATTRIBUTE_NORMAL, 0 // no template file for OPEN_EXISTING
            );
        if (hOutputWrite == INVALID_HANDLE_VALUE) {
            printError("preparePipes: CreateFile(outputWrite)");
            goto errout;
        }
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
            // This occurs when the parent process is a GUI app. Ignoring the error works, but
            // not too sure this is the right approach. Maybe look a bit more at:
            // https://support.microsoft.com/en-us/kb/190351
            //goto errout;
        }
    }

    if (has_input) {
        // now same procedure for input pipe

        sa.bInheritHandle = FALSE; 
        string pipeName = pipeUniqueName("pipe", "input");
        m_hInputWrite = CreateNamedPipeA(
            pipeName.c_str(),
            PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
            PIPE_WAIT,
            1, 4096, 4096, 0, &sa);
        if (m_hInputWrite == INVALID_HANDLE_VALUE) {
            printError("preparePipes: CreateNamedPipe(inputW)");
            goto errout;
        }

        sa.bInheritHandle = TRUE;
        hInputRead = CreateFile(
            pipeName.c_str(),
            FILE_READ_DATA | SYNCHRONIZE,
            0, &sa, OPEN_EXISTING, // very important flag!
            FILE_ATTRIBUTE_NORMAL, 0 // no template file for OPEN_EXISTING
            );
        if (hInputRead == INVALID_HANDLE_VALUE) {
            printError("preparePipes: CreateFile(inputRead)");
            goto errout;
        }
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
            //goto errout;
        }
    }

    // Stderr: output to file or inherit. We don't support the file thing
    // for the moment
    if (false && !m_stderrFile.empty()) {
        // Open the file set up the child handle: TBD
        printError("preparePipes: m_stderrFile not empty");
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
            //goto errout;
        }
    }

    *hChildInput = hInputRead;
    *hChildOutput = hOutputWrite;
    *hChildError = hErrorWrite;
    return true;

errout:
    if (hOutputWrite)
        CloseHandle(hOutputWrite);
    if (hInputRead)
        CloseHandle(hInputRead);
    if (hErrorWrite)
        CloseHandle(hErrorWrite);
    return false;
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

    // What if we're called twice ? First make sure we're clean
    {
        ExecCmdRsrc(this->m);
    }

    // Arm clean up. This will be disabled before a success return.
    ExecCmdRsrc cleaner(this->m);
    
    string cmdline = argvToCmdLine(cmd, args);

    HANDLE hInputRead;
    HANDLE hOutputWrite;
    HANDLE hErrorWrite;
    if (!m->preparePipes(has_input, &hInputRead, has_output, 
                         &hOutputWrite, &hErrorWrite)) {
        LOGERR(("ExecCmd::startExec: preparePipes failed\n"));
        return false;
    }

    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE;

    // Set up members of the PROCESS_INFORMATION structure. 
    ZeroMemory(&m->m_piProcInfo, sizeof(PROCESS_INFORMATION));

    // Set up members of the STARTUPINFO structure. 
    // This structure specifies the STDIN and STDOUT handles for redirection.
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    if (m->m_flags & EXF_SHOWWINDOW) {
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
    } else {
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    }
    siStartInfo.hStdOutput = hOutputWrite;
    siStartInfo.hStdInput = hInputRead;
    siStartInfo.hStdError = hErrorWrite;
    // This is to hide the console when starting a cmd line command from
    // the GUI. Also note STARTF_USESHOWWINDOW above
    siStartInfo.wShowWindow = SW_HIDE;

    char *envir = mergeEnvironment(m->m_env);

    // Create the child process. 
    // Need a writable buffer for the command line, for some reason.
    LOGDEB1(("ExecCmd:startExec: cmdline [%s]\n", cmdline.c_str()));
    LPSTR buf = (LPSTR)malloc(cmdline.size() + 1);
    memcpy(buf, cmdline.c_str(), cmdline.size());
    buf[cmdline.size()] = 0;
    bSuccess = CreateProcess(NULL,
                             buf, // command line 
                             NULL,          // process security attributes 
                             NULL,          // primary thread security attrs 
                             TRUE,         // handles are inherited 
                             CREATE_NEW_PROCESS_GROUP, // creation flags 
                             envir,          // Merged environment 
                             NULL,          // use parent's current directory 
                             &siStartInfo,  // STARTUPINFO pointer 
                             &m->m_piProcInfo);  // PROCESS_INFORMATION 
    if (!bSuccess) {
        printError("ExecCmd::doexec: CreateProcess");
    } 
    free(envir);
    free(buf);
    // Close child-side handles else we'll never see eofs
    if (!CloseHandle(hOutputWrite))
        printError("CloseHandle");
    if (!CloseHandle(hInputRead))
        printError("CloseHandle");
    if (!CloseHandle(hErrorWrite)) 
        printError("CloseHandle");

    if (bSuccess) {
        cleaner.inactivate();
    }
    return bSuccess;
}

// Send data to the child.
int ExecCmd::send(const string& data)
{
    LOGDEB2(("ExecCmd::send: cnt %d\n", int(data.size())));
    BOOL bSuccess = WriteFile(m->m_hInputWrite, data.c_str(),
                              (DWORD)data.size(), NULL, &m->m_oInputWrite);
    DWORD err = GetLastError();

    // TODO: some more decision, either the operation completes immediately
    // and we get success, or it is started (which is indicated by no success)
    // and ERROR_IO_PENDING
    // in the first case bytes read/written parameter can be used directly
    if (!bSuccess && err != ERROR_IO_PENDING) {
        LOGERR(("ExecCmd::send: WriteFile: got err %d\n", err));
        return -1;
    }

    WaitResult waitRes = Wait(m->m_oInputWrite.hEvent, m->m_timeoutMs);
    DWORD dwWritten;
    if (waitRes == Ok) {
        if (!GetOverlappedResult(m->m_hInputWrite, 
                                 &m->m_oInputWrite, &dwWritten, TRUE)) {
            err = GetLastError();
            LOGERR(("ExecCmd::send: GetOverLappedResult: err %d\n", err));
            return -1;
        }
    } else if (waitRes == Quit) {
        printError("ExecCmd::send: got Quit");
        if (!CancelIo(m->m_hInputWrite)) {
            printError("CancelIo");
        }
        return -1;
    } else if (waitRes == Timeout) {
        printError("ExecCmd::send: got Timeout");
        if (!CancelIo(m->m_hInputWrite)) {
            printError("CancelIo");
        }
        return -1;
    }
    LOGDEB2(("ExecCmd::send: returning %d\n", int(dwWritten)));
    return dwWritten;
}

#ifndef MIN
#define MIN(A,B) ((A)<(B)?(A):(B))
#endif

// Read output from the child process's pipe for STDOUT
// and write to cout in this programme
// Stop when there is no more data. 
// @arg cnt count to read, -1 means read to end of data.
//      0 means read whatever comes back on the first read;
int ExecCmd::receive(string& data, int cnt)
{
    LOGDEB1(("ExecCmd::receive: cnt %d\n", cnt));

    int totread = 0;

    // If there is buffered data, use it (remains from a previous getline())
    if (m->m_bufoffs < m->m_buf.size()) {
        int bufcnt = int(m->m_buf.size() - m->m_bufoffs);
        int toread = (cnt > 0) ? MIN(cnt, bufcnt) : bufcnt;
        data.append(m->m_buf, m->m_bufoffs, toread);
        m->m_bufoffs += toread;
        totread += toread;
        if (cnt == 0 || (cnt > 0 && totread == cnt)) {
            return cnt;
        }
    }
    while (true) {
        const int BUFSIZE = 4096;
        CHAR chBuf[BUFSIZE];
        int toread = cnt > 0 ? MIN(cnt - totread, BUFSIZE) : BUFSIZE;
        BOOL bSuccess = ReadFile(m->m_hOutputRead, chBuf, toread,
                                 NULL, &m->m_oOutputRead);
        DWORD err = GetLastError();
        LOGDEB1(("receive: ReadFile: success %d err %d\n",
                 int(bSuccess), int(err)));
        if (!bSuccess && err != ERROR_IO_PENDING) {
            if (err != ERROR_BROKEN_PIPE)
                LOGERR(("ExecCmd::receive: ReadFile error: %d\n", int(err)));
            break;
        }

    waitagain:
        WaitResult waitRes = Wait(m->m_oOutputRead.hEvent, m->m_timeoutMs);
        if (waitRes == Ok) {
            DWORD dwRead;
            if (!GetOverlappedResult(m->m_hOutputRead, &m->m_oOutputRead,
                                     &dwRead, TRUE)) {
                err = GetLastError();
                if (err && err != ERROR_BROKEN_PIPE) {
                    LOGERR(("ExecCmd::recv:GetOverlappedResult: err %d\n",
                            err));
                    return -1;
                }
            }
            if (dwRead > 0) {
                totread += dwRead;
                data.append(chBuf, dwRead);
                if (m->m_advise)
                    m->m_advise->newData(dwRead);
                LOGDEB1(("ExecCmd::recv: ReadFile: %d bytes\n", int(dwRead)));
            }
        } else if (waitRes == Quit) {
            if (!CancelIo(m->m_hOutputRead)) {
                printError("CancelIo");
            }
            break;
        } else if (waitRes == Timeout) {
            LOGDEB0(("ExecCmd::receive: timeout (%d mS)\n", m->m_timeoutMs));
            if (m->tooBig()) {
                if (!CancelIo(m->m_hOutputRead)) {
                    printError("CancelIo");
                }
                return -1;
            }
            // We only want to cancel if m_advise says so here.
            if (m->m_advise) {
                try {
                    m->m_advise->newData(0);
                } catch (...) {
                    if (!CancelIo(m->m_hOutputRead)) {
                        printError("CancelIo");
                    }
                    throw;
                }
            }
            if (m->m_killRequest) {
                LOGINFO(("ExecCmd::doexec: cancel request\n"));
                if (!CancelIo(m->m_hOutputRead)) {
                    printError("CancelIo");
                }
                break;
            }
            goto waitagain;
        }
        if ((cnt == 0 && totread > 0) || (cnt > 0 && totread == cnt))
            break;
    }
    LOGDEB1(("ExecCmd::receive: returning %d bytes\n", totread));
    return totread;
}

int ExecCmd::getline(string& data)
{
    LOGDEB2(("ExecCmd::getline: cnt %d, timeo %d\n", cnt, timeo));
    data.erase();
    if (m->m_buf.empty()) {
        m->m_buf.reserve(4096);
        m->m_bufoffs = 0;
    }

    for (;;) {
        // Transfer from buffer. Have to take a lot of care to keep counts and
        // pointers consistant in all end cases
        int nn = int(m->m_buf.size() - m->m_bufoffs);
        bool foundnl = false;
        for (; nn > 0;) {
            nn--;
            char c = m->m_buf[m->m_bufoffs++];
            if (c == '\r')
                continue;
            data += c;
            if (c == '\n') {
                foundnl = true;
                break;
            }
        }

        if (foundnl) {
            LOGDEB2(("ExecCmd::getline: ret: [%s]\n", data.c_str()));
            return int(data.size());
        }

        // Read more
        m->m_buf.erase();
        if (receive(m->m_buf, 0) < 0) {
            return -1;
        }
        if (m->m_buf.empty()) {
            LOGDEB(("ExecCmd::getline: eof? ret: [%s]\n", data.c_str()));
            return int(data.size());
        }
        m->m_bufoffs = 0;
    }
    //??
    return -1;
}

int ExecCmd::wait() 
{
    // If killRequest was set, we don't perform the normal
    // wait. cleaner will kill the child.
    ExecCmdRsrc cleaner(this->m);
    
    DWORD exit_code = -1;
    if (!m->m_killRequest && m->m_piProcInfo.hProcess) {
        // Wait until child process exits.
        while (WaitForSingleObject(m->m_piProcInfo.hProcess, m->m_timeoutMs)
               == WAIT_TIMEOUT) {
            LOGDEB(("ExecCmd::wait: timeout (ok)\n"));
            if (m->m_advise) {
                m->m_advise->newData(0);
            }
            if (m->tooBig()) {
                // Let cleaner work to kill the child
                m->m_killRequest = true;
                return -1;
            }
        }

        exit_code = 0;
        GetExitCodeProcess(m->m_piProcInfo.hProcess, &exit_code);
        // Clean up, here to avoid cleaner trying to kill the now
        // inexistant process.
        CloseHandle(m->m_piProcInfo.hProcess);
        m->m_piProcInfo.hProcess = NULL;
        if (m->m_piProcInfo.hThread) {
            CloseHandle(m->m_piProcInfo.hThread);
            m->m_piProcInfo.hThread = NULL;
        }
    }
    return (int)exit_code;
}

bool ExecCmd::maybereap(int *status)
{
    ExecCmdRsrc e(this->m);
    *status = -1;

    if (m->m_piProcInfo.hProcess == NULL) {
        // Already waited for ??
        return true;
    }

    WaitResult res = Wait(m->m_piProcInfo.hProcess, 1);
    DWORD exit_code = -1;
    switch (res) {
    case Ok:
        exit_code = 0;
        GetExitCodeProcess(m->m_piProcInfo.hProcess, &exit_code);
        *status = (int)exit_code;
        CloseHandle(m->m_piProcInfo.hProcess);
        m->m_piProcInfo.hProcess = NULL;
        if (m->m_piProcInfo.hThread) {
            CloseHandle(m->m_piProcInfo.hThread);
            m->m_piProcInfo.hThread = NULL;
        }
        return true;
    default:
        e.inactivate();
        return false;
    }
}

// Static
bool ExecCmd::backtick(const vector<string> cmd, string& out)
{
    vector<string>::const_iterator it = cmd.begin();
    it++;
    vector<string> args(it, cmd.end());
    ExecCmd mexec;
    int status = mexec.doexec(*cmd.begin(), args, 0, &out);
    return status == 0;
}

int ExecCmd::doexec(const string &cmd, const vector<string>& args,
                    const string *input, string *output)
{
    if (input && output) {
        LOGERR(("ExecCmd::doexec: can't do both input and output\n"));
        return -1;
    }

    // The assumption is that the state is clean when this method
    // returns.  Have a cleaner ready in case we return without
    // calling wait() e.g. if an exception is raised in receive()
    ExecCmdRsrc cleaner(this->m);

    if (startExec(cmd, args, input != 0, output != 0) < 0) {
        return -1;
    }
    
    if (input) {
        if (!input->empty()) {
            if (send(*input) != (int)input->size()) {
                LOGERR(("ExecCmd::doexec: send failed\n"));
                CloseHandle(m->m_hInputWrite);
                m->m_hInputWrite = NULL;
                return -1;
            }
        }
        if (m->m_provide) {
            for (;;) {
                m->m_provide->newData();
                if (input->empty()) {
                    CloseHandle(m->m_hInputWrite);
                    m->m_hInputWrite = NULL;
                    break;
                }
                if (send(*input) != (int)input->size()) {
                    LOGERR(("ExecCmd::doexec: send failed\n"));
                    CloseHandle(m->m_hInputWrite);
                    m->m_hInputWrite = NULL;
                    break;
                }
            }
        }
    } else if (output) {
        receive(*output);
    }
    cleaner.inactivate();
    return wait();
}
