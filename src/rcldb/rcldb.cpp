#ifndef lint
static char rcsid[] = "@(#$Id: rcldb.cpp,v 1.22 2005-02-08 11:59:08 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
#include <stdio.h>
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
    LOGDEB1(("Rcl::Db::~Db\n"));
    if (pdata == 0)
	return;
    Native *ndb = (Native *)pdata;
    LOGDEB(("Db::~Db: isopen %d iswritable %d\n", ndb->isopen, 
	    ndb->iswritable));
    if (ndb->isopen == false)
	return;
    string ermsg;
    try {
	LOGDEB(("Rcl::Db::~Db: deleting native database\n"));
	if (ndb->iswritable == true)
	    ndb->wdb.flush();
	delete ndb;
	return;
    } catch (const Xapian::Error &e) {
	ermsg = e.get_msg();
    } catch (const string &s) {
	ermsg = s;
    } catch (const char *s) {
	ermsg = s;
    } catch (...) {
	ermsg = "Caught unknown exception";
    }
    LOGERR(("Rcl::Db::~Db: got exception: %s\n", ermsg.c_str()));
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
	    LOGDEB(("Rcl::Db::open: lastdocid: %d\n", 
		    ndb->wdb.get_lastdocid()));
	    ndb->updated.resize(ndb->wdb.get_lastdocid() + 1);
	    for (unsigned int i = 0; i < ndb->updated.size(); i++)
		ndb->updated[i] = false;
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
    string ermsg;
    try {
	if (ndb->iswritable == true) {
	    ndb->wdb.flush();
	    LOGDEB(("Rcl:Db: Called xapian flush\n"));
	}
	delete ndb;
	pdata = new Native;
	if (pdata)
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
    LOGERR(("Rcl::Db:close: exception while deleting db: %s\n", 
	    ermsg.c_str()));
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
class mySplitterCB : public TextSplitCB {
 public:
    Xapian::Document &doc;
    Xapian::termpos basepos; // Base for document section
    Xapian::termpos curpos;  // Last position sent to callback
    mySplitterCB(Xapian::Document &d) : doc(d), basepos(1), curpos(0)
    {}
    bool takeword(const std::string &term, int pos, int, int);
};

// Callback for the document to word splitting class during indexation
bool mySplitterCB::takeword(const std::string &term, int pos, int, int)
{
    // cerr << "splitCb: term " << term << endl;
    //string printable;
    //transcode(term, printable, "UTF-8", "ISO8859-1");
    //cerr << "Adding " << printable << endl;

    try {
	// 1 is the value for wdfinc in index_text when called from omindex
	// TOBEDONE: check what this is used for
	curpos = pos;
	doc.add_posting(term, basepos + curpos, 1);
    } catch (...) {
	LOGERR(("Rcl::Db: Error occurred during xapian add_posting\n"));
	return false;
    }
    return true;
}

// Unaccent and lowercase data: use unac 
// for accents, and do it by hand for upper / lower. Note lowercasing is
// only for ascii letters anyway, so it's just A-Z -> a-z
// Removing crlfs is so that we can use the text in the document data fields.
bool Rcl::dumb_string(const string &in, string &out)
{
    string inter;
    out.erase();
    if (in.empty())
	return true;
    if (!unac_cpp(in, inter)) {
	LOGERR(("dumb_string: unac_cpp failed for %s\n", in.c_str()));
	return false;
    }
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

/* omindex direct */
/* Truncate a string to a given maxlength, avoiding cutting off midword
 * if reasonably possible. */
string
truncate_to_word(string & input, string::size_type maxlen)
{
    string output;
    if (input.length() <= maxlen) {
	output = input;
    } else {
	output = input.substr(0, maxlen);
	const char *SEPAR = " \t\n\r-:.;,/[]{}";
	string::size_type space = output.find_last_of(SEPAR);
	// Original version only truncated at space if space was found after
	// maxlen/2. But we HAVE to truncate at space, else we'd need to do
	// utf8 stuff to avoid truncating at multibyte char. In any case,
	// not finding space means that the text probably has no value.
	// Except probably for Asian languages, so we may want to fix this 
	// one day
	if (space == string::npos) {
	    output.erase();
	} else {
	    output.erase(space);
	}

	output += " ...";
    }

    // replace newlines with spaces
    size_t i = 0;    
    while ((i = output.find('\n', i)) != string::npos) output[i] = ' ';
    return output;
}

bool Rcl::Db::add(const string &fn, const Rcl::Doc &idoc)
{
    LOGDEB(("Rcl::Db::add: fn %s\n", fn.c_str()));
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;

    Rcl::Doc doc = idoc;
    if (doc.abstract.empty()) 
	doc.abstract = truncate_to_word(doc.text, 100);
    else 
	doc.abstract = truncate_to_word(doc.abstract, 100);
    doc.title = truncate_to_word(doc.title, 100);
    doc.keywords = truncate_to_word(doc.keywords, 300);

    Xapian::Document newdocument;

    mySplitterCB splitData(newdocument);

    TextSplit splitter(&splitData);

    string noacc;
    if (!unac_cpp(doc.title, noacc)) {
	LOGERR(("Rcl::Db::add: unac failed\n"));
	return false;
    }
    splitter.text_to_words(noacc);

    splitData.basepos += splitData.curpos + 100;
    if (!dumb_string(doc.text, noacc)) {
	LOGERR(("Rcl::Db::add: dumb_string failed\n"));
	return false;
    }
    splitter.text_to_words(noacc);

    splitData.basepos += splitData.curpos + 100;
    if (!dumb_string(doc.keywords, noacc)) {
	LOGERR(("Rcl::Db::add: dumb_string failed\n"));
	return false;
    }
    splitter.text_to_words(noacc);

    splitData.basepos += splitData.curpos + 100;
    if (!dumb_string(doc.abstract, noacc)) {
	LOGERR(("Rcl::Db::add: dumb_string failed\n"));
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
    LOGDEB1(("Newdocument data: %s\n", record.c_str()));
    newdocument.set_data(record);


    time_t mtime = atol(doc.mtime.c_str());
    struct tm *tm = localtime(&mtime);
    char buf[9];
    sprintf(buf, "%04d%02d%02d",tm->tm_year+1900, tm->tm_mon + 1, tm->tm_mday);
    newdocument.add_term("D" + string(buf)); // Date (YYYYMMDD)
    buf[7] = '\0';
    if (buf[6] == '3') buf[6] = '2';
    newdocument.add_term("W" + string(buf)); // "Weak" - 10ish day interval
    buf[6] = '\0';
    newdocument.add_term("M" + string(buf)); // Month (YYYYMM)
    buf[4] = '\0';
    newdocument.add_term("Y" + string(buf)); // Year (YYYY)

    // If this document has already been indexed, update the existing
    // entry.
    try {
	Xapian::docid did = 
	    ndb->wdb.replace_document(pathterm, newdocument);
	if (did < ndb->updated.size()) {
	    ndb->updated[did] = true;
	    LOGDEB(("Rcl::Db::add: docid %d updated [%s]\n", did, fnc));
	} else {
	    LOGDEB(("Rcl::Db::add: docid %d added [%s]\n", did, fnc));
	}
    } catch (...) {
	// FIXME: is this ever actually needed?
	ndb->wdb.add_document(newdocument);
	LOGDEB(("Rcl::Db::add: %s added (failed re-seek for duplicate)\n", 
		fnc));
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
	const char *cp = strstr(data.c_str(), "mtime=");
	cp += 6;
	long mtime = atol(cp);
	if (mtime >= stp->st_mtime) {
	    if (*did < ndb->updated.size())
		ndb->updated[*did] = true;
	    return false;
	} 
    } catch (...) {
	return true;
    }

    return true;
}

bool Rcl::Db::purge()
{
    LOGDEB(("Rcl::Db::purge\n"));
    // There seems to be problems with the document delete code, when
    // we do this, the database is not actually updated. Especially,
    // if we delete a bunch of docs, so that there is a hole in the
    // docids at the beginning, we can't add anything (appears to work
    // and does nothing). Maybe related to the exceptions below when
    // trying to delete an unexistant document ?
    // Flushing before trying the deletes seeems to work around the problem

    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;
    LOGDEB(("Rcl::Db::purge: isopen %d iswritable %d\n", ndb->isopen, 
	    ndb->iswritable));
    if (ndb->isopen == false || ndb->iswritable == false)
	return false;

    ndb->wdb.flush();
    for (Xapian::docid did = 1; did < ndb->updated.size(); ++did) {
	if (!ndb->updated[did]) {
	    try {
		ndb->wdb.delete_document(did);
		LOGDEB(("Rcl::Db::purge: deleted document #%d\n", did));
	    } catch (const Xapian::DocNotFoundError &) {
		LOGDEB(("Rcl::Db::purge: document #%d not found\n", did));
	    }
	}
    }
    return true;
}


#include <vector>

class wsQData : public TextSplitCB {
 public:
    vector<string> terms;
    string catterms() {
	string s;
	for (unsigned int i=0;i<terms.size();i++) {
	    s += "[" + terms[i] + "] ";
	}
	return s;
    }
    bool takeword(const std::string &term, int , int, int) {
	LOGDEB(("Takeword: %s\n", term.c_str()));
	terms.push_back(term);
	return true;
    }
};


bool Rcl::Db::setQuery(const std::string &iqstring)
{
    LOGDEB(("Rcl::Db::setQuery: %s\n", iqstring.c_str()));
    Native *ndb = (Native *)pdata;
    if (!ndb)
	return false;

    string qstring;;
    if (!dumb_string(iqstring, qstring)) {
	return false;
    }

    // First extract phrases:
    list<string> phrases;
    ConfTree::stringToStrings(qstring, phrases);
    for (list<string>::const_iterator i=phrases.begin();
	 i != phrases.end();i++) {
	LOGDEB(("Rcl::Db::setQuery: phrase: '%s'\n", i->c_str()));
    }
    list<Xapian::Query> pqueries;
    for (list<string>::const_iterator it = phrases.begin(); 
	 it != phrases.end(); it++) {

	wsQData splitData;
	TextSplit splitter(&splitData, true);
	splitter.text_to_words(*it);
	LOGDEB(("Splitter term count: %d\n", splitData.terms.size()));
	switch(splitData.terms.size()) {
	case 0: continue;// ??
	case 1:
	    pqueries.push_back(Xapian::Query(splitData.terms.front()));
	    break;
	default:
	    LOGDEB(("Pushing phrase: %s\n", splitData.catterms().c_str()));
	    pqueries.push_back(Xapian::Query(Xapian::Query::OP_PHRASE,
					     splitData.terms.begin(),
					     splitData.terms.end()));
	}
    }
    ndb->query = Xapian::Query(Xapian::Query::OP_OR, pqueries.begin(), 
			       pqueries.end());
    delete ndb->enquire;
    ndb->enquire = new Xapian::Enquire(ndb->db);
    ndb->enquire->set_query(ndb->query);
    ndb->mset = Xapian::MSet();
    return true;
}

bool Rcl::Db::getQueryTerms(list<string>& terms)
{
    Native *ndb = (Native *)pdata;
    if (!ndb)
	return false;

    terms.clear();
    Xapian::TermIterator it;
    for (it = ndb->query.get_terms_begin(); it != ndb->query.get_terms_end();
	 it++) {
	terms.push_back(*it);
    }
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
