#ifndef _MIMEHANDLER_H_INCLUDED_
#define _MIMEHANDLER_H_INCLUDED_
/* @(#$Id: mimehandler.h,v 1.4 2005-02-01 17:20:05 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

#include "rclconfig.h"
#include "rcldb.h"


/**
 * Document interner class. We sometimes have data to pass to an interner
 */
class MimeHandler {
 public:
    virtual ~MimeHandler() {}
    virtual bool worker(RclConfig *, const std::string &filename, 
			const std::string &mimetype, Rcl::Doc& outdoc) = 0;
};

/**
 * Return indexing handler class for given mime type
 * returned pointer should be deleted by caller
 */
extern MimeHandler *getMimeHandler(const std::string &mtype, 
				   ConfTree *mhandlers);

/**
 * Return external viewer exec string for given mime type
 */
extern string getMimeViewer(const std::string &mtype, 
			    ConfTree *mhandlers);

#endif /* _MIMEHANDLER_H_INCLUDED_ */
