#ifndef _MIMEHANDLER_H_INCLUDED_
#define _MIMEHANDLER_H_INCLUDED_
/* @(#$Id: mimehandler.h,v 1.5 2005-02-04 09:39:44 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

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
extern MimeHandler *getMimeHandler(const std::string &mtyp, ConfTree *mhdlers);

/**
 * Return external viewer exec string for given mime type
 */
extern std::string getMimeViewer(const std::string &mtyp, ConfTree *mhandlers);

bool getUncompressor(const std::string &mtype, ConfTree *mhandlers,
		     std::list<std::string>& cmd);

#endif /* _MIMEHANDLER_H_INCLUDED_ */
