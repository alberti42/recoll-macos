#ifndef _CSGUESS_H_INCLUDED_
#define _CSGUESS_H_INCLUDED_
/* @(#$Id: csguess.h,v 1.2 2004-12-15 15:00:37 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>


// Try to guess the character set. This might guess unicode encodings, and
// some asian charsets, but has no chance, for example, of discriminating
// betweeen the different iso8859-xx charsets.
extern std::string csguess(const std::string &in, const std::string &dflt);

#endif /* _CSGUESS_H_INCLUDED_ */
