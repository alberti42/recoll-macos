#ifndef _UNACPP_H_INCLUDED_
#define _UNACPP_H_INCLUDED_
/* @(#$Id: unacpp.h,v 1.1 2004-12-17 15:36:13 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

// A small wrapper for unac.c
extern bool unac_cpp(const std::string &in, std::string &out, 
		     const char *encoding = "UTF-8");

#endif /* _UNACPP_H_INCLUDED_ */
