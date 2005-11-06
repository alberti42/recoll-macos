#ifndef _PATHHASH_H_INCLUDED_
#define _PATHHASH_H_INCLUDED_
/* @(#$Id: pathhash.h,v 1.1 2005-11-06 11:16:52 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

extern void pathHash(const std::string &path, std::string &hash, 
		     unsigned int len);


#endif /* _PATHHASH_H_INCLUDED_ */
