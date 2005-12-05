#ifndef _COPYFILE_H_INCLUDED_
#define _COPYFILE_H_INCLUDED_
/* @(#$Id: copyfile.h,v 1.1 2005-12-05 14:09:16 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

enum CopyfileFlags {COPYFILE_NONE = 0, COPYFILE_NOERRUNLINK = 1};

extern bool copyfile(const char *src, const char *dst, std::string &reason,
		     int flags = 0);

#endif /* _COPYFILE_H_INCLUDED_ */
