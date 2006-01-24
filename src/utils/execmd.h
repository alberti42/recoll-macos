#ifndef _EXECMD_H_INCLUDED_
#define _EXECMD_H_INCLUDED_
/* @(#$Id: execmd.h,v 1.5 2006-01-24 12:22:20 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

/** Callback function object to advise of new data arrival */
class ExecCmdAdvise {
 public:
    virtual ~ExecCmdAdvise() {}
    virtual void newData() = 0;
};

/**
 * Execute command possibly taking both input and output (will do
 * asynchronous io as appropriate for things to work).
 */
class ExecCmd {
 public:
    /**
     * Execute command. Both input and output can be specified, and
     * asynchronous io is used to prevent blocking. This wont work if
     * input and output need to be synchronized (ie: Q/A), but ok for
     * filtering.
     * @param cmd the program to execute. This must be an absolute file name 
     *   or exist in the PATH.
     * @param args the argument list (NOT including argv[0]).
     * @param input Input to send to the command.
     * @param output Output from the command.
     * @return the exec ouput status (0 if ok).
     */
    int doexec(const std::string &cmd, const std::list<std::string>& args, 
	       const std::string *input = 0, 
	       std::string *output = 0);
    /** 
     * Add/replace environment variable before executing command. This should 
     * be called before doexec of course (possibly multiple times for several
     * variables).
     * @param envassign an environment assignment string (name=value)
     */
    void putenv(const std::string &envassign);

    /** Set function object to call whenever new data is available */
    void setAdvise(ExecCmdAdvise *adv) {m_advise = adv;}

    /** Cancel exec. This can be called from another thread or from newData */
    void setCancel() {m_cancelRequest = true;}

    ExecCmd() : m_advise(0), m_cancelRequest(false) {}

 private:
    std::list<std::string> m_env;
    ExecCmdAdvise *m_advise;
    bool m_cancelRequest;
};


#endif /* _EXECMD_H_INCLUDED_ */
