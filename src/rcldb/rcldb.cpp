#ifndef lint
static char rcsid[] = "@(#$Id: rcldb.cpp,v 1.4 2004-12-17 15:36:13 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <sys/stat.h>

#include <iostream>
#include <string>
#include <vector>

using namespace std;

#include "rcldb.h"
#include "textsplit.h"
#include "transcode.h"

#include "xapian.h"

// Data for a xapian database
class Native {
 public:
    bool isopen;
    bool iswritable;
    class Xapian::Database db;
    class Xapian::WritableDatabase wdb;
    vector<bool> updated;

    Native() : isopen(false), iswritable(false) {}

};

Rcl::Db::Db() 
{
    pdata = new Native;
}

Rcl::Db::~Db()
{
    if (pdata == 0)
	return;
    Native *ndb = (Native *)pdata;
    cerr << "Db::~Db: isopen " << ndb->isopen << " iswritable " <<
	ndb->iswritable << endl;
    try {
	// There is nothing to do for an ro db.
	if (ndb->isopen == false || ndb->iswritable == false) {
	    delete ndb;
	    return;
	}
	ndb->wdb.flush();
	delete ndb;
    } catch (const Xapian::Error &e) {
	cout << "Exception: " << e.get_msg() << endl;
    } catch (const string &s) {
	cout << "Exception: " << s << endl;
    } catch (const char *s) {
	cout << "Exception: " << s << endl;
    } catch (...) {
	cout << "Caught unknown exception" << endl;
    }
}

bool Rcl::Db::open(const string& dir, OpenMode mode)
{
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;
    cerr << "Db::open: isopen " << ndb->isopen << " iswritable " <<
	ndb->iswritable << endl;
    try {
	switch (mode) {
	case DbUpd:
	    ndb->wdb = Xapian::Auto::open(dir, Xapian::DB_CREATE_OR_OPEN);
	    ndb->updated.resize(ndb->wdb.get_lastdocid() + 1);
	    ndb->iswritable = true;
	    break;
	case DbTrunc:
	    ndb->wdb = Xapian::Auto::open(dir, Xapian::DB_CREATE_OR_OVERWRITE);
	    ndb->iswritable = true;
	    break;
	case DbRO:
	default:
	    ndb->iswritable = false;
	    cerr << "Not ready to open RO yet" << endl;
	    exit(1);
	}
	ndb->isopen = true;
	return true;
    } catch (const Xapian::Error &e) {
	cout << "Exception: " << e.get_msg() << endl;
    } catch (const string &s) {
	cout << "Exception: " << s << endl;
    } catch (const char *s) {
	cout << "Exception: " << s << endl;
    } catch (...) {
	cout << "Caught unknown exception" << endl;
    }
    return false;
}

bool Rcl::Db::close()
{
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;
    cerr << "Db::open: isopen " << ndb->isopen << " iswritable " <<
	ndb->iswritable << endl;
    if (ndb->isopen == false)
	return true;
    try {
	if (ndb->isopen == true && ndb->iswritable == true) {
	    ndb->wdb.flush();
	}
	delete ndb;
    } catch (const Xapian::Error &e) {
	cout << "Exception: " << e.get_msg() << endl;
	return false;
    } catch (const string &s) {
	cout << "Exception: " << s << endl;
	return false;
    } catch (const char *s) {
	cout << "Exception: " << s << endl;
	return false;
    } catch (...) {
	cout << "Caught unknown exception" << endl;
	return false;
    }
    pdata = new Native;
    if (pdata)
	return true;
    return false;
}

// A small class to hold state while splitting text
class wsData {
 public:
    Xapian::Document &doc;
    Xapian::termpos basepos; // Base for document section
    Xapian::termpos curpos;  // Last position sent to callback
    wsData(Xapian::Document &d) : doc(d), basepos(1), curpos(0)
    {}
};

bool splitCb(void *cdata, const std::string &term, int pos)
{
    wsData *data = (wsData*)cdata;

    // cerr << "splitCb: term " << term << endl;
    //string printable;
    //transcode(term, printable, "UTF-8", "ISO8859-1");
    //cerr << "Adding " << printable << endl;

    try {
	// 1 is the value for wdfinc in index_text when called from omindex
	// TOBEDONE: check what this is used for
	data->curpos = pos;
	data->doc.add_posting(term, data->basepos + data->curpos, 1);
    } catch (...) {
	cerr << "Error occurred during add_posting" << endl;
	return false;
    }
    return true;
}

bool Rcl::Db::add(const string &fn, const Rcl::Doc &doc)
{
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;

    Xapian::Document newdocument;

    // Document data record. omindex has the following nl separated fields:
    // - url
    // - sample
    // - caption (title limited to 100 chars)
    // - mime type 
    string record = "url=file:/" + fn;
    record += "\nmtime=" + doc.mtime;
    record += "\nsample=";
    record += "\ncaption=" + doc.title;
    record += "\nmtype=" + doc.mimetype;
    record += "\n";
    newdocument.set_data(record);

    // TOBEDONE:
    // Need to add stuff here to unaccent and lowercase the data: use unac 
    // for accents, and do it by hand for upper / lower. Note lowercasing is
    // only for ascii letters anyway, so it's just A-Z -> a-z

    wsData splitData(newdocument);

    TextSplit splitter(splitCb, &splitData);

    splitter.text_to_words(doc.title);

    splitData.basepos += splitData.curpos + 100;
    splitter.text_to_words(doc.text);

    splitData.basepos += splitData.curpos + 100;
    splitter.text_to_words(doc.keywords);

    splitData.basepos += splitData.curpos + 100;
    splitter.text_to_words(doc.abstract);

    newdocument.add_term("T" + doc.mimetype);
    string pathterm  = "P" + fn;
    newdocument.add_term(pathterm);

    if (1 /*dupes == DUPE_replace*/) {
	// If this document has already been indexed, update the existing
	// entry.
	try {
	    Xapian::docid did = ndb->wdb.replace_document(pathterm, 
							  newdocument);
#if 0
	    if (did < updated.size()) {
		updated[did] = true;
		//cout << "updated." << endl;
	    } else {
		//cout << "added." << endl;
	    }
#endif
	} catch (...) {
	    // FIXME: is this ever actually needed?
	    ndb->wdb.add_document(newdocument);
	    //cout << "added (failed re-seek for duplicate)." << endl;
	}
    } else {
	try {
	    ndb->wdb.add_document(newdocument);
	    // cout << "added." << endl;
	} catch (...) {
	    cerr << "Got exception while adding doc" << endl;
	    return false;
	}
    }
  return true;
}


bool Rcl::Db::needUpdate(const string &filename, const struct stat *stp)
{
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;

    string pathterm  = "P" + filename;
    if (!ndb->wdb.term_exists(pathterm))
	return true;
    Xapian::PostingIterator doc;
    try {
	Xapian::PostingIterator did = ndb->wdb.postlist_begin(pathterm);
	if (did == ndb->wdb.postlist_end(pathterm))
	    return true;
	Xapian::Document doc = ndb->wdb.get_document(*did);
	string data = doc.get_data();
	//cout << "DOCUMENT EXISTS " << data << endl;
	const char *cp = strstr(data.c_str(), "mtime=");
	cp += 6;
	long mtime = atol(cp);
	if (mtime >= stp->st_mtime) {
	    // cerr << "DOCUMENT UP TO DATE" << endl;
	    return false;
	} 
    } catch (...) {
	return true;
    }

    return true;
}
