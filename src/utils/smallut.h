#ifndef _SMALLUT_H_INCLUDED_
#define _SMALLUT_H_INCLUDED_
/* @(#$Id: smallut.h,v 1.4 2005-02-10 15:21:12 dockes Exp $  (C) 2004 J.F.Dockes */
#include <string>
#include <list>
using std::string;
using std::list;

extern int stringicmp(const string& s1, const string& s2);
extern int stringlowercmp(const string& alreadylower, const string& s2);
extern int stringuppercmp(const string& alreadyupper, const string& s2); 

extern bool maketmpdir(string& tdir);
extern string stringlistdisp(const list<string>& strs);

#endif /* _SMALLUT_H_INCLUDED_ */
