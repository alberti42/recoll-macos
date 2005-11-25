#ifndef _SMALLUT_H_INCLUDED_
#define _SMALLUT_H_INCLUDED_
/* @(#$Id: smallut.h,v 1.9 2005-11-25 08:50:39 dockes Exp $  (C) 2004 J.F.Dockes */
#include <string>
#include <list>

#ifndef NO_NAMESPACES
using std::string;
using std::list;
#endif /* NO_NAMESPACES */

extern int stringicmp(const string& s1, const string& s2);
extern int stringlowercmp(const string& alreadylower, const string& s2);
extern int stringuppercmp(const string& alreadyupper, const string& s2); 

// Compare charset names, removing the more common spelling variations
extern bool samecharset(const string &cs1, const string &cs2);

extern bool maketmpdir(string& tdir);
extern string stringlistdisp(const list<string>& strs);

/**
 * Parse input stream into list of strings. 
 *
 * Token delimiter is " \t" except inside dquotes. dquote inside
 * dquotes can be escaped with \ etc...
 */
extern bool stringToStrings(const string &s, std::list<string> &tokens);

extern bool stringToBool(const string &s);

#endif /* _SMALLUT_H_INCLUDED_ */
