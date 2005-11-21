#ifndef _MIMETYPE_H_INCLUDED_
#define _MIMETYPE_H_INCLUDED_
/* @(#$Id: mimetype.h,v 1.4 2005-11-21 14:31:24 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

class RclConfig;

/**
 * Try to determine a mime type for filename. 
 * This may imply more than matching the suffix, the name must be usable
 * to actually access file data.
 */
string mimetype(const std::string &filename, RclConfig *cfg, bool usfc);


#endif /* _MIMETYPE_H_INCLUDED_ */
