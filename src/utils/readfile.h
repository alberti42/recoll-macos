#ifndef _READFILE_H_INCLUDED_
#define _READFILE_H_INCLUDED_
/* @(#$Id: readfile.h,v 1.1 2004-12-14 17:54:16 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

/**
 * Read whole file into string. 
 * @return true for ok, false else
 */
bool file_to_string(const std::string &filename, std::string &data);

#endif /* _READFILE_H_INCLUDED_ */
