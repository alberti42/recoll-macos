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
/* @(#$Id: rcldb.h,v 1.49 2007-06-08 16:05:25 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>
#include <vector>

#include "refcntr.h"
#include "rcldoc.h"
#include "stoplist.h"

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

class SearchData;
class Native;
class TermIter;

class TermMatchEntry {
public:
    TermMatchEntry() : wcf(0) {}
    TermMatchEntry(const string&t, int f) : term(t), wcf(f) {}
    TermMatchEntry(const string&t) : term(t), wcf(0) {}
    bool operator==(const TermMatchEntry &o) { return term == o.term;}
    bool operator<(const TermMatchEntry &o) { return term < o.term;}
    string term;
    int    wcf; // Within collection frequency
};

/**
 * Wrapper class for the native database.
 */
class Db {
 public:

    /* General stuff (valid for query or update) ****************************/
    Db();
    ~Db();

    enum OpenMode {DbRO, DbUpd, DbTrunc};
    // KEEP_UPDATED is internal use by reOpen() only
    enum QueryOpts {QO_NONE=0, QO_STEM = 1, QO_KEEP_UPDATED = 8};

    bool open(const string &dbdir, const string &stoplistfn, 
	      OpenMode mode, int qops = QO_NONE);
    bool close();
    bool isopen();

    /** Retrieve main database directory */
    string getDbDir();

    /** Get explanation about last error */
    string getReason() const {return m_reason;}

    /** Return list of configured stop words */
    const StopList& getStopList() const {return m_stops;}


    /* Update-related methods ******************************************/

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

    /** Adjust flush threshold */
    void setFlushMb(int mb) {m_flushmb = mb;}


    /* Query-related methods ************************************/

    /** Return total docs in db */
    int  docCnt(); 

    // Parse query string and initialize query
    bool setQuery(RefCntr<SearchData> q, int opts = QO_NONE,
		  const string& stemlang = "english");
    bool getQueryTerms(list<string>& terms);
    bool getMatchTerms(const Doc& doc, list<string>& terms);

    /** Return a list of index terms that match the input string
     * Expansion is performed either with either wildcard or regexp processing
     * Stem expansion is performed if lang is not empty */
    enum MatchType {ET_WILD, ET_REGEXP, ET_STEM};
    bool termMatch(MatchType typ, const string &lang, const string &s, 
		   list<TermMatchEntry>& result, int max = -1);

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

    /* Build synthetic abstract out of query terms and term position data */
    bool makeDocAbstract(Doc &doc, string& abstract);

    /** Get document for given filename and ipath */
    bool getDoc(const string &fn, const string &ipath, Doc &doc, int *percent);

    /** Expand query */
    list<string> expand(const Doc &doc);

    /** Get results count for current query */
    int getResCnt();

    /** Get a list of existing stemming databases */
    std::list<std::string> getStemLangs();

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
    
    /** Filename wildcard expansion */
    bool filenameWildExp(const string& exp, list<string>& names);

private:

    string m_filterTopDir; // Current query filter on subtree top directory 
    vector<int> m_dbindices; // In case there is a postq filter: sequence of 
                             // db indices that match

    string m_reason; // Error explanation

    // A place for things we don't want visible here.
    friend class Native;
    Native *m_ndb; 

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

    // Flush threshold. Megabytes of text indexed before we flush.
    int          m_flushmb;
    // Text bytes indexed since last flush
    long long    m_curtxtsz;

    // Database directory
    string m_basedir;

    // List of directories for additional databases to query
    list<string> m_extraDbs;

    OpenMode m_mode;

    vector<bool> updated;

    StopList m_stops;

    bool reOpen(); // Close/open, same mode/opts
    bool stemExpand(const string &lang, const string &s, 
		    list<TermMatchEntry>& result, int max = -1);

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
