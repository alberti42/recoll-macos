#ifndef _UNACPP_H_INCLUDED_
#define _UNACPP_H_INCLUDED_
/* @(#$Id: unacpp.h,v 1.2 2006-01-05 16:37:26 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

// A small wrapper for unac.c
extern bool unac_cpp(const std::string &in, std::string &out, 
		     const char *encoding = "UTF-8");
extern bool unac_cpp_utf16be(const std::string &in, std::string &out);

#endif /* _UNACPP_H_INCLUDED_ */
