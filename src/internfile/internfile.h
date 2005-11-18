#ifndef _INTERNFILE_H_INCLUDED_
#define _INTERNFILE_H_INCLUDED_
/* @(#$Id: internfile.h,v 1.5 2005-11-18 15:19:14 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

#include "rclconfig.h"
#include "rcldb.h"

class MimeHandler;

/// Turn external file into internal representation, according to mime
/// type etc
class FileInterner {
 public:
    /**
     * Identify and possibly decompress file, create adequate
     * handler. The mtype parameter is only set when the object is
     * created for previewing a file. Filter output may be
     * different for previewing and indexing.
     *
     * @param fn file name 
     * @param cnf Recoll configuration
     * @param td  temporary directory to use as working space if 
     *            decompression needed.
     * @param mtype mime type if known. For a compressed file this is the 
     *   mime type for the uncompressed version. This currently doubles up 
     *   to indicate that this object is for previewing (not indexing).
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

 private:
    string m_fn;
    RclConfig *m_cfg;
    const string &m_tdir;
    MimeHandler *m_handler;

    string m_tfile;
    string m_mime;

    void tmpcleanup();
};

#endif /* _INTERNFILE_H_INCLUDED_ */
