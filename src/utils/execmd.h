#ifndef _EXECMD_H_INCLUDED_
#define _EXECMD_H_INCLUDED_
/* @(#$Id: execmd.h,v 1.4 2005-11-18 15:19:14 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

/**
 Execute command possibly taking both input and output (will do
 asynchronous io as appropriate for things to work).
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

 private:
    std::list<std::string> env;
};


#endif /* _EXECMD_H_INCLUDED_ */
