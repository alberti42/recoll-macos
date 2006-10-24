/*
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
#ifndef _DB_H_INCLUDED_
#define _DB_H_INCLUDED_
/* @(#$Id: rcldb.h,v 1.39 2006-10-24 09:28:31 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>
#include <vector>

#ifndef NO_NAMESPACES
using std::string;
using std::list;
using std::vector;
#endif

// rcldb defines an interface for a 'real' text database. The current 
// implementation uses xapian only, and xapian-related code is in rcldb.cpp
// If support was added for other backend, the xapian code would be moved in 
// rclxapian.cpp, another file would be created for the new backend, and the
// configuration/compile/link code would be adjusted to allow choosing. There 
// is no plan for supporting multiple different backends.
// 
// In no case does this try to implement a useful virtualized text-db interface
// The main goal is simplicity and good matching to usage inside the recoll
// user interface. In other words, this is not exhaustive or well-designed or 
// reusable.


struct stat;

#ifndef NO_NAMESPACES
namespace Rcl {
#endif

/**
 * Dumb holder for document attributes and data
 */
class Doc {
 public:
    // These fields potentially go into the document data record
    // We indicate the routine that sets them up during indexing
    string url;          // This is just "file://" + binary filename. 
                         // No transcoding: this is used to access files
                         // Computed from fn by Db::add
    string utf8fn;       // Transcoded version of the simple file name for
                         // SFN-prefixed specific file name indexation
                         // Set by DbIndexer::processone
    string ipath;        // Internal path for multi-doc files. Ascii
                         // Set by DbIndexer::processone
    string mimetype;     // Set by FileInterner::internfile
    string fmtime;       // File modification time as decimal ascii unix time
                         // Set by DbIndexer::processone
    string dmtime;       // Data reference date (same format). Ie: mail date
                         // Possibly set by handler
    string origcharset;  // Charset we transcoded from (in case we want back)
                         // Possibly set by handler
    string title;        // Possibly set by handler
    string keywords;     // Possibly set by handler
    string abstract;     // Possibly set by handler
    string fbytes;       // File size. Set by Db::Add
    string dbytes;       // Doc size. Set by Db::Add from text length

    // The following fields don't go to the db record
    
    string text; // During indexing only: text returned by input handler will
                 // be split and indexed 

    int pc; // used by sortseq, convenience
    unsigned long xdocid; // Opaque: rcldb doc identifier.

    void erase() {
	url.erase();
	utf8fn.erase();
	ipath.erase();
	mimetype.erase();
	fmtime.erase();
	dmtime.erase();
	origcharset.erase();
	title.erase();
	keywords.erase();
	abstract.erase();
	fbytes.erase();
	dbytes.erase();

	text.erase();
	pc = 0;
	xdocid = 0;
    }
};

class AdvSearchData;
class Native;
class TermIter;
 
/**
 * Wrapper class for the native database.
 */
class Db {
 public:
    Db();
    ~Db();

    enum OpenMode {DbRO, DbUpd, DbTrunc};
    // KEEP_UPDATED is internal use by reOpen() only
    enum QueryOpts {QO_NONE=0, QO_STEM = 1, QO_BUILD_ABSTRACT = 2,
		    QO_REPLACE_ABSTRACT = 4, QO_KEEP_UPDATED = 8};

    bool open(const string &dbdir, OpenMode mode, int qops = QO_NONE);
    bool close();
    bool isopen();

    /** Return total docs in db */
    int  docCnt(); 


    /* Update-related functions */

    /** Add document. The Doc class should have been filled as much as
       possible depending on the document type */
    bool add(const string &filename, const Doc &doc, const struct stat *stp);

    /** Test if the db entry for the given filename/stat is up to date */
    bool needUpdate(const string &filename, const struct stat *stp);

    /** Remove documents that no longer exist in the file system. This
	depends on the update map, which is built during
	indexation. This should only be called after a full walk of
	the file system, else the update map will not be complete, and
	many documents will be deleted that shouldn't */
    bool purge();

    /** Delete document(s) for given filename */
    bool purgeFile(const string &filename);

    /** Create stem expansion database for given language. */
    bool createStemDb(const string &lang);
    /** Delete stem expansion database for given language. */
    bool deleteStemDb(const string &lang);

    /* Query-related functions */

    // Parse query string and initialize query
    bool setQuery(AdvSearchData &q, int opts = QO_NONE,
		  const string& stemlang = "english");
    bool getQueryTerms(list<string>& terms);
    bool getMatchTerms(const Doc& doc, list<string>& terms);

    // Return a list of database terms that begin with the input string
    // Stem expansion is performed if lang is not empty
    list<string> completions(const string &s, const string &lang, int max=20);

    /** Add extra database for querying */
    bool addQueryDb(const string &dir);
    /** Remove extra database. if dir == "", remove all. */
    bool rmQueryDb(const string &dir);
    /** Tell if directory seems to hold xapian db */
    static bool testDbDir(const string &dir);

    /** Get document at rank i in current query. 

	This is probably vastly inferior to the type of interface in
	Xapian, but we have to start with something simple to
	experiment with the GUI. i is sequential from 0 to some value.
    */
    bool getDoc(int i, Doc &doc, int *percent = 0);

    /** Get document for given filename and ipath */
    bool getDoc(const string &fn, const string &ipath, Doc &doc, int *percent);

    /** Expand query */
    list<string> expand(const Doc &doc);

    /** Get results count for current query */
    int getResCnt();

    /** Get a list of existing stemming databases */
    std::list<std::string> getStemLangs();

    /** Retrieve main database directory */
    string getDbDir();

    /** Set parameters for synthetic abstract generation */
    void setAbstractParams(int idxTrunc, int synthLen, int syntCtxLen);

    /** Whole term list walking. */
    TermIter *termWalkOpen();
    bool termWalkNext(TermIter *, string &term);
    void termWalkClose(TermIter *);
    /** Test term existence */
    bool termExists(const string& term);
    /** Test if terms stem to different roots. */
    bool stemDiffers(const string& lang, const string& term, 
		     const string& base);
    
    /** Perform stem expansion across all dbs configured for searching */
    list<string> stemExpand(const string& lang, const string& term);

private:

    string m_filterTopDir; // Current query filter on subtree top directory 
    vector<int> m_dbindices; // In case there is a postq filter: sequence of 
                             // db indices that match

    // Things we don't want to have here.
    friend class Native;
    Native *m_ndb; // Pointer to private data. We don't want db(ie
                 // xapian)-specific defs to show in here

    unsigned int m_qOpts;
    
    // This is how long an abstract we keep or build from beginning of
    // text when indexing. It only has an influence on the size of the
    // db as we are free to shorten it again when displaying
    int          m_idxAbsTruncLen;
    // This is the size of the abstract that we synthetize out of query
    // term contexts at *query time*
    int          m_synthAbsLen;
    // This is how many words (context size) we keep around query terms
    // when building the abstract
    int          m_synthAbsWordCtxLen;

    // Database directory
    string m_basedir;

    // List of directories for additional databases to query
    list<string> m_extraDbs;

    OpenMode m_mode;

    vector<bool> updated;

    bool reOpen(); // Close/open, same mode/opts
    /* Copyconst and assignemt private and forbidden */
    Db(const Db &) {}
    Db & operator=(const Db &) {return *this;};
};

// Unaccent and lowercase data.
extern bool dumb_string(const string &in, string &out);

#ifndef NO_NAMESPACES
}
#endif // NO_NAMESPACES


#endif /* _DB_H_INCLUDED_ */
