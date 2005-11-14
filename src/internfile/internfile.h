#ifndef _INTERNFILE_H_INCLUDED_
#define _INTERNFILE_H_INCLUDED_
/* @(#$Id: internfile.h,v 1.4 2005-11-14 09:59:17 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

#include "rclconfig.h"
#include "rcldb.h"

class MimeHandler;

/// Turn external file into internal representation, according to mime
/// type etc
class FileInterner {
    string fn;
    RclConfig *config;
    const string &tdir;
    MimeHandler *handler;
    string tfile;
    string mime;

    void tmpcleanup();

 public:
    /**
     * Identify and possibly decompress file, create adequate handler
     * @param fn file name 
     * @param cnf Recoll configuration
     * @param td  temporary directory to use as working space for possible 
     *           decompression
     * @param mimetype mime type if known. For a compressed file this is the 
     *   mime type for the uncompressed version.
     */
    FileInterner(const std::string &fn, RclConfig *cnf, const string& td,
		 const std::string *mtype = 0);

    ~FileInterner();

    /// Return values for internfile()
    enum Status {FIError, FIDone, FIAgain};

    /** 
     * Turn file or file part into Recoll document.
     * 
     * For multidocument files (ie: mail folder), this must be called multiple
     * times to retrieve the subdocuments
     * @param doc output document
     * @param ipath internal path. If set by caller, the specified subdoc will
     *  be returned. Else the next document according to current state will 
     *  be returned, and the internal path will be set.
     * @return FIError and FIDone are self-explanatory. If FIAgain is returned,
     *  this is a multi-document file, with more subdocs, and internfile() 
     * should be called again to get the following one(s).
     */
    Status internfile(Rcl::Doc& doc, string &ipath);
};

#endif /* _INTERNFILE_H_INCLUDED_ */
