#ifndef _DB_H_INCLUDED_
#define _DB_H_INCLUDED_
/* @(#$Id: rcldb.h,v 1.11 2005-02-08 14:45:54 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

#ifndef NO_NAMESPACES
using std::string;
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

namespace Rcl {

/**
 * Dumb bunch holder for document attributes and data
 */
class Doc {
 public:
    // This fields potentially go into the document data record
    string url;
    string mimetype;
    string mtime;       // Modification time as decimal ascii
    string origcharset;
    string title;
    string keywords;
    string abstract;

    string text;
    void erase() {
	url.erase();
	mimetype.erase();
	mtime.erase();
	origcharset.erase();
	title.erase();
	keywords.erase();
	abstract.erase();

	text.erase();
    }
};

/**
 * Wrapper class for the native database.
 */
class Db {
    void *pdata;
    Doc curdoc;
 public:
    Db();
    ~Db();
    enum OpenMode {DbRO, DbUpd, DbTrunc};
    bool open(const string &dbdir, OpenMode mode);
    bool close();
    bool isopen();

    // Update-related functions
    bool add(const string &filename, const Doc &doc);
    bool needUpdate(const string &filename, const struct stat *stp);
    bool purge();

    // Query-related functions

    // Parse query string and initialize query
    enum QueryOpts {QO_NONE=0, QO_STEM = 1};
    bool setQuery(const string &q, QueryOpts opts = QO_NONE, 
		  const string& stemlang = "english");
    bool getQueryTerms(std::list<string>& terms);

    // Get document at rank i. This is probably vastly inferior to the type
    // of interface in Xapian, but we have to start with something simple
    // to experiment with the GUI
    bool getDoc(int i, Doc &doc, int *percent = 0);
    // Get results count
    int getResCnt();
};

// Unaccent and lowercase data.
extern bool dumb_string(const string &in, string &out);

}

#endif /* _DB_H_INCLUDED_ */
