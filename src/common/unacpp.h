#ifndef _UNACPP_H_INCLUDED_
#define _UNACPP_H_INCLUDED_
/* @(#$Id: unacpp.h,v 1.3 2006-01-06 13:18:17 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

// A small stringified wrapper for unac.c
extern bool unacmaybefold(const std::string &in, std::string &out, 
			  const char *encoding, bool dofold);
#endif /* _UNACPP_H_INCLUDED_ */
