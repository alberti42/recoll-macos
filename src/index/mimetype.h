#ifndef _MIMETYPE_H_INCLUDED_
#define _MIMETYPE_H_INCLUDED_
/* @(#$Id: mimetype.h,v 1.3 2005-11-10 08:47:49 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include "conftree.h"


/**
 * Try to determine a mime type for filename. 
 * This may imply more than matching the suffix, the name must be usable
 * to actually access file data.
 */
string mimetype(const std::string &filename, ConfTree *mtypes, bool usfc);


#endif /* _MIMETYPE_H_INCLUDED_ */
