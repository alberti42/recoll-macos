#ifndef lint
static char rcsid[] = "@(#$Id: rcldb.cpp,v 1.30 2005-10-19 14:14:17 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
#include <stdio.h>
#include <sys/stat.h>

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

#include "rcldb.h"
#include "textsplit.h"
#include "transcode.h"
#include "unacpp.h"
#include "conftree.h"
#include "debuglog.h"
#include "pathut.h"
#include "smallut.h"

#include "xapian.h"
#include <xapian/stem.h>

// Data for a xapian database. There could actually be 2 different
// ones for indexing or query as there is not much in common.
class Native {
 public:
    bool isopen;
    bool iswritable;
    string basedir;

    // Indexing
    Xapian::WritableDatabase wdb;
    vector<bool> updated;

    // Querying
    Xapian::Database db;
    Xapian::Query    query; // query descriptor: terms and subqueries
			    // joined by operators (or/and etc...)
    Xapian::Enquire *enquire;
    Xapian::MSet     mset;

    Native() : isopen(false), iswritable(false), enquire(0) { }
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
	    break;
	case DbRO:
	default:
	    ndb->iswritable = false;
	    ndb->db = Xapian::Database(dir);
	    break;
	}
	ndb->isopen = true;
	ndb->basedir = dir;
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
// for accents, and do it by hand for upper / lower. 
// TOBEDONE: lowercasing is done only for ascii letters, just A-Z -> a-z 
// Removing crlfs is so that we can use the text in the document data fields.
bool Rcl::dumb_string(const string &in, string &out)
{
    string inter;
    out.erase();
    if (in.empty())
	return true;
    if (!unac_cpp(in, inter)) {
	LOGERR(("dumb_string: unac_cpp failed for %s\n", in.c_str()));
	// Ok, no need to stop the whole show
	inter = "";
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
    LOGDEB1(("Rcl::Db::add: fn %s\n", fn.c_str()));
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
    if (!dumb_string(doc.title, noacc)) {
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

    string uniterm;
    if (!doc.ipath.empty()) {
	uniterm  = "Q" + fn + "|" + doc.ipath;
	newdocument.add_term(uniterm);
    }


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
    if (!doc.ipath.empty()) {
	record += "\nipath=" + doc.ipath;
    }

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
	    ndb->wdb.replace_document(uniterm.empty() ? pathterm : uniterm, 
				      newdocument);
	if (did < ndb->updated.size()) {
	    ndb->updated[did] = true;
	    LOGDEB(("Rcl::Db::add: docid %d updated [%s , %s]\n", did, fnc,
		    doc.ipath.c_str()));
	} else {
	    LOGDEB(("Rcl::Db::add: docid %d added [%s , %s]\n", did, fnc, 
		    doc.ipath.c_str()));
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

    // If no document exist with this path, we do need update
    string pathterm  = "P" + filename;
    if (!ndb->wdb.term_exists(pathterm)) {
	return true;
    }

    // Look for all documents with this path. Check the update time (once). 
    // If the db is up to date, set the update flags for all documents
    Xapian::PostingIterator doc;
    try {
	Xapian::PostingIterator docid0 = ndb->wdb.postlist_begin(pathterm);
	for (Xapian::PostingIterator docid = docid0;
	     docid != ndb->wdb.postlist_end(pathterm); docid++) {

	    Xapian::Document doc = ndb->wdb.get_document(*docid);

	    // Check the date once. no need to look at the others if the
	    // db needs updating.
	    if (docid == docid0) {
		string data = doc.get_data();
		const char *cp = strstr(data.c_str(), "mtime=");
		cp += 6;
		long mtime = atol(cp);
		if (mtime < stp->st_mtime) {
		    // Db is not up to date. Let's index the file
		    return true;
		} 
	    }

	    // Db is up to date. Make a note that this document exists.
	    if (*docid < ndb->updated.size())
		ndb->updated[*docid] = true;
	}
    } catch (...) {
	return true;
    }

    return false;
}

/// Compute name of stem db for given base database and language
static string stemdbname(const string& basename, string lang)
{
    string nm = basename;
    path_cat(nm, string("stem_") + lang);
    return nm;
}

// Is char non-lowercase ascii ?
inline static bool
p_notlowerorutf(unsigned int c)
{
    if (c < 'a' || (c > 'z' && c < 128))
	return true;
    return false;
}

/**
 * Create database of stem to parents associations for a given language.
 * We walk the list of all terms, stem them, and create another Xapian db
 * with documents indexed by a single term (the stem), and with the list of
 * parent terms in the document data.
 */
bool Rcl::Db::createStemDb(const string& lang)
{
    LOGDEB(("Rcl::Db::createStemDb(%s)\n", lang.c_str()));
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;
    if (ndb->isopen == false || ndb->iswritable == false)
	return false;

    // First build the in-memory stem database:
    // We walk the list of all terms, and stem each. 
    //   If the stem is identical to the term, no need to create an entry
    // Else, we add an entry to the multimap.
    // At the end, we only save stem-terms associations with several terms, the
    // others are not useful
    multimap<string, string> assocs;
    // Statistics
    int nostem=0; // Dont even try: not-alphanum (incomplete for now)
    int stemconst=0; // Stem == term
    int stemdiff=0;  // Count of all different stems
    int stemmultiple = 0; // Count of stems with multiple derivatives
    try {
	Xapian::Stem stemmer(lang);
	Xapian::TermIterator it;
	for (it = ndb->wdb.allterms_begin(); 
	     it != ndb->wdb.allterms_end(); it++) {
	    // If it has any non-lowercase 7bit char, cant be stemmable
	    string::iterator sit = (*it).begin(), eit = sit + (*it).length();
	    if ((sit = find_if(sit, eit, p_notlowerorutf)) != eit) {
		++nostem;
		// LOGDEB(("stemskipped: '%s', because of 0x%x\n", 
		// (*it).c_str(), *sit));
		continue;
	    }
	    string stem = stemmer.stem_word(*it);
	    //cerr << "word " << *it << " stem " << stem << endl;
	    if (stem == *it) {
		++stemconst;
		continue;
	    }
	    assocs.insert(pair<string,string>(stem, *it));
	}
    } catch (...) {
	LOGERR(("Stem database build failed: no stemmer for %s ? \n", 
		lang.c_str()));
	return false;
    }

    // Create xapian database for stem relations
    string stemdbdir = stemdbname(ndb->basedir, lang);
    string ermsg = "NOERROR";
    Xapian::WritableDatabase sdb;
    try {
	sdb = Xapian::WritableDatabase(stemdbdir, 
				       Xapian::DB_CREATE_OR_OVERWRITE);
    } catch (const Xapian::Error &e) {
	ermsg = e.get_msg();
    } catch (const string &s) {
	ermsg = s;
    } catch (const char *s) {
	ermsg = s;
    } catch (...) {
	ermsg = "Caught unknown exception";
    }
    if (ermsg != "NOERROR") {
	LOGERR(("Rcl::Db::createstemdb: exception while opening '%s': %s\n", 
		stemdbdir.c_str(), ermsg.c_str()));
	return false;
    }

    // Enter pseud-docs in db. Walk the multimap, only enter
    // associations where there are several parent terms
    string stem;
    list<string> derivs;
    for (multimap<string,string>::const_iterator it = assocs.begin();
	 it != assocs.end(); it++) {
	if (stem == it->first) {
	    // Staying with same stem
	    derivs.push_back(it->second);
	    // cerr << " " << it->second << endl;
	} else {
	    // Changing stems 
	    ++stemdiff;
	    if (derivs.size() > 1) {
		// Previous stem has multiple derivatives. Enter in db
		++stemmultiple;
		Xapian::Document newdocument;
		newdocument.add_term(stem);
		// The doc data is just parents=blank-separated-list
		string record = "parents=";
		for (list<string>::const_iterator it = derivs.begin(); 
		     it != derivs.end(); it++) {
		    record += *it + " ";
		}
		record += "\n";
		LOGDEB1(("stemdocument data: %s\n", record.c_str()));
		newdocument.set_data(record);
		try {
		    sdb.replace_document(stem, newdocument);
		} catch (...) {
		    LOGERR(("Rcl::Db::createstemdb: replace failed\n"));
		    return false;
		}
	    }
	    derivs.clear();
	    stem = it->first;
	    derivs.push_back(it->second);
	    //	    cerr << "\n" << stem << " " << it->second;
	}
    }
    LOGDEB(("Stem map size: %d stems %d mult %d no %d const %d\n", 
	    assocs.size(), stemdiff, stemmultiple, nostem, stemconst));
    return true;
}

/**
 * This is called at the end of an indexing session, to delete the
 *  documents for files that are no longer there. We also build the
 *  stem database while we are at it.
 */
bool Rcl::Db::purge()
{
    LOGDEB(("Rcl::Db::purge\n"));
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;
    LOGDEB(("Rcl::Db::purge: isopen %d iswritable %d\n", ndb->isopen, 
	    ndb->iswritable));
    if (ndb->isopen == false || ndb->iswritable == false)
	return false;

    // There seems to be problems with the document delete code, when
    // we do this, the database is not actually updated. Especially,
    // if we delete a bunch of docs, so that there is a hole in the
    // docids at the beginning, we can't add anything (appears to work
    // and does nothing). Maybe related to the exceptions below when
    // trying to delete an unexistant document ?
    // Flushing before trying the deletes seeems to work around the problem
    ndb->wdb.flush();
    for (Xapian::docid docid = 1; docid < ndb->updated.size(); ++docid) {
	if (!ndb->updated[docid]) {
	    try {
		ndb->wdb.delete_document(docid);
		LOGDEB(("Rcl::Db::purge: deleted document #%d\n", docid));
	    } catch (const Xapian::DocNotFoundError &) {
		LOGDEB2(("Rcl::Db::purge: document #%d not found\n", docid));
	    }
	}
    }
    ndb->wdb.flush();
    return true;
}

/**
 * Expand term to list of all terms which stem to the same term.
 */
static list<string> stemexpand(Native *ndb, string term, const string& lang)
{
    list<string> explist;
    try {
	Xapian::Stem stemmer(lang);
	string stem = stemmer.stem_word(term);
	LOGDEB(("stemexpand: '%s' stem-> '%s'\n", term.c_str(), stem.c_str()));
	// Try to fetch the doc from the stem db
	string stemdbdir = stemdbname(ndb->basedir, lang);
	Xapian::Database sdb(stemdbdir);
	LOGDEB1(("stemexpand: %s lastdocid: %d\n", 
		stemdbdir.c_str(), sdb.get_lastdocid()));
	if (!sdb.term_exists(stem)) {
	    LOGDEB1(("Rcl::Db::stemexpand: no term for %s\n", stem.c_str()));
	    explist.push_back(term);
	    return explist;
	}
	Xapian::PostingIterator did = sdb.postlist_begin(stem);
	if (did == sdb.postlist_end(stem)) {
	    LOGDEB1(("stemexpand: no term(1) for %s\n",stem.c_str()));
	    explist.push_back(term);
	    return explist;
	}
	Xapian::Document doc = sdb.get_document(*did);
	string data = doc.get_data();
	// No need for a conftree, but we need to massage the data a little
	string::size_type pos = data.find_first_of("=");
	++pos;
	string::size_type pos1 = data.find_last_of("\n");
	if (pos == string::npos || pos1 == string::npos ||pos1 <= pos) { // ??
	    explist.push_back(term);
	    return explist;
	}	    
	ConfTree::stringToStrings(data.substr(pos, pos1-pos), explist);
	if (find(explist.begin(), explist.end(), term) == explist.end()) {
	    explist.push_back(term);
	}
	LOGDEB(("stemexpand: %s ->  %s\n", stem.c_str(),
		stringlistdisp(explist).c_str()));
    } catch (...) {
	LOGERR(("stemexpand: error accessing stem db\n"));
	explist.push_back(term);
	return explist;
    }
    return explist;
}


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
	LOGDEB1(("wsQData::takeword: %s\n", term.c_str()));
	terms.push_back(term);
	return true;
    }
    void dumball() {
	for (vector<string>::iterator it=terms.begin(); it !=terms.end();it++){
	    string dumb;
	    Rcl::dumb_string(*it, dumb);
	    *it = dumb;
	}
    }
};


///
// Turn string into possibly complex xapian query. There is little
// interpretation done on the string (no +term -term or filename:term
// stuff). We just separate words and phrases, and interpret
// capitalized terms as wanting no stem expansion
//
static void stringToXapianQueries(const string &iq,
				  const string& stemlang,
				  Native *ndb,
				  list<Xapian::Query> &pqueries,
				  Rcl::Db::QueryOpts opts = Rcl::Db::QO_NONE)
{
    string qstring = iq;
#if 0
    // Unaccent and lowerterm. Note that lowerterming here may not be
    // such a good idea because it forbids using capitalized words to
    // indicate that a term should not use stem expansion, for
    // example.
    if (!Rcl::dumb_string(iqstring, qstring))
	return false;
#endif

    // Split into (possibly single word) phrases ("this is a phrase"):
    list<string> phrases;
    ConfTree::stringToStrings(qstring, phrases);

    // Then process each phrase: split into terms and transform into
    // appropriate Xapian Query

    for (list<string>::iterator it=phrases.begin(); it !=phrases.end(); it++) {
	LOGDEB(("strToXapianQ: phrase or word: [%s]\n", it->c_str()));

	wsQData splitData;
	TextSplit splitter(&splitData, true);
	splitter.text_to_words(*it);
	LOGDEB(("strToXapianQ: splitter term count: %d\n", 
		splitData.terms.size()));
	switch(splitData.terms.size()) {
	case 0: continue;// ??
	case 1: // Not a real phrase: one term
	    {
		string term = splitData.terms.front();
		bool nostemexp = false;
		// Yes this doesnt work with accented or non-european
		// majuscules. TOBEDONE: something :)
		if (term.length() > 0 && term[0] >= 'A' && term[0] <= 'Z')
		    nostemexp = true;

		LOGDEB(("Term: %s\n", term.c_str()));

		// Possibly perform stem compression/expansion
		list<string> exp;  
		string term1;
		Rcl::dumb_string(term, term1);
		if (!nostemexp && (opts & Rcl::Db::QO_STEM)) {
		    exp = stemexpand(ndb, term1, stemlang);
		} else {
		    exp.push_back(term1);
		}

		// Push either term or stem-expanded set
		pqueries.push_back(Xapian::Query(Xapian::Query::OP_OR, 
						 exp.begin(), exp.end()));
	    }
	    break;

	default:
	    // Phrase: no stem expansion
	    splitData.dumball();
	    LOGDEB(("Pushing phrase: [%s]\n", splitData.catterms().c_str()));
	    pqueries.push_back(Xapian::Query(Xapian::Query::OP_PHRASE,
					     splitData.terms.begin(),
					     splitData.terms.end()));
	}
    }
}

// Prepare query out of simple query string 
bool Rcl::Db::setQuery(const std::string &iqstring, QueryOpts opts, 
		       const string& stemlang)
{
    LOGDEB(("Rcl::Db::setQuery: q: '%s', opts 0x%x, stemlang %s\n", 
	    iqstring.c_str(), (unsigned int)opts, stemlang.c_str()));
    Native *ndb = (Native *)pdata;
    if (!ndb)
	return false;

    asdata.erase();
    dbindices.clear();
    list<Xapian::Query> pqueries;
    stringToXapianQueries(iqstring, stemlang, ndb, pqueries, opts);
    ndb->query = Xapian::Query(Xapian::Query::OP_OR, pqueries.begin(), 
			       pqueries.end());
    delete ndb->enquire;
    ndb->enquire = new Xapian::Enquire(ndb->db);
    ndb->enquire->set_query(ndb->query);
    ndb->mset = Xapian::MSet();
    return true;
}

// Prepare query out of "advanced search" data
bool Rcl::Db::setQuery(AdvSearchData &sdata, const string& stemlang)
{
    LOGDEB(("Rcl::Db::setQuery: adv:\n"));
    LOGDEB((" allwords: %s\n", sdata.allwords.c_str()));
    LOGDEB((" phrase:   %s\n", sdata.phrase.c_str()));
    LOGDEB((" orwords:  %s\n", sdata.orwords.c_str()));
    LOGDEB((" nowords:  %s\n", sdata.nowords.c_str()));
    string ft;
    for (list<string>::iterator it = sdata.filetypes.begin(); 
    	 it != sdata.filetypes.end(); it++) {ft += *it + " ";}
    if (!ft.empty()) 
	LOGDEB((" searched file types: %s\n", ft.c_str()));
    if (!sdata.topdir.empty())
	LOGDEB((" restricted to: %s\n", sdata.topdir.c_str()));

    asdata = sdata;
    dbindices.clear();

    Native *ndb = (Native *)pdata;
    if (!ndb)
	return false;
    list<Xapian::Query> pqueries;
    Xapian::Query xq;
    
    if (!sdata.allwords.empty()) {
	stringToXapianQueries(sdata.allwords, stemlang, ndb, pqueries);
	if (!pqueries.empty()) {
	    xq = Xapian::Query(Xapian::Query::OP_AND, pqueries.begin(), 
			       pqueries.end());
	    pqueries.clear();
	}
    }

    if (!sdata.orwords.empty()) {
	stringToXapianQueries(sdata.orwords, stemlang, ndb, pqueries);
	if (!pqueries.empty()) {
	    Xapian::Query nq;
	    nq = Xapian::Query(Xapian::Query::OP_OR, pqueries.begin(),
			       pqueries.end());
	    xq = xq.empty() ? nq :
		Xapian::Query(Xapian::Query::OP_AND, xq, nq);
	    pqueries.clear();
	}
    }

    if (!sdata.nowords.empty()) {
	stringToXapianQueries(sdata.nowords, stemlang, ndb, pqueries);
	if (!pqueries.empty()) {
	    Xapian::Query nq;
	    nq = Xapian::Query(Xapian::Query::OP_OR, pqueries.begin(),
			       pqueries.end());
	    xq = xq.empty() ? nq :
		Xapian::Query(Xapian::Query::OP_AND_NOT, xq, nq);
	    pqueries.clear();
	}
    }

    if (!sdata.phrase.empty()) {
	Xapian::Query nq;
	string s = string("\"") + sdata.phrase + string("\"");
	stringToXapianQueries(s, stemlang, ndb, pqueries);
	if (!pqueries.empty()) {
	    // There should be a single list element phrase query.
	    xq = xq.empty() ? *pqueries.begin() : 
		Xapian::Query(Xapian::Query::OP_AND, xq, *pqueries.begin());
	    pqueries.clear();
	}
    }

    if (!sdata.filetypes.empty()) {
	Xapian::Query tq;
	for (list<string>::iterator it = sdata.filetypes.begin(); 
	     it != sdata.filetypes.end(); it++) {
	    string term = "T" + *it;
	    LOGDEB(("Adding file type term: [%s]\n", term.c_str()));
	    tq = tq.empty() ? Xapian::Query(term) : 
		Xapian::Query(Xapian::Query::OP_OR, tq, Xapian::Query(term));
	}
	xq = xq.empty() ? tq : Xapian::Query(Xapian::Query::OP_AND, xq, tq);
    }

    ndb->query = xq;
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

// This class (friend to RclDb) exists so that we can have functions that 
// access private RclDb data and have Xapian-specific parameters (so that we 
// don't want them to appear in the public rcldb.h).
class Rcl::DbPops {
 public:
    static bool filterMatch(Rcl::Db *rdb, Xapian::Document &xdoc) {
	// Parse xapian document's data and populate doc fields
	string data = xdoc.get_data();
	ConfSimple parms(&data);

	// The only filtering for now is on file path (subtree)
	string url;
	parms.get(string("url"), url);
	url = url.substr(7);
	if (url.find(rdb->asdata.topdir) == 0) 
	    return true;
	return false;
    }
};

// Get document at rank i in query (i is the index in the whole result
// set, as in the enquire class. We check if the current mset has the
// doc, else ask for an other one. We use msets of 10 documents. Don't
// know if the whole thing makes sense at all but it seems to work.
//
// If there is a postquery filter (ie: file names), we have to
// maintain a correspondance from the sequential external index
// sequence to the internal Xapian hole-y one (the holes being the documents 
// that dont match the filter).
bool Rcl::Db::getDoc(int exti, Doc &doc, int *percent)
{
    const int qquantum = 30;
    LOGDEB1(("Rcl::Db::getDoc: exti %d\n", exti));
    Native *ndb = (Native *)pdata;
    if (!ndb || !ndb->enquire) {
	LOGERR(("Rcl::Db::getDoc: no query opened\n"));
	return false;
    }

    // For now the only post-query filter is on dir subtree
    bool postqfilter = !asdata.topdir.empty();
    LOGDEB1(("Topdir %s postqflt %d\n", asdata.topdir.c_str(), postqfilter));

    int xapi;
    if (postqfilter) {
	// There is a postquery filter, does this fall in already known area ?
	if (exti >= (int)dbindices.size()) {
	    // Have to fetch xapian docs and filter until we get
	    // enough or fail
	    dbindices.reserve(exti+1);
	    // First xapian doc we fetch is the one after last stored 
	    int first = dbindices.size() > 0 ? dbindices.back() + 1 : 0;
	    // Loop until we get enough docs
	    while (exti >= (int)dbindices.size()) {
		LOGDEB(("Rcl::Db::getDoc: fetching %d starting at %d\n",
			qquantum, first));
		ndb->mset = ndb->enquire->get_mset(first, qquantum);
		if (ndb->mset.empty()) {
		    LOGDEB(("Rcl::Db::getDoc: got empty mset\n"));
		    return false;
		}
		first = ndb->mset.get_firstitem();
		for (unsigned int i = 0; i < ndb->mset.size() ; i++) {
		    LOGDEB(("Rcl::Db::getDoc: [%d]\n", i));
		    Xapian::Document xdoc = ndb->mset[i].get_document();
		    if (Rcl::DbPops::filterMatch(this, xdoc)) {
			dbindices.push_back(first + i);
		    }
		}
		first = first + ndb->mset.size();
	    }
	}
	xapi = dbindices[exti];
    } else {
	xapi = exti;
    }


    // From there on, we work with a xapian enquire item number. Fetch it
    int first = ndb->mset.get_firstitem();
    int last = first + ndb->mset.size() -1;

    if (!(xapi >= first && xapi <= last)) {
	LOGDEB(("Fetching for first %d, count %d\n", xapi, qquantum));
	ndb->mset = ndb->enquire->get_mset(xapi, qquantum);
	if (ndb->mset.empty())
	    return false;
	first = ndb->mset.get_firstitem();
	last = first + ndb->mset.size() -1;
    }

    LOGDEB1(("Rcl::Db::getDoc: Qry '%s' win [%d-%d] Estimated results: %d",
	     ndb->query.get_description().c_str(), 
	     first, last,
	     ndb->mset.get_matches_lower_bound()));

    Xapian::Document xdoc = ndb->mset[xapi-first].get_document();
    if (percent)
	*percent = ndb->mset.convert_to_percent(ndb->mset[xapi-first]);

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
    parms.get(string("ipath"), doc.ipath);
    return true;
}
