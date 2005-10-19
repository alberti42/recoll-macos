#ifndef _IDFILE_H_INCLUDED_
#define _IDFILE_H_INCLUDED_
/* @(#$Id: idfile.h,v 1.2 2005-10-19 14:14:17 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

// Return mime type for file or empty string. The system's file utility does
// a bad job on mail folders. idFile only looks for mail file types for now, 
// but this may change
extern std::string idFile(const char *fn);

// Return all types known to us
extern std::list<std::string> idFileAllTypes();

#endif /* _IDFILE_H_INCLUDED_ */
