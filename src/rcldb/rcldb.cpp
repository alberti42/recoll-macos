#ifndef lint
static char rcsid[] = "@(#$Id: rcldb.cpp,v 1.9 2005-01-26 13:03:02 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <sys/stat.h>

#include <iostream>
#include <string>
#include <vector>

using namespace std;

#include "rcldb.h"
#include "textsplit.h"
#include "transcode.h"
#include "unacpp.h"
#include "conftree.h"
#include "debuglog.h"

#include "xapian.h"

// Data for a xapian database. There could actually be 2 different
// ones for indexing or query as there is not much in common.
class Native {
 public:
    bool isopen;
    bool iswritable;
    // Indexing
    Xapian::WritableDatabase wdb;
    vector<bool> updated;

    // Querying
    Xapian::Database db;
    Xapian::Query query;
    Native() : isopen(false), iswritable(false) {}

};

Rcl::Db::Db() 
{
    pdata = new Native;
}

Rcl::Db::~Db()
{
    LOGDEB(("Rcl::Db::~Db\n"));
    if (pdata == 0)
	return;
    Native *ndb = (Native *)pdata;
    LOGDEB(("Db::~Db: isopen %d iswritable %d\n", ndb->isopen, 
	    ndb->iswritable));
    if (ndb->isopen == false)
	return;
    try {
	LOGDEB(("Rcl::Db::~Db: deleting native database\n"));
	if (ndb->iswritable == true)
	    ndb->wdb.flush();
	delete ndb;
	return;
    } catch (const Xapian::Error &e) {
	cerr << "Exception: " << e.get_msg() << endl;
    } catch (const string &s) {
	cerr << "Exception: " << s << endl;
    } catch (const char *s) {
	cerr << "Exception: " << s << endl;
    } catch (...) {
	cerr << "Caught unknown exception" << endl;
    }
    LOGERR(("Rcl::Db::~Db: got exception\n"));
}

bool Rcl::Db::open(const string& dir, OpenMode mode)
{
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;
    LOGDEB(("Db::open: isopen %d iswritable %d\n", ndb->isopen, 
	    ndb->iswritable));

    if (ndb->isopen) {
	LOGERR(("Rcl::Db::open: already open\n"));
	return false;
    }

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
	    ndb->db = Xapian::Auto::open(dir, Xapian::DB_OPEN);
	    break;
	}
	ndb->isopen = true;
	return true;
    } catch (const Xapian::Error &e) {
	cerr << "Exception: " << e.get_msg() << endl;
    } catch (const string &s) {
	cerr << "Exception: " << s << endl;
    } catch (const char *s) {
	cerr << "Exception: " << s << endl;
    } catch (...) {
	cerr << "Caught unknown exception" << endl;
    }
    LOGERR(("Rcl::Db::open: got exception\n"));
    return false;
}

// Note: xapian has no close call, we delete and recreate the db
bool Rcl::Db::close()
{
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;
    LOGDEB(("Db::close(): isopen %d iswritable %d\n", ndb->isopen, 
	    ndb->iswritable));
    if (ndb->isopen == false)
	return true;
    try {
	if (ndb->iswritable == true)
	    ndb->wdb.flush();
	delete ndb;
    } catch (const Xapian::Error &e) {
	cerr << "Exception: " << e.get_msg() << endl;
	return false;
    } catch (const string &s) {
	cerr << "Exception: " << s << endl;
	return false;
    } catch (const char *s) {
	cerr << "Exception: " << s << endl;
	return false;
    } catch (...) {
	cerr << "Caught unknown exception" << endl;
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

// Callback for the document to word splitting class during indexation
static bool splitCb(void *cdata, const std::string &term, int pos)
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
	LOGERR(("Error occurred during add_posting\n"));
	return false;
    }
    return true;
}

// Unaccent and lowercase data: use unac 
// for accents, and do it by hand for upper / lower. Note lowercasing is
// only for ascii letters anyway, so it's just A-Z -> a-z
bool dumb_string(const string &in, string &out)
{
    string inter;
    out.erase();
    if (!unac_cpp(in, inter))
	return false;
    out.reserve(inter.length());
    for (unsigned int i = 0; i < inter.length(); i++) {
	if (inter[i] >= 'A' && inter[i] <= 'Z')
	    out += inter[i] + 'a' - 'A';
	else
	    out += inter[i];
    }
    return true;
}

bool Rcl::Db::add(const string &fn, const Rcl::Doc &doc)
{
    LOGDEB(("Rcl::Db::add: fn %s\n", fn.c_str()));
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

    wsData splitData(newdocument);

    TextSplit splitter(splitCb, &splitData);

    string noacc;
    if (!unac_cpp(doc.title, noacc)) {
	LOGERR(("Rcl::Db::add: unac failed\n"));
	return false;
    }
    splitter.text_to_words(noacc);

    splitData.basepos += splitData.curpos + 100;
    if (!dumb_string(doc.text, noacc)) {
	LOGERR(("Rcl::Db::add: dum_string failed\n"));
	return false;
    }
    splitter.text_to_words(noacc);

    splitData.basepos += splitData.curpos + 100;
    if (!dumb_string(doc.keywords, noacc)) {
	LOGERR(("Rcl::Db::add: dum_string failed\n"));
	return false;
    }
    splitter.text_to_words(noacc);

    splitData.basepos += splitData.curpos + 100;
    if (!dumb_string(doc.abstract, noacc)) {
	LOGERR(("Rcl::Db::add: dum_string failed\n"));
	return false;
    }
    splitter.text_to_words(noacc);

    newdocument.add_term("T" + doc.mimetype);
    string pathterm  = "P" + fn;
    newdocument.add_term(pathterm);
    const char *fnc = fn.c_str();
    if (1 /*dupes == DUPE_replace*/) {
	// If this document has already been indexed, update the existing
	// entry.
	try {
#if 0
	    Xapian::docid did = 
#endif
		ndb->wdb.replace_document(pathterm, newdocument);
#if 0
	    if (did < updated.size()) {
		updated[did] = true;
		LOGDEB(("%s updated\n", fnc));
	    } else {
		LOGDEB(("%s added\n", fnc));
	    }
#endif
	} catch (...) {
	    // FIXME: is this ever actually needed?
	    ndb->wdb.add_document(newdocument);
	    LOGDEB(("%s added (failed re-seek for duplicate).\n", fnc));
	}
    } else {
	try {
	    ndb->wdb.add_document(newdocument);
	    LOGDEB(("%s added\n", fnc));
	} catch (...) {
	    LOGERR(("%s : Got exception while adding doc\n", fnc));
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
	//cerr << "DOCUMENT EXISTS " << data << endl;
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

#include <vector>

class wsQData {
 public:
    vector<string> terms;
};

// Callback for the query-to-words splitting
static bool splitQCb(void *cdata, const std::string &term, int )
{
    wsQData *data = (wsQData*)cdata;
    data->terms.push_back(term);
    return true;
}

bool Rcl::Db::setQuery(const std::string &querystring)
{
    wsQData splitData;
    TextSplit splitter(splitQCb, &splitData);

    string noacc;
    if (!dumb_string(querystring, noacc)) {
	return false;
    }
    splitter.text_to_words(noacc);

    Native *ndb = (Native *)pdata;

    ndb->query = Xapian::Query(Xapian::Query::OP_OR, splitData.terms.begin(), 
			       splitData.terms.end());

    return true;
}

bool Rcl::Db::getDoc(int i, Doc &doc)
{
    LOGDEB1(("Rcl::Db::getDoc: %d\n", i));
    Native *ndb = (Native *)pdata;

    Xapian::Enquire enquire(ndb->db);
    enquire.set_query(ndb->query);
    Xapian::MSet matches = enquire.get_mset(i, 1);

    LOGDEB1(("Rcl::Db::getDoc: Query '%s' Estimated results: %d\n",
	     ndb->query.get_description(), matches.get_matches_lower_bound()));

    if (matches.empty())
	return false;

    Xapian::Document xdoc = matches.begin().get_document();

    // Parse xapian document's data and populate doc fields
    string data = xdoc.get_data();
    ConfSimple parms(&data);
    parms.get(string("mtype"), doc.mimetype);
    parms.get(string("mtime"), doc.mtime);
    parms.get(string("url"), doc.url);
    return true;
}
