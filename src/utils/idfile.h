#ifndef _IDFILE_H_INCLUDED_
#define _IDFILE_H_INCLUDED_
/* @(#$Id: idfile.h,v 1.1 2005-04-07 09:05:39 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

// Return mime type for file or empty string. The system's file utility does
// a bad job on mail folders. idFile only looks for mail file types for now, 
// but this may change
extern std::string idFile(const char *fn);

#endif /* _IDFILE_H_INCLUDED_ */
