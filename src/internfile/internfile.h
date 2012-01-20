/* Copyright (C) 2004 J.F.Dockes
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef _INTERNFILE_H_INCLUDED_
#define _INTERNFILE_H_INCLUDED_

#include <string>
#include <vector>
#include <map>
#include <set>
using std::string;
using std::vector;
using std::map;
using std::set;

#include "pathut.h"
#include "Filter.h"
// Beware: the class changes according to RCL_USE_XATTR, so any file
// including this needs autoconfig.h
#include "autoconfig.h"

class RclConfig;
namespace Rcl {
class Doc;
}

struct stat;

/** Storage for missing helper program info. We want to keep this out of the 
 * FileInterner class, because the data will typically be accumulated by several
 * FileInterner objects. Can't use static member either (because there
 * may be several separate usages of the class which shouldn't mix
 * their data).
 */
class FIMissingStore {
public:
    // Missing external programs
    set<string>           m_missingExternal;
    map<string, set<string> > m_typesForMissing;
};

/** 
 * A class to convert data from a datastore (file-system, firefox
 * history, etc.)  into possibly one or severaldocuments in internal
 * representation, either for indexing or viewing at query time (gui preview).
 * Things work a little differently when indexing or previewing:
 *  - When indexing, all data has to come from the datastore, and it is 
 *    normally desired that all found subdocuments be returned (ie:
 *    all messages and attachments out of a single file mail folder)
 *  - When previewing, some data is taken from the index (ie: the mime type 
 *    is already known, and a single document usually needs to be processed,
 *    so that the full doc identifier is passed in: high level url 
 *    (ie: file path) and internal identifier: ipath, ie: message and 
 *    attachment number.
 */
class FileInterner {
 public:
    /// Operation modifier flags
    enum Flags {FIF_none, FIF_forPreview, FIF_doUseInputMimetype};
    /// Return values for internfile()
    enum Status {FIError, FIDone, FIAgain};

    /**
     * Get immediate parent for document. 
     *
     * This is not in general the same as the "parent" document used 
     * with Rcl::Db::addOrUpdate(). The latter is generally the enclosing file,
     * this would be for exemple the email containing the attachment.
     */
    static bool getEnclosing(const string &url, const string &ipath,
			     string &eurl, string &eipath, string& udi);

    /** Constructors take the initial step to preprocess the data object and
     *  create the top filter */

    /**
     * Identify and possibly decompress file, and create the top filter.
     * - The mtype parameter is not always set (it is when the object is
     *   created for previewing a file). 
     * - Filter output may be different for previewing and indexing.
     *
     * @param fn file name 
     * @param stp pointer to updated stat struct.
     * @param cnf Recoll configuration
     * @param td  temporary directory to use as working space if 
     *   decompression needed. Must be private and will be wiped clean.
     * @param mtype mime type if known. For a compressed file this is the 
     *   mime type for the uncompressed version. 
     */
    FileInterner(const string &fn, const struct stat *stp, 
		 RclConfig *cnf, TempDir &td, int flags,
		 const string *mtype = 0);
    
    /** 
     * Alternate constructor for the case where the data is in memory.
     * This is mainly for data extracted from the web cache. The mime type
     * must be set, input must be uncompressed.
     */
    FileInterner(const string &data, RclConfig *cnf, TempDir &td, 
                 int flags, const string& mtype);

    /**
     * Alternate constructor for the case where it is not known where
     * the data will come from. We'll use the doc fields and try our
     * best. This is only used at query time, the idoc was built from index 
     * data.
     */
    FileInterner(const Rcl::Doc& idoc, RclConfig *cnf, TempDir &td, 
                 int flags);

    /** 
     * Build sig for doc coming from rcldb. This is here because we know how
     * to query the right backend */
    static bool makesig(const Rcl::Doc& idoc, string& sig);

    ~FileInterner();

    void setMissingStore(FIMissingStore *st)
    {
	m_missingdatap = st;
    }

    /** 
     * Turn file or file part into Recoll document.
     * 
     * For multidocument files (ie: mail folder), this must be called multiple
     * times to retrieve the subdocuments
     * @param doc output document
     * @param ipath internal path. If set by caller, the specified subdoc will
     *  be returned. Else the next document according to current state will 
     *  be returned, and doc.ipath will be set on output.
     * @return FIError and FIDone are self-explanatory. If FIAgain is returned,
     *  this is a multi-document file, with more subdocs, and internfile() 
     *  should be called again to get the following one(s).
     */
    Status internfile(Rcl::Doc& doc, const string &ipath = "");

    /** Return the file's (top level object) mimetype (useful for 
     *  container files) 
     */ 
    const string&  getMimetype() {return m_mimetype;}

    /** We normally always return text/plain data. A caller can request
     *  that we stop conversion at the native document type (ie: extracting
     *  an email attachment and starting an external viewer)
     */
    void setTargetMType(const string& tp) {m_targetMType = tp;}

    /** In case we see an html version while converting, it is set aside 
     *  and can be recovered 
     */
    const string& get_html() {return m_html;}

    /** If we happen to be processing an image file and need a temp file,
	we keep it around to save work for our caller, which can get it here */
    TempFile get_imgtmp() {return m_imgtmp;}

    /** Extract internal document into temporary file. 
     *  This is used mainly for starting an external viewer for a
     *  subdocument (ie: mail attachment).
     * @return true for success.
     * @param temp output reference-counted temp file object (goes
     *   away magically). Only used if tofile.empty()
     * @param tofile output file if not null
     * @param cnf The recoll config
     * @param doc Doc data taken from the index. We use it to access the 
     *            actual document (ie: use mtype, fn, ipath...).
     */
    static bool idocToFile(TempFile& temp, const string& tofile, 
			   RclConfig *cnf, const Rcl::Doc& doc);

    /** 
     * Does file appear to be the compressed version of a document?
     */
    static bool isCompressed(const string& fn, RclConfig *cnf);

    /** 
     * Check input compressed, allocate temp file and uncompress if it is.  
     * @return true if ok, false for error. Actual decompression is indicated
     *  by the TempFile status (!isNull())
     */
    static bool maybeUncompressToTemp(TempFile& temp, const string& fn, 
                                      RclConfig *cnf, const Rcl::Doc& doc);

    const string& getReason() const {return m_reason;}
    static void getMissingExternal(FIMissingStore *st, string& missing);
    static void getMissingDescription(FIMissingStore *st, string& desc);
    // Parse "missing" file contents into memory struct
    static void getMissingFromDescription(FIMissingStore *st, const string& desc);
    bool ok() {return m_ok;}

 private:
    static const unsigned int MAXHANDLERS = 20;
    RclConfig             *m_cfg;
    string                 m_fn;
    string                 m_mimetype; // Mime type for [uncompressed] file
    bool                   m_forPreview;
    string                 m_html; // Possibly set-aside html text for preview
    TempFile               m_imgtmp; // Possible reference to an image temp file
    string                 m_targetMType;
    string                 m_reachedMType; // target or text/plain
    // m_tdir and m_tfile are used only for decompressing input file if needed
    TempDir                &m_tdir; 
    string                 m_tfile;
    bool                   m_ok; // Set after construction if ok
#ifdef RCL_USE_XATTR
    // Fields found in file extended attributes. This is kept here,
    // not in the file-level handler because we are only interested in
    // the top-level file, not any temp file necessitated by
    // processing the internal doc hierarchy.
    map<string, string> m_XAttrsFields;
#endif // RCL_USE_XATTR

    // Filter stack, path to the current document from which we're
    // fetching subdocs
    vector<Dijon::Filter*> m_handlers;
    // Temporary files used for decoding the current stack
    bool                   m_tmpflgs[MAXHANDLERS];
    vector<TempFile>       m_tempfiles;
    // Error data if any
    string                 m_reason;
    FIMissingStore        *m_missingdatap;

    // Pseudo-constructors
    void init(const string &fn, const struct stat *stp, 
              RclConfig *cnf, int flags, const string *mtype = 0);
    void init(const string &data, RclConfig *cnf, int flags, 
	      const string& mtype);
    void initcommon(RclConfig *cnf, int flags);

    void tmpcleanup();
    bool dijontorcl(Rcl::Doc&);
    void collectIpathAndMT(Rcl::Doc&) const;
    bool dataToTempFile(const string& data, const string& mt, string& fn);
    void popHandler();
    int addHandler();
    void checkExternalMissing(const string& msg, const string& mt);
    void processNextDocError(Rcl::Doc &doc);
#ifdef RCL_USE_XATTR
    void reapXAttrs(const string& fn);
#endif
};

 
#endif /* _INTERNFILE_H_INCLUDED_ */
