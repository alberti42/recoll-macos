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
#ifndef _DB_H_INCLUDED_
#define _DB_H_INCLUDED_

#include "autoconfig.h"

#include <string>
#include <vector>

#include "cstr.h"
#include "refcntr.h"
#include "rcldoc.h"
#include "stoplist.h"
#include "rclconfig.h"
#include "utf8iter.h"
#include "textsplit.h"

using std::string;
using std::vector;

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
//
// Unique Document Identifier: uniquely identifies a document in its
// source storage (file system or other). Used for up to date checks
// etc. "udi". Our user is responsible for making sure it's not too
// big, cause it's stored as a Xapian term (< 150 bytes would be
// reasonable)

class RclConfig;

namespace Rcl {

// Omega compatible values. We leave a hole for future omega values. Not sure 
// it makes any sense to keep any level of omega compat given that the index
// is incompatible anyway.
enum value_slot {
    // Omega-compatible values:
    VALUE_LASTMOD = 0,	// 4 byte big endian value - seconds since 1970.
    VALUE_MD5 = 1,	// 16 byte MD5 checksum of original document.
    VALUE_SIZE = 2,     // sortable_serialise(<file size in bytes>)

    // Recoll only:
    VALUE_SIG = 10      // Doc sig as chosen by app (ex: mtime+size
};

class SearchData;
class TermIter;
class Query;

/** Used for returning result lists for index terms matching some criteria */
class TermMatchEntry {
public:
    TermMatchEntry() 
	: wcf(0) 
    {
    }
    TermMatchEntry(const string& t, int f, int d)
	: term(t), wcf(f), docs(d) 
    {
    }
    TermMatchEntry(const string& t)
    : term(t), wcf(0) 
    {
    }
    bool operator==(const TermMatchEntry &o) const 
    { 
	return term == o.term;
    }
    bool operator<(const TermMatchEntry &o) const 
    { 
	return term < o.term;
    }

    string term;
    int    wcf; // Total count of occurrences within collection.
    int    docs; // Number of documents countaining term.
};

/** Term match result list header: statistics and global info */
class TermMatchResult {
public:
    TermMatchResult() 
    {
	clear();
    }
    void clear() 
    {
	entries.clear(); 
	dbdoccount = 0; 
	dbavgdoclen = 0;
    }
    // Term expansion
    vector<TermMatchEntry> entries;
    // If a field was specified, this is the corresponding index prefix
    string prefix;
    // Index-wide stats
    unsigned int dbdoccount;
    double       dbavgdoclen;
};

inline bool has_prefix(const string& trm)
{
#ifndef RCL_INDEX_STRIPCHARS
    if (o_index_stripchars) {
#endif
	return !trm.empty() && 'A' <= trm[0] && trm[0] <= 'Z';
#ifndef RCL_INDEX_STRIPCHARS
    } else {
	return !trm.empty() && trm[0] == ':';
    }
#endif
}

inline string strip_prefix(const string& trm)
{
    if (trm.empty())
	return trm;
    string::size_type st = 0;
#ifndef RCL_INDEX_STRIPCHARS
    if (o_index_stripchars) {
#endif
	st = trm.find_first_not_of("ABCDEFIJKLMNOPQRSTUVWXYZ");
	if (st == string::npos)
	    return string();
#ifndef RCL_INDEX_STRIPCHARS
    } else {
	if (has_prefix(trm)) {
	    st = trm.find_last_of(":") + 1;
	} else {
	    return trm;
	}
    }
#endif
    return trm.substr(st);
}

inline string wrap_prefix(const string& pfx) 
{
#ifndef RCL_INDEX_STRIPCHARS
    if (o_index_stripchars) {
#endif
	return pfx;
#ifndef RCL_INDEX_STRIPCHARS
    } else {
	return cstr_colon + pfx + cstr_colon;
    }
#endif
}

/**
 * Wrapper class for the native database.
 */
class Db {
 public:
    // A place for things we don't want visible here.
    class Native;
    friend class Native;

    /* General stuff (valid for query or update) ****************************/
    Db(const RclConfig *cfp);
    ~Db();

    enum OpenMode {DbRO, DbUpd, DbTrunc};
    enum OpenError {DbOpenNoError, DbOpenMainDb, DbOpenExtraDb};
    bool open(OpenMode mode, OpenError *error = 0);
    bool close();
    bool isopen();

    /** Get explanation about last error */
    string getReason() const {return m_reason;}

    /** Return all possible stemmer names */
    static vector<string> getStemmerNames();

    /** Return existing stemming databases */
    vector<string> getStemLangs();

    /** Test word for spelling correction candidate: not too long, no 
	special chars... */
    static bool isSpellingCandidate(const string& term)
    {
	if (term.empty() || term.length() > 50)
	    return false;
	if (has_prefix(term))
	    return false;
	Utf8Iter u8i(term);
	if (TextSplit::isCJK(*u8i)) 
	    return false;
	if (term.find_first_of(" !\"#$%&()*+,-./0123456789:;<=>?@[\\]^_`{|}~") 
	    != string::npos)
	    return false;
	return true;
    }


#ifdef TESTING_XAPIAN_SPELL
    /** Return spelling suggestion */
    string getSpellingSuggestion(const string& word);
#endif

    /* The next two, only for searchdata, should be somehow hidden */
    /* Return configured stop words */
    const StopList& getStopList() const {return m_stops;}
    /* Field name to prefix translation (ie: author -> 'A') */
    bool fieldToTraits(const string& fldname, const FieldTraits **ftpp);

    /* Update-related methods ******************************************/

    /** Test if the db entry for the given udi is up to date (by
     * comparing the input and stored sigs). 
     * Side-effect: set the existence flag for the file document 
     * and all subdocs if any (for later use by 'purge()') 
     */
    bool needUpdate(const string &udi, const string& sig);

    /** Add or update document identified by unique identifier.
     * @param config Config object to use. Can be the same as the member config
     *   or a clone, to avoid sharing when called in multithread context.
     * @param udi the Unique Document Identifier is opaque to us. 
     *   Maximum size 150 bytes.
     * @param parent_udi the UDI for the container document. In case of complex
     *  embedding, this is not always the immediate parent but the UDI for
     *  the container file (which may be a farther ancestor). It is
     *  used for purging subdocuments when a file ceases to exist and
     *  to set the existence flags of all subdocuments of a container
     *  that is found to be up to date. In other words, the
     *  parent_udi is the UDI for the ancestor of the document which
     *  is subject to needUpdate() and physical existence tests (some
     *  kind of file equivalent). Empty for top-level docs. Should
     *  probably be renamed container_udi.
     * @param doc container for document data. Should have been filled as 
     *   much as possible depending on the document type. 
     */
    bool addOrUpdate(const string &udi, 
		     const string &parent_udi, Doc &doc);
#ifdef IDX_THREADS
    void waitUpdIdle();
#endif

    /** Delete document(s) for given UDI, including subdocs */
    bool purgeFile(const string &udi, bool *existed = 0);

    /** Remove documents that no longer exist in the file system. This
     * depends on the update map, which is built during
     * indexing (needUpdate() / addOrUpdate()). 
     *
     * This should only be called after a full walk of
     * the file system, else the update map will not be complete, and
     * many documents will be deleted that shouldn't, which is why this
     * has to be called externally, rcldb can't know if the indexing
     * pass was complete or partial.
     */
    bool purge();

    /** Create stem expansion database for given languages. */
    bool createStemDbs(const std::vector<std::string> &langs);
    /** Delete stem expansion database for given language. */
    bool deleteStemDb(const string &lang);

    /* Query-related methods ************************************/

    /** Return total docs in db */
    int  docCnt(); 
    /** Return count of docs which have an occurrence of term */
    int termDocCnt(const string& term);
    /** Add extra Xapian database for querying. 
     * @param dir must point to something which can be passed as parameter 
     *      to a Xapian::Database constructor (directory or stub).
     */
    bool addQueryDb(const string &dir);
    /** Remove extra database. if dir == "", remove all. */
    bool rmQueryDb(const string &dir);
    /** Look where the doc result comes from. 
     * @param doc must come from a db query so that "opaque" xdocid is set.
     * @return: 0 main index, (size_t)-1 don't know, 
     *   other: order of database in add_database() sequence.
     */
    size_t whatDbIdx(const Doc& doc);
    /** Tell if directory seems to hold xapian db */
    static bool testDbDir(const string &dir);

    /** Return the index terms that match the input string
     * Expansion is performed either with either wildcard or regexp processing
     * Stem expansion is performed if lang is not empty 
     * 
     * @param typ defines the kind of expansion: wildcard, regexp or stemming
     * @param lang sets the stemming language(s). Can be a space-separated list
     * @param term is the term to expand
     * @param result is the main output
     * @param max defines the maximum result count
     * @param field if set, defines the field within with the expansion should
     *        be performed. Only used for wildcards and regexps, stemming is
     *        always global. If this is set, the resulting output terms 
     *        will be appropriately prefix and the prefix value will be set 
     *        in the TermMatchResult header
     */
    enum MatchType {ET_WILD, ET_REGEXP, ET_STEM};
    bool termMatch(MatchType typ, const string &lang, const string &term, 
		   TermMatchResult& result, int max = -1, 
		   const string& field = cstr_null);
    /** Return min and max years for doc mod times in db */
    bool maxYearSpan(int *minyear, int *maxyear);

    /** Wildcard expansion specific to file names. Internal/sdata use only */
    bool filenameWildExp(const string& exp, vector<string>& names, int max);

    /** Set parameters for synthetic abstract generation */
    void setAbstractParams(int idxTrunc, int synthLen, int syntCtxLen);
    int getAbsCtxLen() const 
    {
	return m_synthAbsWordCtxLen;
    }
    int getAbsLen() const
    {
	return m_synthAbsLen;
    }
    /** Get document for given udi
     *
     * Used by the 'history' feature, and to retrieve ancestor documents.
     */
    bool getDoc(const string &udi, Doc &doc);

    /* The following are mainly for the aspell module */
    /** Whole term list walking. */
    TermIter *termWalkOpen();
    bool termWalkNext(TermIter *, string &term);
    void termWalkClose(TermIter *);
    /** Test term existence */
    bool termExists(const string& term);
    /** Test if terms stem to different roots. */
    bool stemDiffers(const string& lang, const string& term, 
		     const string& base);

    const RclConfig *getConf() {return m_config;}

    /** 
	Activate the "in place reset" mode where all documents are
	considered as needing update. This is a global/per-process
	option, and can't be reset. It should be set at the start of
	the indexing pass. 2012-10: no idea why this is done this way...
    */
    static void setInPlaceReset() {o_inPlaceReset = true;}

    /* This has to be public for access by embedded Query::Native */
    Native *m_ndb; 
    bool purgeFileWrite(const string& udi, const string& uniterm);
private:
    const RclConfig *m_config;
    string     m_reason; // Error explanation

    // Xapian directories for additional databases to query
    vector<string> m_extraDbs;
    OpenMode m_mode;
    // File existence vector: this is filled during the indexing pass. Any
    // document whose bit is not set at the end is purged
    vector<bool> updated;
    // Stop terms: those don't get indexed.
    StopList m_stops;
    // Text bytes indexed since beginning
    long long    m_curtxtsz;
    // Text bytes at last flush
    long long    m_flushtxtsz;
    // Text bytes at last fsoccup check
    long long    m_occtxtsz;
    // First fs occup check ?
    int         m_occFirstCheck;

    /***************
     * Parameters cached out of the configuration files. Logically const 
     * after init */
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
    int          m_flushMb;
    // Maximum file system occupation percentage
    int          m_maxFsOccupPc;
    // Database directory
    string       m_basedir;
    // When this is set, all documents are considered as needing a reindex.
    // This implements an alternative to just erasing the index before 
    // beginning, with the advantage that, for small index formats updates, 
    // between releases the index remains available while being recreated.
    static bool o_inPlaceReset;
    /******* End logical constnesss */

    // Internal form of close, can be called during destruction
    bool i_close(bool final);
    // Reinitialize when adding/removing additional dbs
    bool adjustdbs(); 
    bool stemExpand(const string &lang, const string &s, 
		    TermMatchResult& result);

    // Flush when idxflushmb is reached
    bool maybeflush(off_t moretext);
    bool docExists(const string& uniterm);

    /* Copyconst and assignement private and forbidden */
    Db(const Db &) {}
    Db& operator=(const Db &) {return *this;};
};

// This has to go somewhere, and as it needs the Xapian version, this is
// the most reasonable place.
string version_string();

extern const string pathelt_prefix;
extern const string udi_prefix;
extern const string parent_prefix;
extern const string mimetype_prefix;
#ifdef RCL_INDEX_STRIPCHARS
extern const string start_of_field_term;
extern const string end_of_field_term;
#else
extern string start_of_field_term;
extern string end_of_field_term;
#endif
}

#endif /* _DB_H_INCLUDED_ */
