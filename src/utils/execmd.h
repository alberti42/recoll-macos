#ifndef _EXECMD_H_INCLUDED_
#define _EXECMD_H_INCLUDED_
/* @(#$Id: execmd.h,v 1.2 2005-03-17 14:02:06 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

class ExecCmd {
 public:
    //    ExecCmd() : argv(0) {};
    //    ~ExeCmd() {delete [] argv;}
    int doexec(const std::string &cmd, const std::list<std::string>& a, 
	       const std::string *input = 0, 
	       std::string *output = 0);
};


#endif /* _EXECMD_H_INCLUDED_ */
