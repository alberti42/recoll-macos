#ifndef _MIMETYPE_H_INCLUDED_
#define _MIMETYPE_H_INCLUDED_
/* @(#$Id: mimetype.h,v 1.1 2004-12-13 15:42:16 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include "conftree.h"


/**
 * Try to determine a mime type for filename. 
 * This may imply more than matching the suffix, the name must be usable
 * to actually access file data.
 */
string mimetype(const std::string &filename, ConfTree *mtypes);

#endif /* _MIMETYPE_H_INCLUDED_ */
