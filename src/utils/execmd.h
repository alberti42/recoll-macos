#ifndef _EXECMD_H_INCLUDED_
#define _EXECMD_H_INCLUDED_
/* @(#$Id: execmd.h,v 1.1 2004-12-12 08:58:12 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

class ExecCmd {
 public:
    //    ExecCmd() : argv(0) {};
    //    ~ExeCmd() {delete [] argv;}
    int doexec(const std::string &cmd, const std::list<std::string> a, 
	       const std::string *input = 0, 
	       std::string *output = 0);
};


#endif /* _EXECMD_H_INCLUDED_ */
