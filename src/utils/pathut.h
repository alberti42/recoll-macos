#ifndef _PATHUT_H_INCLUDED_
#define _PATHUT_H_INCLUDED_
/* @(#$Id: pathut.h,v 1.2 2004-12-14 17:54:16 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

inline void path_catslash(std::string &s) {
    if (s.empty() || s[s.length() - 1] != '/')
	s += '/';
}
inline void path_cat(std::string &s1, const std::string &s2) {
    path_catslash(s1);
    s1 += s2;
}
		     
extern std::string path_getsimple(const std::string &s);
extern std::string path_getfather(const std::string &s);
extern std::string path_home();

#endif /* _PATHUT_H_INCLUDED_ */
