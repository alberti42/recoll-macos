#ifndef lint
static char rcsid[] = "@(#$Id: rcldb.cpp,v 1.14 2005-01-31 14:31:09 dockes Exp $ (C) 2004 J.F.Dockes";
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
    Xapian::Enquire *enquire;
    Xapian::MSet mset;

    Native() : isopen(false), iswritable(false), enquire(0) {
    }
    ~Native() {
	delete enquire;
    }
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
    string ermsg;
    try {
	switch (mode) {
	case DbUpd:
	    ndb->wdb = 
		Xapian::WritableDatabase(dir, Xapian::DB_CREATE_OR_OPEN);
	    ndb->updated.resize(ndb->wdb.get_lastdocid() + 1);
	    ndb->iswritable = true;
	    break;
	case DbTrunc:
	    ndb->wdb = 
		Xapian::WritableDatabase(dir, Xapian::DB_CREATE_OR_OVERWRITE);
	    ndb->iswritable = true;
	    break;
	case DbRO:
	default:
	    ndb->iswritable = false;
	    ndb->db = Xapian::Database(dir);
	    break;
	}
	ndb->isopen = true;
	return true;
    } catch (const Xapian::Error &e) {
	ermsg = e.get_msg();
    } catch (const string &s) {
	ermsg = s;
    } catch (const char *s) {
	ermsg = s;
    } catch (...) {
	ermsg = "Caught unknown exception";
    }
    LOGERR(("Rcl::Db::open: exception while opening '%s': %s\n", 
	    dir.c_str(), ermsg.c_str()));
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

bool Rcl::Db::isopen()
{
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;
    return ndb->isopen;
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
// Removing crlfs is so that we can use the text in the document data fields.
bool dumb_string(const string &in, string &out)
{
    string inter;
    out.erase();
    if (!unac_cpp(in, inter))
	return false;
    out.reserve(inter.length());
    for (unsigned int i = 0; i < inter.length(); i++) {
	if (inter[i] >= 'A' && inter[i] <= 'Z') {
	    out += inter[i] + 'a' - 'A';
	} else {
	    if (inter[i] == '\n' || inter[i] == '\r')
		out += ' ';
	    else
		out += inter[i];
	}
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

    // Document data record. omindex has the following nl separated fields:
    // - url
    // - sample
    // - caption (title limited to 100 chars)
    // - mime type 
    string record = "url=file://" + fn;
    record += "\nmtype=" + doc.mimetype;
    record += "\nmtime=" + doc.mtime;
    record += "\norigcharset=" + doc.origcharset;
    record += "\ncaption=" + doc.title;
    record += "\nkeywords=" + doc.keywords;
    record += "\nabstract=" + doc.abstract;
    record += "\n";
    LOGDEB(("Newdocument data: %s\n", record.c_str()));
    newdocument.set_data(record);

    // If this document has already been indexed, update the existing
    // entry.
    try {
	Xapian::docid did = 
	    ndb->wdb.replace_document(pathterm, newdocument);
	if (did < ndb->updated.size()) {
	    ndb->updated[did] = true;
	    LOGDEB(("%s updated\n", fnc));
	} else {
	    LOGDEB(("%s added\n", fnc));
	}
    } catch (...) {
	// FIXME: is this ever actually needed?
	ndb->wdb.add_document(newdocument);
	LOGDEB(("%s added (failed re-seek for duplicate).\n", fnc));
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
	if (*did < ndb->updated.size())
	    ndb->updated[*did] = true;
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

bool Rcl::Db::purge()
{
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;
    if (ndb->isopen == false || ndb->iswritable == false)
	return false;

    for (Xapian::docid did = 1; did < ndb->updated.size(); ++did) {
	if (!ndb->updated[did]) {
	    try {
		ndb->wdb.delete_document(did);
		LOGDEB(("Rcl::Db::purge: deleted document #%d\n", did));
	    } catch (const Xapian::DocNotFoundError &) {
	    }
	}
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
    LOGDEB(("Rcl::Db::setQuery: %s\n", querystring.c_str()));
    Native *ndb = (Native *)pdata;
    if (!ndb)
	return false;

    wsQData splitData;
    TextSplit splitter(splitQCb, &splitData);

    string noacc;
    if (!dumb_string(querystring, noacc)) {
	return false;
    }
    splitter.text_to_words(noacc);


    ndb->query = Xapian::Query(Xapian::Query::OP_OR, splitData.terms.begin(), 
			       splitData.terms.end());
    delete ndb->enquire;
    ndb->enquire = new Xapian::Enquire(ndb->db);
    ndb->enquire->set_query(ndb->query);
    ndb->mset = Xapian::MSet();
    return true;
}

int Rcl::Db::getResCnt()
{
    Native *ndb = (Native *)pdata;
    if (!ndb || !ndb->enquire) {
	LOGERR(("Rcl::Db::getResCnt: no query opened\n"));
	return -1;
    }
    if (ndb->mset.size() <= 0)
	return -1;
    return ndb->mset.get_matches_lower_bound();
}

bool Rcl::Db::getDoc(int i, Doc &doc, int *percent)
{
    LOGDEB(("Rcl::Db::getDoc: %d\n", i));
    Native *ndb = (Native *)pdata;
    if (!ndb || !ndb->enquire) {
	LOGERR(("Rcl::Db::getDoc: no query opened\n"));
	return false;
    }

    int first = ndb->mset.get_firstitem();
    int last = first + ndb->mset.size() -1;

    if (!(i >= first && i <= last)) {
	LOGDEB1(("Fetching for first %d, count 10\n", i));
	ndb->mset = ndb->enquire->get_mset(i, 10);
	if (ndb->mset.empty())
	    return false;
	first = ndb->mset.get_firstitem();
	last = first + ndb->mset.size() -1;
    }

    LOGDEB1(("Rcl::Db::getDoc: Qry '%s' win [%d-%d] Estimated results: %d",
	     ndb->query.get_description().c_str(), 
	     first, last,
	     ndb->mset.get_matches_lower_bound()));

    Xapian::Document xdoc = ndb->mset[i-first].get_document();
    if (percent)
	*percent = ndb->mset.convert_to_percent(ndb->mset[i-first]);

    // Parse xapian document's data and populate doc fields
    string data = xdoc.get_data();
    LOGDEB1(("Rcl::Db::getDoc: data: %s\n", data.c_str()));
    ConfSimple parms(&data);
    parms.get(string("url"), doc.url);
    parms.get(string("mtype"), doc.mimetype);
    parms.get(string("mtime"), doc.mtime);
    parms.get(string("origcharset"), doc.origcharset);
    parms.get(string("caption"), doc.title);
    parms.get(string("keywords"), doc.keywords);
    parms.get(string("abstract"), doc.abstract);
    return true;
}
