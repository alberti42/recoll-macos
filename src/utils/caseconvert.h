#ifndef _CASECONVERT_H_INCLUDED_
#define _CASECONVERT_H_INCLUDED_
/* @(#$Id: caseconvert.h,v 1.1 2006-01-05 16:16:14 dockes Exp $  (C) 2005 J.F.Dockes */
#include <string>

// Lower-case string
// Input and output must be utf-16be
extern bool ucs2lower(const std::string &in, std::string &out);

#endif /* _CASECONVERT_H_INCLUDED_ */
