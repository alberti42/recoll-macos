#ifndef _PATHUT_H_INCLUDED_
#define _PATHUT_H_INCLUDED_
/* @(#$Id: pathut.h,v 1.4 2005-12-13 12:42:59 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

extern void path_catslash(std::string &s);
extern std::string path_cat(const std::string &s1, const std::string &s2);
extern std::string path_getsimple(const std::string &s);
extern std::string path_getfather(const std::string &s);
extern std::string path_home();
extern std::string path_tildexpand(const std::string &s);

#endif /* _PATHUT_H_INCLUDED_ */
