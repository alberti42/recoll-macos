#ifndef _INTERNFILE_H_INCLUDED_
#define _INTERNFILE_H_INCLUDED_
/* @(#$Id: internfile.h,v 1.2 2005-02-09 12:07:29 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

#include "rclconfig.h"
#include "rcldb.h"

/// Turn external file into internal representation, according to mime type etc
extern bool internfile(const std::string &fn, RclConfig *config, 
		       Rcl::Doc& doc, const string& tdir);

#endif /* _INTERNFILE_H_INCLUDED_ */
