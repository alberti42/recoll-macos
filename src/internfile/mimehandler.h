#ifndef _MIMEHANDLER_H_INCLUDED_
#define _MIMEHANDLER_H_INCLUDED_
/* @(#$Id: mimehandler.h,v 1.2 2005-01-26 11:47:27 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

#include "rclconfig.h"
#include "rcldb.h"

/* Definition for document interner functions */
typedef bool (*MimeHandlerFunc)(RclConfig *, const std::string &, 
				const std::string &, Rcl::Doc&);

extern MimeHandlerFunc getMimeHandler(const std::string &mtype, 
				      ConfTree *mhandlers);

extern bool textHtmlToDoc(RclConfig *conf, const string &fn, 
			  const string &mtype, Rcl::Doc &docout);

#endif /* _MIMEHANDLER_H_INCLUDED_ */
