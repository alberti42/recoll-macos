#ifndef _DB_H_INCLUDED_
#define _DB_H_INCLUDED_
/* @(#$Id: rcldb.h,v 1.6 2005-01-28 15:25:40 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

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
    std::string url;
    std::string mimetype;
    std::string mtime;       // Modification time as decimal ascii
    std::string origcharset;
    std::string title;
    std::string keywords;
    std::string abstract;

    std::string text;
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
    bool open(const std::string &dbdir, OpenMode mode);
    bool close();

    // Update-related functions
    bool add(const std::string &filename, const Doc &doc);
    bool needUpdate(const std::string &filename, const struct stat *stp);

    // Query-related functions

    // Parse query string and initialize query
    bool setQuery(const std::string &q);

    // Get document at rank i. This is probably vastly inferior to the type
    // of interface in Xapian, but we have to start with something simple
    // to experiment with the GUI
    bool getDoc(int i, Doc &doc, int *percent = 0);
    // Get results count
    int getResCnt();
};


}

#endif /* _DB_H_INCLUDED_ */
