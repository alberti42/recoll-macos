#ifndef _SMALLUT_H_INCLUDED_
#define _SMALLUT_H_INCLUDED_
/* @(#$Id: smallut.h,v 1.3 2005-02-09 12:07:30 dockes Exp $  (C) 2004 J.F.Dockes */
#include <string>

using std::string;

extern int stringicmp(const string& s1, const string& s2);
extern int stringlowercmp(const string& alreadylower, const string& s2);
extern int stringuppercmp(const string& alreadyupper, const string& s2); 

extern bool maketmpdir(string& tdir);

#endif /* _SMALLUT_H_INCLUDED_ */
