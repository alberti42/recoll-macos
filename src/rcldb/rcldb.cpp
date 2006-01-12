#ifndef lint
static char rcsid[] = "@(#$Id: rcldb.cpp,v 1.52 2006-01-12 09:13:55 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

#include "rcldb.h"
#include "textsplit.h"
#include "transcode.h"
#include "unacpp.h"
#include "conftree.h"
#include "debuglog.h"
#include "pathut.h"
#include "smallut.h"
#include "pathhash.h"
#include "utf8iter.h"
#include "wipedir.h"

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
    const char *ermsg = "Unknown error";
    try {
	LOGDEB(("Rcl::Db::~Db: closing native database\n"));
	if (ndb->iswritable == true) {
	    ndb->wdb.flush();
	}
	delete ndb;
	return;
    } catch (const Xapian::Error &e) {
	ermsg = e.get_msg().c_str();
    } catch (const string &s) {
	ermsg = s.c_str();
    } catch (const char *s) {
	ermsg = s;
    } catch (...) {
	ermsg = "Caught unknown exception";
    }
    LOGERR(("Rcl::Db::~Db: got exception: %s\n", ermsg));
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
    const char *ermsg = "Unknown";
    try {
	switch (mode) {
	case DbUpd:
	case DbTrunc: 
	    {
		int action = (mode == DbUpd) ? Xapian::DB_CREATE_OR_OPEN :
		    Xapian::DB_CREATE_OR_OVERWRITE;
		ndb->wdb = Xapian::WritableDatabase(dir, action);
		LOGDEB(("Rcl::Db::open: lastdocid: %d\n", 
			ndb->wdb.get_lastdocid()));
		ndb->updated.resize(ndb->wdb.get_lastdocid() + 1);
		for (unsigned int i = 0; i < ndb->updated.size(); i++)
		    ndb->updated[i] = false;
		ndb->iswritable = true;
	    }
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
	ermsg = e.get_msg().c_str();
    } catch (const string &s) {
	ermsg = s.c_str();
    } catch (const char *s) {
	ermsg = s;
    } catch (...) {
	ermsg = "Caught unknown exception";
    }
    LOGERR(("Rcl::Db::open: exception while opening [%s]: %s\n", 
	    dir.c_str(), ermsg));
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
    const char *ermsg = "Unknown";
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
	ermsg = e.get_msg().c_str();
    } catch (const string &s) {
	ermsg = s.c_str();
    } catch (const char *s) {
	ermsg = s;
    } catch (...) {
	ermsg = "Caught unknown exception";
    }
    LOGERR(("Rcl::Db:close: exception while deleting db: %s\n", ermsg));
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
#if 0
    LOGDEB(("mySplitterCB::takeword:splitCb: [%s]\n", term.c_str()));
    string printable;
    if (transcode(term, printable, "UTF-8", "ISO-8859-1")) {
	LOGDEB(("                                [%s]\n", printable.c_str()));
    }
#endif

    const char *ermsg;
    try {
	// Note: 1 is the within document frequency increment. It would 
	// be possible to assign different weigths to doc parts (ie title)
	// by using a higher value
	curpos = pos;
	doc.add_posting(term, basepos + curpos, 1);
	return true;
    } catch (const Xapian::Error &e) {
	ermsg = e.get_msg().c_str();
    } catch (...) {
	ermsg= "Unknown error";
    }
    LOGERR(("Rcl::Db: xapian add_posting error %s\n", ermsg));
    return false;
}

// Unaccent and lowercase data, replace \n\r with spaces
// Removing crlfs is so that we can use the text in the document data fields.
// Use unac (with folding extension) for removing accents and casefolding
//
// Note that we always return true (but set out to "" on error). We don't
// want to stop indexation because of a bad string
bool Rcl::dumb_string(const string &in, string &out)
{
    out.erase();
    if (in.empty())
	return true;

    string s1;
    s1.reserve(in.length());
    for (unsigned int i = 0; i < in.length(); i++) {
	if (in[i] == '\n' || in[i] == '\r')
	    s1 += ' ';
	else
	    s1 += in[i];
    }
    if (!unacmaybefold(s1, out, "UTF-8", true)) {
	LOGERR(("dumb_string: unac failed for %s\n", in.c_str()));
	out.erase();
	return true;
    }
    return true;
}

/* From omindex direct */
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
    // No need to replace newlines with spaces, we do this in dumb_string()
    return output;
}

// Truncate longer path and uniquize with hash . The goal for this is
// to avoid xapian max term length limitations, not to gain space (we
// gain very little even with very short maxlens like 30)
#define PATHHASHLEN 150

// Add document in internal form to the database: index the terms in
// the title abstract and body and add special terms for file name,
// date, mime type ... , create the document data record (more
// metadata), and update database
bool Rcl::Db::add(const string &fn, const Rcl::Doc &idoc)
{
    LOGDEB1(("Rcl::Db::add: fn %s\n", fn.c_str()));
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;

    // Truncate abstract, title and keywords to reasonable lengths
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

    // /////// Split and index terms in document body and auxiliary fields
    string noacc;

    // Split and index file name. This supposes that it's either ascii
    // or utf-8. If this fails, we just go on. We need a config
    // parameter for file name charset.
    // Do we really want to fold case here ?
    if (dumb_string(fn, noacc)) {
	splitter.text_to_words(noacc);
	splitData.basepos += splitData.curpos + 100;
    }

    // Split and index title
    if (!dumb_string(doc.title, noacc)) {
	LOGERR(("Rcl::Db::add: dumb_string failed\n"));
	return false;
    }
    splitter.text_to_words(noacc);
    splitData.basepos += splitData.curpos + 100;

    // Split and index body
    if (!dumb_string(doc.text, noacc)) {
	LOGERR(("Rcl::Db::add: dumb_string failed\n"));
	return false;
    }
    splitter.text_to_words(noacc);
    splitData.basepos += splitData.curpos + 100;

    // Split and index keywords
    if (!dumb_string(doc.keywords, noacc)) {
	LOGERR(("Rcl::Db::add: dumb_string failed\n"));
	return false;
    }
    splitter.text_to_words(noacc);
    splitData.basepos += splitData.curpos + 100;

    // Split and index abstract
    if (!dumb_string(doc.abstract, noacc)) {
	LOGERR(("Rcl::Db::add: dumb_string failed\n"));
	return false;
    }
    splitter.text_to_words(noacc);
    splitData.basepos += splitData.curpos + 100;

    ////// Special terms for metadata
    // Mime type
    newdocument.add_term("T" + doc.mimetype);

    // Path name
    string hash;
    pathHash(fn, hash, PATHHASHLEN);
    LOGDEB2(("Rcl::Db::add: pathhash [%s]\n", hash.c_str()));
    string pathterm  = "P" + hash;
    newdocument.add_term(pathterm);

    // Internal path: with path, makes unique identifier for documents
    // inside multidocument files.
    string uniterm;
    if (!doc.ipath.empty()) {
	uniterm  = "Q" + hash + "|" + doc.ipath;
	newdocument.add_term(uniterm);
    }

    // Dates etc...
    time_t mtime = atol(doc.dmtime.empty() ? doc.fmtime.c_str() : 
			doc.dmtime.c_str());
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

    // Document data record. omindex has the following nl separated fields:
    // - url
    // - sample
    // - caption (title limited to 100 chars)
    // - mime type 
    string record = "url=file://" + fn;
    record += "\nmtype=" + doc.mimetype;
    record += "\nfmtime=" + doc.fmtime;
    if (!doc.dmtime.empty()) {
	record += "\ndmtime=" + doc.dmtime;
    }
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

    const char *fnc = fn.c_str();
    // Add db entry or update existing entry:
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
	try {
	    ndb->wdb.add_document(newdocument);
	    LOGDEB(("Rcl::Db::add: %s added (failed re-seek for duplicate)\n", 
		    fnc));
	} catch (...) {
	    LOGERR(("Rcl::Db::add: failed again after replace_document\n"));
	    return false;
	}
    }
    return true;
}

// Test if given filename has changed since last indexed:
bool Rcl::Db::needUpdate(const string &filename, const struct stat *stp)
{
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;

    // If no document exist with this path, we do need update
    string hash;
    pathHash(filename, hash, PATHHASHLEN);
    string pathterm  = "P" + hash;
    const char *ermsg;

    // Look for all documents with this path. We need to look at all
    // to set their existence flag.  We check the update time on the
    // fmtime field which will be identical for all docs inside a
    // multi-document file (we currently always reindex all if the
    // file changed)
    Xapian::PostingIterator doc;
    try {
	if (!ndb->wdb.term_exists(pathterm)) {
	    LOGDEB1(("Db::needUpdate: no such path: %s\n", pathterm.c_str()));
	    return true;
	}

	Xapian::PostingIterator docid0 = ndb->wdb.postlist_begin(pathterm);
	for (Xapian::PostingIterator docid = docid0;
	     docid != ndb->wdb.postlist_end(pathterm); docid++) {

	    Xapian::Document doc = ndb->wdb.get_document(*docid);

	    // Check the date once. no need to look at the others if
	    // the db needs updating. Note that the fmtime used to be
	    // called mtime, and we're keeping compat
	    if (docid == docid0) {
		string data = doc.get_data();
		const char *cp = strstr(data.c_str(), "fmtime=");
		if (cp) {
		    cp += 7;
		} else {
		    cp = strstr(data.c_str(), "mtime=");
		    if (cp)
			cp+= 6;
		}
		long mtime = cp ? atol(cp) : 0;
		if (mtime < stp->st_mtime) {
		    LOGDEB2(("Db::needUpdate: yes: mtime: Db %ld file %ld\n", 
			    (long)mtime, (long)stp->st_mtime));
		    // Db is not up to date. Let's index the file
		    return true;
		} 
	    }

	    // Db is up to date. Make a note that this document exists.
	    if (*docid < ndb->updated.size())
		ndb->updated[*docid] = true;
	}
	return false;
    } catch (const Xapian::Error &e) {
	ermsg = e.get_msg().c_str();
    } catch (...) {
	ermsg= "Unknown error";
    }
    LOGERR(("Db::needUpdate: error while checking existence: %s\n", ermsg));
    return true;
}

const static string stemdirstem = "stem_";
/// Compute name of stem db for given base database and language
static string stemdbname(const string& basename, string lang)
{
    string nm = path_cat(basename, stemdirstem + lang);
    return nm;
}

// Deciding if we try to stem the term. If it has numerals or capitals
// we don't
inline static bool
p_notlowerorutf(unsigned int c)
{
    if (c < 'a' || (c > 'z' && c < 128))
	return true;
    return false;
}

/**
 * Delete stem db for given language
 */
bool Rcl::Db::deleteStemDb(const string& lang)
{
    LOGDEB(("Rcl::Db::deleteStemDb(%s)\n", lang.c_str()));
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;
    if (ndb->isopen == false)
	return false;

    string dir = stemdbname(ndb->basedir, lang);
    if (wipedir(dir) == 0 && rmdir(dir.c_str()) == 0)
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
    if (ndb->isopen == false)
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
		// LOGDEB(("stemskipped: [%s], because of 0x%x\n", 
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
    } catch (const Xapian::Error &e) {
	LOGERR(("Db::createStemDb: build failed: %s\n", e.get_msg().c_str()));
	return false;
    } catch (...) {
	LOGERR(("Db::createStemDb: build failed: no stemmer for %s ? \n", 
		lang.c_str()));
	return false;
    }

    class DirWiper {
    public:
	string dir;
	bool do_it;
	DirWiper(string d) : dir(d), do_it(true) {}
	~DirWiper() {
	    if (do_it) {
		wipedir(dir);
		rmdir(dir.c_str());
	    }
	}
    };
    // Create xapian database for stem relations
    string stemdbdir = stemdbname(ndb->basedir, lang);
    // We want to get rid of the db dir in case of error. This gets disarmed
    // just before success return.
    DirWiper wiper(stemdbdir);
    const char *ermsg = "NOERROR";
    Xapian::WritableDatabase sdb;
    try {
	sdb = Xapian::WritableDatabase(stemdbdir, 
				       Xapian::DB_CREATE_OR_OVERWRITE);
    } catch (const Xapian::Error &e) {
	ermsg = e.get_msg().c_str();
    } catch (const string &s) {
	ermsg = s.c_str();
    } catch (const char *s) {
	ermsg = s;
    } catch (...) {
	ermsg = "Caught unknown exception";
    }
    if (ermsg != "NOERROR") {
	LOGERR(("Rcl::Db::createstemdb: exception while opening [%s]: %s\n", 
		stemdbdir.c_str(), ermsg));
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
    wiper.do_it = false;
    return true;
}

list<string> Rcl::Db::getStemLangs()
{
    list<string> dirs;
    LOGDEB(("Rcl::Db::getStemLang\n"));
    if (pdata == 0)
	return dirs;
    Native *ndb = (Native *)pdata;
    string pattern = stemdirstem + "*";
    dirs = path_dirglob(ndb->basedir, pattern);
    for (list<string>::iterator it = dirs.begin(); it != dirs.end(); it++) {
	*it = path_basename(*it);
	*it = it->substr(stemdirstem.length(), string::npos);
    }
    return dirs;
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
    try {
	ndb->wdb.flush();
    } catch (...) {
	LOGDEB(("Rcl::Db::purge: 1st flush failed\n"));
    }
    for (Xapian::docid docid = 1; docid < ndb->updated.size(); ++docid) {
	if (!ndb->updated[docid]) {
	    try {
		ndb->wdb.delete_document(docid);
		LOGDEB(("Rcl::Db::purge: deleted document #%d\n", docid));
	    } catch (const Xapian::DocNotFoundError &) {
		LOGDEB(("Rcl::Db::purge: document #%d not found\n", docid));
	    }
	}
    }
    try {
	ndb->wdb.flush();
    } catch (...) {
	LOGDEB(("Rcl::Db::purge: 2nd flush failed\n"));
    }
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
	LOGDEB(("stemexpand: [%s] stem-> [%s]\n", term.c_str(), stem.c_str()));
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
	stringToStrings(data.substr(pos, pos1-pos), explist);
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


// Turn string into list of xapian queries. There is little
// interpretation done on the string (no +term -term or filename:term
// stuff). We just separate words and phrases, and interpret
// capitalized terms as wanting no stem expansion. 
// The final list contains one query for each term or phrase
//   - Elements corresponding to a stem-expanded part are an OP_OR
//     composition of the stem-expanded terms (or a single term query).
//   - Elements corresponding to a phrase are an OP_PHRASE composition of the
//     phrase terms (no stem expansion in this case)
static void stringToXapianQueries(const string &iq,
				  const string& stemlang,
				  Native *ndb,
				  list<Xapian::Query> &pqueries,
				  Rcl::Db::QueryOpts opts = Rcl::Db::QO_NONE)
{
    string qstring = iq;

    // Split into (possibly single word) phrases ("this is a phrase"):
    list<string> phrases;
    stringToStrings(qstring, phrases);

    // Then process each phrase: split into terms and transform into
    // appropriate Xapian Query

    for (list<string>::iterator it=phrases.begin(); it !=phrases.end(); it++) {
	LOGDEB(("strToXapianQ: phrase or word: [%s]\n", it->c_str()));

	wsQData splitData;
	TextSplit splitter(&splitData, true);
	splitter.text_to_words(*it);
	LOGDEB1(("strToXapianQ: splitter term count: %d\n", 
		splitData.terms.size()));
	switch(splitData.terms.size()) {
	case 0: continue;// ??
	case 1: // Not a real phrase: one term
	    {
		string term = splitData.terms.front();
		bool nostemexp = false;
		// Check if the first letter is a majuscule in which
		// case we do not want to do stem expansion. Note that
		// the test is convoluted and possibly problematic
		if (term.length() > 0) {
		    string noacterm,noaclowterm;
		    if (unacmaybefold(term, noacterm, "UTF-8", false) &&
			unacmaybefold(noacterm, noaclowterm, "UTF-8", true)) {
			Utf8Iter it1(noacterm);
			Utf8Iter it2(noaclowterm);
			if (*it1 != *it2)
			    nostemexp = true;
		    }
		}
		LOGDEB1(("Term: %s stem expansion: %s\n", 
			term.c_str(), nostemexp?"no":"yes"));

		list<string> exp;  
		string term1;
		Rcl::dumb_string(term, term1);
		// Possibly perform stem compression/expansion
		if (!nostemexp && (opts & Rcl::Db::QO_STEM)) {
		    exp = stemexpand(ndb, term1, stemlang);
		} else {
		    exp.push_back(term1);
		}

		// Push either term or OR of stem-expanded set
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
    LOGDEB(("Rcl::Db::setQuery: q: [%s], opts 0x%x, stemlang %s\n", 
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
bool Rcl::Db::setQuery(AdvSearchData &sdata, QueryOpts opts, 
		       const string& stemlang)
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
	stringToXapianQueries(sdata.allwords, stemlang, ndb, pqueries, opts);
	if (!pqueries.empty()) {
	    xq = Xapian::Query(Xapian::Query::OP_AND, pqueries.begin(), 
			       pqueries.end());
	    pqueries.clear();
	}
    }

    if (!sdata.orwords.empty()) {
	stringToXapianQueries(sdata.orwords, stemlang, ndb, pqueries, opts);
	if (!pqueries.empty()) {
	    Xapian::Query nq;
	    nq = Xapian::Query(Xapian::Query::OP_OR, pqueries.begin(),
			       pqueries.end());
	    xq = xq.empty() ? nq :
		Xapian::Query(Xapian::Query::OP_AND_MAYBE, xq, nq);
	    pqueries.clear();
	}
    }

    // We do no stem expansion on 'No' words. Should we ?
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
	xq = xq.empty() ? tq : Xapian::Query(Xapian::Query::OP_FILTER, xq, tq);
    }

    ndb->query = xq;
    delete ndb->enquire;
    ndb->enquire = new Xapian::Enquire(ndb->db);
    ndb->enquire->set_query(ndb->query);
    ndb->mset = Xapian::MSet();
    // Get the query description and trim the "Xapian::Query"
    sdata.description = ndb->query.get_description();
    if (sdata.description.find("Xapian::Query") == 0)
	sdata.description = sdata.description.substr(strlen("Xapian::Query"));
    LOGDEB(("Rcl::Db::SetQuery: Q: %s\n", sdata.description.c_str()));
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

bool Rcl::Db::dbDataToRclDoc(std::string &data, Doc &doc)
{
    LOGDEB1(("Rcl::Db::dbDataToRclDoc: data: %s\n", data.c_str()));
    ConfSimple parms(&data);
    if (!parms.ok())
	return false;
    parms.get(string("url"), doc.url);
    parms.get(string("mtype"), doc.mimetype);
    parms.get(string("fmtime"), doc.fmtime);
    parms.get(string("dmtime"), doc.dmtime);
    parms.get(string("origcharset"), doc.origcharset);
    parms.get(string("caption"), doc.title);
    parms.get(string("keywords"), doc.keywords);
    parms.get(string("abstract"), doc.abstract);
    parms.get(string("ipath"), doc.ipath);
    return true;
}

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
		try {
		    ndb->mset = ndb->enquire->get_mset(first, qquantum);
		} catch (const Xapian::DatabaseModifiedError &error) {
		    ndb->db.reopen();
		    ndb->mset = ndb->enquire->get_mset(first, qquantum);
		} catch (const Xapian::Error & error) {
		  LOGERR(("enquire->get_mset: exception: %s\n", 
			  error.get_msg().c_str()));
		  abort();
		}

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
	try {
	  ndb->mset = ndb->enquire->get_mset(xapi, qquantum);
	} catch (const Xapian::DatabaseModifiedError &error) {
	    ndb->db.reopen();
	    ndb->mset = ndb->enquire->get_mset(xapi, qquantum);
	} catch (const Xapian::Error & error) {
	  LOGERR(("enquire->get_mset: exception: %s\n", 
		  error.get_msg().c_str()));
	  abort();
	}
	if (ndb->mset.empty())
	    return false;
	first = ndb->mset.get_firstitem();
	last = first + ndb->mset.size() -1;
    }

    LOGDEB1(("Rcl::Db::getDoc: Qry [%s] win [%d-%d] Estimated results: %d",
	     ndb->query.get_description().c_str(), 
	     first, last,
	     ndb->mset.get_matches_lower_bound()));

    Xapian::Document xdoc = ndb->mset[xapi-first].get_document();
    if (percent)
	*percent = ndb->mset.convert_to_percent(ndb->mset[xapi-first]);

    // Parse xapian document's data and populate doc fields
    string data = xdoc.get_data();
    return dbDataToRclDoc(data, doc);
}

// Retrieve document defined by file name and internal path. Very inefficient,
// used only for history display. We'd need to enter path+ipath terms in the
// db if we wanted to make this more efficient.
bool Rcl::Db::getDoc(const string &fn, const string &ipath, Doc &doc)
{
    LOGDEB(("Rcl::Db:getDoc: [%s] (%d) [%s]\n", fn.c_str(), fn.length(),
	    ipath.c_str()));
    if (pdata == 0)
	return false;
    Native *ndb = (Native *)pdata;

    string hash;
    pathHash(fn, hash, PATHHASHLEN);
    string pathterm  = "P" + hash;

    // Look for all documents with this path, searching for the one
    // with the appropriate ipath. This is very inefficient.
    const char *ermsg = "";
    try {
	if (!ndb->db.term_exists(pathterm)) {
	    char len[20];
	    sprintf(len, "%d", pathterm.length());
	    throw string("path inexistant: [") + pathterm +"] length " + len;
	}
	for (Xapian::PostingIterator docid = 
		 ndb->db.postlist_begin(pathterm);
	     docid != ndb->db.postlist_end(pathterm); docid++) {

	    Xapian::Document xdoc = ndb->db.get_document(*docid);
	    string data = xdoc.get_data();
	    if (dbDataToRclDoc(data, doc) && doc.ipath == ipath)
		return true;
	}
    } catch (const Xapian::Error &e) {
	ermsg = e.get_msg().c_str();
    } catch (const string &s) {
	ermsg = s.c_str();
    } catch (const char *s) {
	ermsg = s;
    } catch (...) {
	ermsg = "Caught unknown exception";
    }
    if (*ermsg) {
	LOGERR(("Rcl::Db::getDoc: %s\n", ermsg));
	// Initialize what we can anyway. If this is history, caller
	// will make partial display
	doc.ipath = ipath;
	doc.url = string("file://") + fn;
    }
    return false;
}
