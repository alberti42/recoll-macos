#ifndef _CSGUESS_H_INCLUDED_
#define _CSGUESS_H_INCLUDED_
/* @(#$Id: csguess.h,v 1.1 2004-12-15 08:21:05 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

// Try to guess the character set. This might guess unicode encodings, and
// some asian charsets, but has no chance, for example, of discriminating
// betweeen the different iso8859-xx charsets.
extern std::string csguess(const std::string &in);

#endif /* _CSGUESS_H_INCLUDED_ */
