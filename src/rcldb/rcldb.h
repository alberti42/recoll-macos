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
/* @(#$Id: rcldb.h,v 1.31 2006-04-06 13:08:28 dockes Exp $  (C) 2004 J.F.Dockes */

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
    
    string text; // text is split and indexed 

    int pc; // used by sortseq, convenience

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
    }
};

/**
 * Holder for the advanced query data 
 */
class AdvSearchData {
    public:
    string allwords;
    string phrase;
    string orwords;
    string nowords;
    string filename; 
    list<string> filetypes; // restrict to types. Empty if inactive
    string topdir; // restrict to subtree. Empty if inactive
    string description; // Printable expanded version of the complete query
                        // returned after setQuery.
    void erase() {
	allwords.erase();
	phrase.erase();
	orwords.erase();
	nowords.erase();
	filetypes.clear(); 
	topdir.erase();
	filename.erase();
	description.erase();
    }
};

class Native;
 
/**
 * Wrapper class for the native database.
 */
class Db {
 public:
    Db();
    ~Db();

    enum OpenMode {DbRO, DbUpd, DbTrunc};
    enum QueryOpts {QO_NONE=0, QO_STEM = 1, QO_BUILD_ABSTRACT = 2,
		    QO_REPLACE_ABSTRACT = 4};

    bool open(const string &dbdir, OpenMode mode, int qops = 0);
    bool close();
    bool isopen();

    // Update-related functions
    bool add(const string &filename, const Doc &doc, const struct stat *stp);
    bool needUpdate(const string &filename, const struct stat *stp);
    bool purge();
    bool createStemDb(const string &lang);
    bool deleteStemDb(const string &lang);

    // Query-related functions

    // Parse query string and initialize query
    bool setQuery(const string &q, QueryOpts opts = QO_NONE, 
		  const string& stemlang = "english");
    bool setQuery(AdvSearchData &q, QueryOpts opts = QO_NONE,
		  const string& stemlang = "english");
    bool getQueryTerms(list<string>& terms);

    /// Add extra database for querying
    bool addQueryDb(const string &dir);
    /// Remove extra database. if dir == "", remove all.
    bool rmQueryDb(const string &dir);
    /// Tell if directory seems to hold xapian db
    static bool testDbDir(const string &dir);

    /** Get document at rank i in current query. 

	This is probably vastly inferior to the type of interface in
	Xapian, but we have to start with something simple to
	experiment with the GUI. i is sequential from 0 to some value.
    */
    bool getDoc(int i, Doc &doc, int *percent = 0);

    /** Get document for given filename and ipath */
    bool getDoc(const string &fn, const string &ipath, Doc &doc, int *percent);

    /** Get results count for current query */
    int getResCnt();

    /** Get a list of existing stemming databases */
    std::list<std::string> getStemLangs();

    string getDbDir();
private:

    AdvSearchData m_asdata;
    vector<int> m_dbindices; // In case there is a postq filter: sequence of 
                             // db indices that match

    // Things we don't want to have here.
    friend class Native;
    Native *m_ndb; // Pointer to private data. We don't want db(ie
                 // xapian)-specific defs to show in here

    unsigned int m_qOpts;

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
