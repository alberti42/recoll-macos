#ifndef _MIMEHANDLER_H_INCLUDED_
#define _MIMEHANDLER_H_INCLUDED_
/* @(#$Id: mimehandler.h,v 1.1 2005-01-25 14:37:57 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

#include "rclconfig.h"
#include "rcldb.h"

/* Definition for document interner functions */
typedef bool (*MimeHandlerFunc)(RclConfig *, const std::string &, 
				const std::string &, Rcl::Doc&);

extern MimeHandlerFunc getMimeHandler(const std::string &mtype, 
				      ConfTree *mhandlers);

#endif /* _MIMEHANDLER_H_INCLUDED_ */
