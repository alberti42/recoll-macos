#ifndef _SMALLUT_H_INCLUDED_
#define _SMALLUT_H_INCLUDED_
/* @(#$Id: smallut.h,v 1.6 2005-04-06 10:20:11 dockes Exp $  (C) 2004 J.F.Dockes */
#include <string>
#include <list>
#include <iostream>
using std::string;
using std::list;
using std::ostream;
extern int stringicmp(const string& s1, const string& s2);
extern int stringlowercmp(const string& alreadylower, const string& s2);
extern int stringuppercmp(const string& alreadyupper, const string& s2); 

extern bool maketmpdir(string& tdir);
extern string stringlistdisp(const list<string>& strs);

#endif /* _SMALLUT_H_INCLUDED_ */
