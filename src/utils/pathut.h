#ifndef _PATHUT_H_INCLUDED_
#define _PATHUT_H_INCLUDED_
/* @(#$Id: pathut.h,v 1.5 2006-01-09 16:53:31 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

extern void path_catslash(std::string &s);
extern std::string path_cat(const std::string &s1, const std::string &s2);
extern std::string path_getsimple(const std::string &s);
extern std::string path_basename(const std::string &s, const std::string &suff="");
extern std::string path_getfather(const std::string &s);
extern std::string path_home();
extern std::string path_tildexpand(const std::string &s);

extern std::string path_canon(const std::string &s);
extern std::list<std::string> path_dirglob(const std::string &dir, 
					   const std::string pattern);
#endif /* _PATHUT_H_INCLUDED_ */
