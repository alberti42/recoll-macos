#ifndef _MIMEHANDLER_H_INCLUDED_
#define _MIMEHANDLER_H_INCLUDED_
/* @(#$Id: mimehandler.h,v 1.8 2005-11-16 15:07:20 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

#include "rclconfig.h"
#include "rcldb.h"


/**
 * Document interner class. 
 */
class MimeHandler {
 public:
    virtual ~MimeHandler() {}

    /// Status from mkDoc method.
    enum Status {MHError, MHDone, MHAgain};
    /**
     * Transform external data into internal utf8 document
     *
     * @param conf the global configuration
     * @param filename File from which the data comes from
     * @param mimetype its mime type (from the mimemap configuration file)
     * @param outdoc   The output document
     * @param ipath the access path for the document inside the file. 
     *              For mono-document file types, this will always be empty. 
     *              It is used, for example for mbox files which may contain
     *              multiple emails. If this is not empty in input, then the
     *              caller is requesting a single document (ie: for display).
     *              If this is empty (during indexation), it will be filled-up
     *              by the function, and all the file's documents will be 
     *              returned by successive calls.
     * @return The return value indicates if there are more documents to be 
     *         fetched from the same file.
     */
    virtual MimeHandler::Status mkDoc(RclConfig * conf, 
				      const std::string &filename, 
				      const std::string &mimetype, 
				      Rcl::Doc& outdoc,
				      string& ipath) = 0;
};

/**
 * Return indexing handler object for the given mime type
 * returned pointer should be deleted by caller
 */
extern MimeHandler *getMimeHandler(const std::string &mtyp, ConfTree *mhdlers);

/**
 * Return external viewer exec string for given mime type
 */
extern std::string getMimeViewer(const std::string &mtyp, ConfTree *mhandlers);

/**
 * Return icon name
 */
extern std::string getMimeIconName(const std::string &mtyp, ConfTree *mhandlers);


/** 
 * Return command to uncompress the given type. The returned command has
 * substitutable places for input file name and temp dir name, and will
 * return output name
 */
bool getUncompressor(const std::string &mtype, ConfTree *mhandlers,
		     std::list<std::string>& cmd);

#endif /* _MIMEHANDLER_H_INCLUDED_ */
