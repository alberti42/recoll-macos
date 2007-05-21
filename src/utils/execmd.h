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
#ifndef _EXECMD_H_INCLUDED_
#define _EXECMD_H_INCLUDED_
/* @(#$Id: execmd.h,v 1.12 2007-05-21 13:30:22 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>
#include <vector>
#ifndef NO_NAMESPACES
using std::list;
using std::string;
using std::vector;
#endif

/** Callback function object to advise of new data arrival (or just heartbeat)  * if cnt is 0 */
class ExecCmdAdvise {
 public:
    virtual ~ExecCmdAdvise() {}
    virtual void newData(int cnt) = 0;
};

/** Callback function object to get more input data. Data has to be provided
 *  in the initial input string, set it to empty to signify eof
 */
class ExecCmdProvide {
 public:
    virtual ~ExecCmdProvide() {}
    virtual void newData() = 0;
};

/**
 * Execute command possibly taking both input and output (will do
 * asynchronous io as appropriate for things to work).
 *
 * Input to the command can be provided either once in a parameter to doexec
 * or provided in chunks by setting a callback which will be called to 
 * request new data. In this case, the 'input' parameter to doexec may be
 * empty (but not null)
 *
 * Output from the command is normally returned in a single string, but a
 * callback can be set to be called whenever new data arrives, in which case
 * it is permissible to consume the data and erase the string.
 * 
 * Note that SIGPIPE should be ignored and SIGCLD blocked when calling doexec,
 * else things might fail randomly. (This is not done inside the class because
 * of concerns with multithreaded programs).
 *
 */
class ExecCmd {
 public:
    /**
     * Execute command. 
     *
     * Both input and output can be specified, and asynchronous 
     * io (select-based) is used to prevent blocking. This will not
     * work if input and output need to be synchronized (ie: Q/A), but
     * works ok for filtering.
     * The function is exception-safe. In case an exception occurs in the 
     * advise callback, fds and pids will be cleaned-up properly.
     *
     * @param cmd the program to execute. This must be an absolute file name 
     *   or exist in the PATH.
     * @param args the argument list (NOT including argv[0]).
     * @param input Input to send TO the command.
     * @param output Output FROM the command.
     * @return the exec ouput status (0 if ok).
     */
    int doexec(const string &cmd, const list<string>& args, 
	       const string *input = 0, 
	       string *output = 0);
    /** 
     * Add/replace environment variable before executing command. This must
     * be called before doexec to have an effect (possibly multiple
     * times for several variables).
     * @param envassign an environment assignment string (name=value)
     */
    void putenv(const string &envassign);

    /** 
     * Set function objects to call whenever new data is available or on
     * select timeout / whenever new data is needed to send.
     */
    void setAdvise(ExecCmdAdvise *adv) {m_advise = adv;}
    void setProvide(ExecCmdProvide *p) {m_provide = p;}

    /**
     * Set select timeout in milliseconds. The default is 1 S. 
     */
    void setTimeout(int mS) {if (mS > 30) m_timeoutMs = mS;}

    /** 
     * Set destination for stderr data. The default is to let it alone (will 
     * usually go to the terminal or to wherever the desktop messages go).
     * There is currently no option to put stderr data into a program variable
     * If the parameter can't be opened for writing, the command's
     * stderr will be closed.
     */
    void setStderr(const string &stderrFile) {m_stderrFile = stderrFile;}

    /** 
     * Cancel/kill command. This can be called from another thread or
     * from the advise callback, which could also raise an exception to 
     * accomplish the same thing
     */
    void setCancel() {m_cancelRequest = true;}

    ExecCmd() 
	: m_advise(0), m_provide(0), m_cancelRequest(false), m_timeoutMs(1000)
	{}

    /**
     * Utility routine: check if/where a command is found according to the
     * current PATH
     */
    static bool which(const string& cmd, string& path);

 private:
    vector<string>   m_env;
    ExecCmdAdvise   *m_advise;
    ExecCmdProvide  *m_provide;
    bool             m_cancelRequest;
    int              m_timeoutMs;
    string           m_stderrFile;
};


#endif /* _EXECMD_H_INCLUDED_ */
