#ifndef _INDEXTEXT_H_INCLUDED_
#define _INDEXTEXT_H_INCLUDED_
/* @(#$Id: indextext.h,v 1.1 2005-01-28 15:25:39 dockes Exp $  (C) 2004 J.F.Dockes */
/* Note: this only exists to help with using myhtmlparse.cc */

// Minimize changes to myhtmlparse.cpp
#include "debuglog.h"

#include <string>

// lets hope that the charset includes ascii values...
static inline void
lowercase_term(std::string &term)
{
    std::string::iterator i = term.begin();
    while (i != term.end()) {
	if (*i >= 'A' && *i <= 'Z')
	    *i = *i + 'a' - 'A';
        i++;
    }
}

#endif /* _INDEXTEXT_H_INCLUDED_ */
