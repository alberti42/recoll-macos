#ifndef lint
static char rcsid[] = "@(#$Id: rcldb.cpp,v 1.67 2006-04-12 10:41:39 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
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
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fnmatch.h>

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

#ifndef MAX
#define MAX(A,B) (A>B?A:B)
#endif
#ifndef MIN
#define MIN(A,B) (A<B?A:B)
#endif
#ifndef NO_NAMESPACES
namespace Rcl {
#endif
// This is how long an abstract we keep or build from beginning of text when
// indexing. It only has an influence on the size of the db as we are free
// to shorten it again when displaying
#define INDEX_ABSTRACT_SIZE 250

// This is the size of the abstract that we synthetize out of query
// term contexts at query time
#define MA_ABSTRACT_SIZE 250
// This is how many words (context size) we keep around query terms
// when building the abstract
#define MA_EXTRACT_WIDTH 4

// Truncate longer path and uniquize with hash . The goal for this is
// to avoid xapian max term length limitations, not to gain space (we
// gain very little even with very short maxlens like 30)
#define PATHHASHLEN 150

// Synthetic abstract marker (to discriminate from abstract actually
// found in doc)
const static string rclSyntAbs = "?!#@";

// Data for a xapian database. There could actually be 2 different
// ones for indexing or query as there is not much in common.
class Native {
 public:
    bool m_isopen;
    bool m_iswritable;
    Db::OpenMode m_mode;
    string m_basedir;

    // List of directories for additional databases to query
    list<string> m_extraDbs;

    // Indexing
    Xapian::WritableDatabase wdb;
    vector<bool> updated;

    // Querying
    Xapian::Database db;
    Xapian::Query    query; // query descriptor: terms and subqueries
			    // joined by operators (or/and etc...)
    Xapian::Enquire *enquire; // Open query descriptor.
    Xapian::MSet     mset;    // Partial result set

    string makeAbstract(Xapian::docid id, const list<string>& terms);
    bool dbDataToRclDoc(std::string &data, Doc &doc, 
			int qopts,
			Xapian::docid docid,
			const list<string>& terms);

    Native() 
	: m_isopen(false), m_iswritable(false), m_mode(Db::DbRO), enquire(0) 
    { }
    ~Native() {
	delete enquire;
    }
    bool filterMatch(Db *rdb, Xapian::Document &xdoc) {
	// Parse xapian document's data and populate doc fields
	string data = xdoc.get_data();
	ConfSimple parms(&data);

	// The only filtering for now is on file path (subtree)
	string url;
	parms.get(string("url"), url);
	url = url.substr(7);
	if (url.find(rdb->m_asdata.topdir) == 0) 
	    return true;
	return false;
    }
};

Db::Db() 
    : m_qOpts(0)
{
    m_ndb = new Native;
}

Db::~Db()
{
    LOGDEB1(("Db::~Db\n"));
    if (m_ndb == 0)
	return;
    LOGDEB(("Db::~Db: isopen %d m_iswritable %d\n", m_ndb->m_isopen, 
	    m_ndb->m_iswritable));
    if (m_ndb->m_isopen == false)
	return;
    const char *ermsg = "Unknown error";
    try {
	LOGDEB(("Db::~Db: closing native database\n"));
	if (m_ndb->m_iswritable == true) {
	    m_ndb->wdb.flush();
	}
	delete m_ndb;
	m_ndb = 0;
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
    LOGERR(("Db::~Db: got exception: %s\n", ermsg));
}

bool Db::open(const string& dir, OpenMode mode, int qops)
{
    if (m_ndb == 0)
	return false;
    LOGDEB(("Db::open: m_isopen %d m_iswritable %d\n", m_ndb->m_isopen, 
	    m_ndb->m_iswritable));

    if (m_ndb->m_isopen) {
	// We used to return an error here but I see no reason to
	if (!close())
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
		m_ndb->wdb = Xapian::WritableDatabase(dir, action);
		LOGDEB(("Db::open: lastdocid: %d\n", 
			m_ndb->wdb.get_lastdocid()));
		m_ndb->updated.resize(m_ndb->wdb.get_lastdocid() + 1);
		for (unsigned int i = 0; i < m_ndb->updated.size(); i++)
		    m_ndb->updated[i] = false;
		m_ndb->m_iswritable = true;
	    }
	    break;
	case DbRO:
	default:
	    m_ndb->m_iswritable = false;
	    m_ndb->db = Xapian::Database(dir);
	    for (list<string>::iterator it = m_ndb->m_extraDbs.begin();
		 it != m_ndb->m_extraDbs.end(); it++) {
		string aerr;
		LOGDEB(("Db::Open: adding query db [%s]\n", it->c_str()));
		aerr.clear();
		try {
		    // Make this non-fatal
		    m_ndb->db.add_database(Xapian::Database(*it));
		} catch (const Xapian::Error &e) {
		    aerr = e.get_msg().c_str();
		} catch (const string &s) {
		    aerr = s.c_str();
		} catch (const char *s) {
		    aerr = s;
		} catch (...) {
		    aerr = "Caught unknown exception";
		}
		if (!aerr.empty())
		    LOGERR(("Db::Open: error while trying to add database "
			    "from [%s]: %s\n", it->c_str(), aerr.c_str()));
	    }
	    break;
	}
	m_qOpts = qops;
	m_ndb->m_mode = mode;
	m_ndb->m_isopen = true;
	m_ndb->m_basedir = dir;
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
    LOGERR(("Db::open: exception while opening [%s]: %s\n", 
	    dir.c_str(), ermsg));
    return false;
}

string Db::getDbDir()
{
    if (m_ndb == 0)
	return "";
    return m_ndb->m_basedir;
}

// Note: xapian has no close call, we delete and recreate the db
bool Db::close()
{
    if (m_ndb == 0)
	return false;
    LOGDEB(("Db::close(): m_isopen %d m_iswritable %d\n", m_ndb->m_isopen, 
	    m_ndb->m_iswritable));
    if (m_ndb->m_isopen == false)
	return true;
    const char *ermsg = "Unknown";
    try {
	if (m_ndb->m_iswritable == true) {
	    m_ndb->wdb.flush();
	    LOGDEB(("Rcl:Db: Called xapian flush\n"));
	}
	delete m_ndb;
	m_ndb = new Native;
	if (m_ndb)
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
    LOGERR(("Db:close: exception while deleting db: %s\n", ermsg));
    return false;
}

bool Db::reOpen()
{
    if (m_ndb && m_ndb->m_isopen) {
	if (!close())
	    return false;
	if (!open(m_ndb->m_basedir, m_ndb->m_mode, m_qOpts)) {
	    return false;
	}
    }
    return true;
}

int Db::docCnt()
{
    if (m_ndb && m_ndb->m_isopen) {
	return m_ndb->m_iswritable ? m_ndb->wdb.get_doccount() : 
	    m_ndb->db.get_doccount();
    }
    return -1;
}

bool Db::addQueryDb(const string &dir) 
{
    LOGDEB(("Db::addQueryDb: ndb %p iswritable %d db [%s]\n", m_ndb,
	      (m_ndb)?m_ndb->m_iswritable:0, dir.c_str()));
    if (!m_ndb)
	return false;
    if (m_ndb->m_iswritable)
	return false;
    if (find(m_ndb->m_extraDbs.begin(), m_ndb->m_extraDbs.end(), dir) == 
	m_ndb->m_extraDbs.end()) {
	m_ndb->m_extraDbs.push_back(dir);
    }
    return reOpen();
}

bool Db::rmQueryDb(const string &dir)
{
    if (!m_ndb)
	return false;
    if (m_ndb->m_iswritable)
	return false;
    if (dir.empty()) {
	m_ndb->m_extraDbs.clear();
    } else {
	list<string>::iterator it = find(m_ndb->m_extraDbs.begin(), 
					 m_ndb->m_extraDbs.end(), dir);
	if (it != m_ndb->m_extraDbs.end()) {
	    m_ndb->m_extraDbs.erase(it);
	}
    }
    return reOpen();
}
bool Db::testDbDir(const string &dir)
{
    string aerr;
    LOGDEB(("Db::testDbDir: [%s]\n", dir.c_str()));
    try {
	Xapian::Database db(dir);
    } catch (const Xapian::Error &e) {
	aerr = e.get_msg().c_str();
    } catch (const string &s) {
	aerr = s.c_str();
    } catch (const char *s) {
	aerr = s;
    } catch (...) {
	aerr = "Caught unknown exception";
    }
    if (!aerr.empty()) {
	LOGERR(("Db::Open: error while trying to open database "
		"from [%s]: %s\n", dir.c_str(), aerr.c_str()));
	return false;
    }
    return true;
}

bool Db::isopen()
{
    if (m_ndb == 0)
	return false;
    return m_ndb->m_isopen;
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
    LOGERR(("Db: xapian add_posting error %s\n", ermsg));
    return false;
}

// Unaccent and lowercase data, replace \n\r with spaces
// Removing crlfs is so that we can use the text in the document data fields.
// Use unac (with folding extension) for removing accents and casefolding
//
// Note that we always return true (but set out to "" on error). We don't
// want to stop indexation because of a bad string
bool dumb_string(const string &in, string &out)
{
    out.erase();
    if (in.empty())
	return true;

    string s1 = neutchars(in, "\n\r");
    if (!unacmaybefold(s1, out, "UTF-8", true)) {
	LOGERR(("dumb_string: unac failed for %s\n", in.c_str()));
	out.erase();
	// See comment at start of func
	return true;
    }
    return true;
}

// Add document in internal form to the database: index the terms in
// the title abstract and body and add special terms for file name,
// date, mime type ... , create the document data record (more
// metadata), and update database
bool Db::add(const string &fn, const Doc &idoc, 
		  const struct stat *stp)
{
    LOGDEB1(("Db::add: fn %s\n", fn.c_str()));
    if (m_ndb == 0)
	return false;

    Doc doc = idoc;

    // Truncate abstract, title and keywords to reasonable lengths. If
    // abstract is currently empty, we make up one with the beginning
    // of the document.
    if (doc.abstract.empty()) {
	doc.abstract = rclSyntAbs + 
	    truncate_to_word(doc.text, INDEX_ABSTRACT_SIZE);
    } else {
	doc.abstract = truncate_to_word(doc.abstract, INDEX_ABSTRACT_SIZE);
    }
    doc.abstract = neutchars(doc.abstract, "\n\r");
    doc.title = truncate_to_word(doc.title, 100);
    doc.keywords = truncate_to_word(doc.keywords, 300);

    Xapian::Document newdocument;

    mySplitterCB splitData(newdocument);

    TextSplit splitter(&splitData);

    // /////// Split and index terms in document body and auxiliary fields
    string noacc;

    // Split and index file name as document term(s)
    if (dumb_string(doc.utf8fn, noacc)) {
	splitter.text_to_words(noacc);
	splitData.basepos += splitData.curpos + 100;
    }

    // Split and index title
    if (!dumb_string(doc.title, noacc)) {
	LOGERR(("Db::add: dumb_string failed\n"));
	return false;
    }
    splitter.text_to_words(noacc);
    splitData.basepos += splitData.curpos + 100;

    // Split and index body
    if (!dumb_string(doc.text, noacc)) {
	LOGERR(("Db::add: dumb_string failed\n"));
	return false;
    }
    splitter.text_to_words(noacc);
    splitData.basepos += splitData.curpos + 100;

    // Split and index keywords
    if (!dumb_string(doc.keywords, noacc)) {
	LOGERR(("Db::add: dumb_string failed\n"));
	return false;
    }
    splitter.text_to_words(noacc);
    splitData.basepos += splitData.curpos + 100;

    // Split and index abstract
    if (!dumb_string(doc.abstract, noacc)) {
	LOGERR(("Db::add: dumb_string failed\n"));
	return false;
    }
    splitter.text_to_words(noacc);
    splitData.basepos += splitData.curpos + 100;

    ////// Special terms for metadata
    // Mime type
    newdocument.add_term("T" + doc.mimetype);

    // Simple file name. This is used for file name searches only. We index
    // it with a term prefix. utf8fn used to be the full path, but it's now
    // the simple file name.
    if (dumb_string(doc.utf8fn, noacc) && !noacc.empty()) {
	noacc = string("XSFN") + noacc;
	newdocument.add_term(noacc);
    }

    // Pathname/ipath terms. This is used for file existence/uptodate
    // checks, and unique id for the replace_document() call 

    // Truncate the filepath part to a reasonable length and
    // replace the truncated part with a hopefully unique hash
    string hash;
    pathHash(fn, hash, PATHHASHLEN);
    LOGDEB2(("Db::add: pathhash [%s]\n", hash.c_str()));
    string pathterm = "P" + hash;
    newdocument.add_term(pathterm);
    
    // Unique term: with path, makes unique identifier for documents
    // inside multidocument files.
    string uniterm;
    if (doc.ipath.empty()) {
	uniterm = pathterm;
    } else {
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
    char sizebuf[20]; 
    sizebuf[0] = 0;
    if (stp)
	sprintf(sizebuf, "%ld", (long)stp->st_size);
    if (sizebuf[0])
	record += string("\nfbytes=") + sizebuf;
    sprintf(sizebuf, "%u", (unsigned int)doc.text.length());
    record += string("\ndbytes=") + sizebuf;
    if (!doc.ipath.empty()) {
	record += "\nipath=" + doc.ipath;
    }
    record += "\ncaption=" + doc.title;
    record += "\nkeywords=" + doc.keywords;
    record += "\nabstract=" + doc.abstract;
    record += "\n";
    LOGDEB1(("Newdocument data: %s\n", record.c_str()));
    newdocument.set_data(record);

    const char *fnc = fn.c_str();
    // Add db entry or update existing entry:
    try {
	Xapian::docid did = 
	    m_ndb->wdb.replace_document(uniterm, newdocument);
	if (did < m_ndb->updated.size()) {
	    m_ndb->updated[did] = true;
	    LOGDEB(("Db::add: docid %d updated [%s , %s]\n", did, fnc,
		    doc.ipath.c_str()));
	} else {
	    LOGDEB(("Db::add: docid %d added [%s , %s]\n", did, fnc, 
		    doc.ipath.c_str()));
	}
    } catch (...) {
	// FIXME: is this ever actually needed?
	try {
	    m_ndb->wdb.add_document(newdocument);
	    LOGDEB(("Db::add: %s added (failed re-seek for duplicate)\n", 
		    fnc));
	} catch (...) {
	    LOGERR(("Db::add: failed again after replace_document\n"));
	    return false;
	}
    }
    return true;
}

// Test if given filename has changed since last indexed:
bool Db::needUpdate(const string &filename, const struct stat *stp)
{
    if (m_ndb == 0)
	return false;

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
	if (!m_ndb->wdb.term_exists(pathterm)) {
	    LOGDEB1(("Db::needUpdate: no such path: %s\n", pathterm.c_str()));
	    return true;
	}

	Xapian::PostingIterator docid0 = m_ndb->wdb.postlist_begin(pathterm);
	for (Xapian::PostingIterator docid = docid0;
	     docid != m_ndb->wdb.postlist_end(pathterm); docid++) {

	    Xapian::Document doc = m_ndb->wdb.get_document(*docid);

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
	    if (*docid < m_ndb->updated.size())
		m_ndb->updated[*docid] = true;
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

// Return list of existing stem db languages
list<string> Db::getStemLangs()
{
    list<string> dirs;
    LOGDEB(("Db::getStemLang\n"));
    if (m_ndb == 0)
	return dirs;
    string pattern = stemdirstem + "*";
    dirs = path_dirglob(m_ndb->m_basedir, pattern);
    for (list<string>::iterator it = dirs.begin(); it != dirs.end(); it++) {
	*it = path_basename(*it);
	*it = it->substr(stemdirstem.length(), string::npos);
    }
    return dirs;
}

/**
 * Delete stem db for given language
 */
bool Db::deleteStemDb(const string& lang)
{
    LOGDEB(("Db::deleteStemDb(%s)\n", lang.c_str()));
    if (m_ndb == 0)
	return false;
    if (m_ndb->m_isopen == false)
	return false;

    string dir = stemdbname(m_ndb->m_basedir, lang);
    if (wipedir(dir) == 0 && rmdir(dir.c_str()) == 0)
	return true;
    return false;
}

// Deciding if we try to stem the term. If it has numerals or capitals
// we don't
inline static bool
p_notlowerascii(unsigned int c)
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
bool Db::createStemDb(const string& lang)
{
    LOGDEB(("Db::createStemDb(%s)\n", lang.c_str()));
    if (m_ndb == 0)
	return false;
    if (m_ndb->m_isopen == false)
	return false;

    // First build the in-memory stem database:
    // We walk the list of all terms, and stem each. 
    //   If the stem is identical to the term, no need to create an entry
    // Else, we add an entry to the multimap.
    // At the end, we only save stem-terms associations with several terms, the
    // others are not useful
    // Note: a map<string, list<string> > would probably be more efficient
    multimap<string, string> assocs;
    // Statistics
    int nostem=0; // Dont even try: not-alphanum (incomplete for now)
    int stemconst=0; // Stem == term
    int stemdiff=0;  // Count of all different stems
    int stemmultiple = 0; // Count of stems with multiple derivatives
    try {
	Xapian::Stem stemmer(lang);
	Xapian::TermIterator it;
	for (it = m_ndb->wdb.allterms_begin(); 
	     it != m_ndb->wdb.allterms_end(); it++) {
	    // If it has any non-lowercase 7bit char, cant be stemmable
	    string::iterator sit = (*it).begin(), eit = sit + (*it).length();
	    if ((sit = find_if(sit, eit, p_notlowerascii)) != eit) {
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
    string stemdbdir = stemdbname(m_ndb->m_basedir, lang);
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
	LOGERR(("Db::createstemdb: exception while opening [%s]: %s\n", 
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
	    if (derivs.size() == 1) {
		// Exactly one term stems to this. Check for the case where
		// the stem itself exists as a term. The code above would not
		// have inserted anything in this case.
		if (m_ndb->wdb.term_exists(stem))
		    derivs.push_back(stem);
	    }
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
		    LOGERR(("Db::createstemdb: replace failed\n"));
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


/**
 * This is called at the end of an indexing session, to delete the
 * documents for files that are no longer there. 
 */
bool Db::purge()
{
    LOGDEB(("Db::purge\n"));
    if (m_ndb == 0)
	return false;
    LOGDEB(("Db::purge: m_isopen %d m_iswritable %d\n", m_ndb->m_isopen, 
	    m_ndb->m_iswritable));
    if (m_ndb->m_isopen == false || m_ndb->m_iswritable == false)
	return false;

    // There seems to be problems with the document delete code, when
    // we do this, the database is not actually updated. Especially,
    // if we delete a bunch of docs, so that there is a hole in the
    // docids at the beginning, we can't add anything (appears to work
    // and does nothing). Maybe related to the exceptions below when
    // trying to delete an unexistant document ?
    // Flushing before trying the deletes seeems to work around the problem
    try {
	m_ndb->wdb.flush();
    } catch (...) {
	LOGDEB(("Db::purge: 1st flush failed\n"));
    }
    for (Xapian::docid docid = 1; docid < m_ndb->updated.size(); ++docid) {
	if (!m_ndb->updated[docid]) {
	    try {
		m_ndb->wdb.delete_document(docid);
		LOGDEB(("Db::purge: deleted document #%d\n", docid));
	    } catch (const Xapian::DocNotFoundError &) {
		LOGDEB(("Db::purge: document #%d not found\n", docid));
	    }
	}
    }
    try {
	m_ndb->wdb.flush();
    } catch (...) {
	LOGDEB(("Db::purge: 2nd flush failed\n"));
    }
    return true;
}

/**
 * Expand term to list of all terms which stem to the same term.
 */
static list<string> stemexpand(Native *m_ndb, string term, const string& lang)
{
    list<string> explist;
    try {
	Xapian::Stem stemmer(lang);
	string stem = stemmer.stem_word(term);
	LOGDEB(("stemexpand: [%s] stem-> [%s]\n", term.c_str(), stem.c_str()));
	// Try to fetch the doc from the stem db
	string stemdbdir = stemdbname(m_ndb->m_basedir, lang);
	Xapian::Database sdb(stemdbdir);
	LOGDEB1(("stemexpand: %s lastdocid: %d\n", 
		stemdbdir.c_str(), sdb.get_lastdocid()));
	if (!sdb.term_exists(stem)) {
	    LOGDEB1(("Db::stemexpand: no term for %s\n", stem.c_str()));
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


// Splitter callback for breaking query into terms
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
	    dumb_string(*it, dumb);
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
				  Native *m_ndb,
				  list<Xapian::Query> &pqueries,
				  Db::QueryOpts opts = Db::QO_NONE)
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
		dumb_string(term, term1);
		// Possibly perform stem compression/expansion
		if (!nostemexp && (opts & Db::QO_STEM)) {
		    exp = stemexpand(m_ndb, term1, stemlang);
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
bool Db::setQuery(const std::string &iqstring, QueryOpts opts, 
		       const string& stemlang)
{
    LOGDEB(("Db::setQuery: q: [%s], opts 0x%x, stemlang %s\n", 
	    iqstring.c_str(), (unsigned int)opts, stemlang.c_str()));
    if (!m_ndb)
	return false;
    m_asdata.erase();
    m_dbindices.clear();
    list<Xapian::Query> pqueries;
    stringToXapianQueries(iqstring, stemlang, m_ndb, pqueries, opts);
    m_ndb->query = Xapian::Query(Xapian::Query::OP_OR, pqueries.begin(), 
			       pqueries.end());
    delete m_ndb->enquire;
    m_ndb->enquire = new Xapian::Enquire(m_ndb->db);
    m_ndb->enquire->set_query(m_ndb->query);
    m_ndb->mset = Xapian::MSet();
    return true;
}

// Prepare query out of "advanced search" data
bool Db::setQuery(AdvSearchData &sdata, QueryOpts opts, 
		       const string& stemlang)
{
    LOGDEB(("Db::setQuery: adv:\n"));
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

    m_asdata = sdata;
    m_dbindices.clear();

    if (!m_ndb)
	return false;
    list<Xapian::Query> pqueries;
    Xapian::Query xq;

    if (!sdata.filename.empty()) {
	LOGDEB((" filename search\n"));
	// File name search, with possible wildcards. 
	// We expand wildcards by scanning the filename terms (prefixed 
        // with XSFN) from the database. 
	// We build an OR query with the expanded values if any.
	string pattern;
	dumb_string(sdata.filename, pattern);

	// If pattern is not quoted, and has no wildcards, we add * at
	// each end: match any substring
	if (pattern[0] == '"' && pattern[pattern.size()-1] == '"') {
	    pattern = pattern.substr(1, pattern.size() -2);
	} else if (pattern.find_first_of("*?[") == string::npos) {
	    pattern = "*" + pattern + "*";
	} // else let it be

	LOGDEB((" pattern: [%s]\n", pattern.c_str()));

	// Match pattern against all file names in the db
	Xapian::TermIterator it = m_ndb->db.allterms_begin(); 
	it.skip_to("XSFN");
	list<string> names;
	for (;it != m_ndb->db.allterms_end(); it++) {
	    if ((*it).find("XSFN") != 0)
		break;
	    string fn = (*it).substr(4);
	    LOGDEB2(("Matching [%s] and [%s]\n", pattern.c_str(), fn.c_str()));
	    if (fnmatch(pattern.c_str(), fn.c_str(), 0) != FNM_NOMATCH) {
		names.push_back((*it).c_str());
	    }
	    // Limit the match count
	    if (names.size() > 1000) {
		LOGERR(("Db::SetQuery: too many matched file names\n"));
		break;
	    }
	}
	if (names.empty()) {
	    // Build an impossible query: we know its impossible because we
	    // control the prefixes!
	    names.push_back("XIMPOSSIBLE");
	}
	// Build a query out of the matching file name terms.
	xq = Xapian::Query(Xapian::Query::OP_OR, names.begin(), names.end());
    }

    if (!sdata.allwords.empty()) {
	stringToXapianQueries(sdata.allwords, stemlang, m_ndb, pqueries, opts);
	if (!pqueries.empty()) {
	    Xapian::Query nq = 
		Xapian::Query(Xapian::Query::OP_AND, pqueries.begin(),
			      pqueries.end());
	    xq = xq.empty() ? nq :
		Xapian::Query(Xapian::Query::OP_AND, xq, nq);
	    pqueries.clear();
	}
    }

    if (!sdata.orwords.empty()) {
	stringToXapianQueries(sdata.orwords, stemlang, m_ndb, pqueries, opts);
	if (!pqueries.empty()) {
	    Xapian::Query nq = 
		Xapian::Query(Xapian::Query::OP_OR, pqueries.begin(),
			       pqueries.end());
	    xq = xq.empty() ? nq :
		Xapian::Query(Xapian::Query::OP_AND_MAYBE, xq, nq);
	    pqueries.clear();
	}
    }

    if (!sdata.phrase.empty()) {
	Xapian::Query nq;
	string s = string("\"") + sdata.phrase + string("\"");
	stringToXapianQueries(s, stemlang, m_ndb, pqueries);
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

    // "And not" part. Must come last, as we have to check it's not
    // the only term in the query.  We do no stem expansion on 'No'
    // words. Should we ?
    if (!sdata.nowords.empty()) {
	stringToXapianQueries(sdata.nowords, stemlang, m_ndb, pqueries);
	if (!pqueries.empty()) {
	    Xapian::Query nq;
	    nq = Xapian::Query(Xapian::Query::OP_OR, pqueries.begin(),
			       pqueries.end());
	    if (xq.empty()) {
		// Xapian cant do this currently. Have to have a positive 
		// part!
		sdata.description = "Error: pure negative query\n";
		LOGERR(("Rcl::Db::setQuery: error: pure negative query\n"));
		return false;
	    }
	    xq = Xapian::Query(Xapian::Query::OP_AND_NOT, xq, nq);
	    pqueries.clear();
	}
    }

    m_ndb->query = xq;
    delete m_ndb->enquire;
    m_ndb->enquire = new Xapian::Enquire(m_ndb->db);
    m_ndb->enquire->set_query(m_ndb->query);
    m_ndb->mset = Xapian::MSet();
    // Get the query description and trim the "Xapian::Query"
    sdata.description = m_ndb->query.get_description();
    if (sdata.description.find("Xapian::Query") == 0)
	sdata.description = sdata.description.substr(strlen("Xapian::Query"));
    LOGDEB(("Db::SetQuery: Q: %s\n", sdata.description.c_str()));
    return true;
}

bool Db::getQueryTerms(list<string>& terms)
{
    if (!m_ndb)
	return false;

    terms.clear();
    Xapian::TermIterator it;
    for (it = m_ndb->query.get_terms_begin(); it != m_ndb->query.get_terms_end();
	 it++) {
	terms.push_back(*it);
    }
    return true;
}

// Mset size
static const int qquantum = 30;

int Db::getResCnt()
{
    if (!m_ndb || !m_ndb->enquire) {
	LOGERR(("Db::getResCnt: no query opened\n"));
	return -1;
    }
    if (m_ndb->mset.size() <= 0) {
	try {
	    m_ndb->mset = m_ndb->enquire->get_mset(0, qquantum);
	} catch (const Xapian::DatabaseModifiedError &error) {
	    m_ndb->db.reopen();
	    m_ndb->mset = m_ndb->enquire->get_mset(0, qquantum);
	} catch (const Xapian::Error & error) {
	    LOGERR(("enquire->get_mset: exception: %s\n", 
		    error.get_msg().c_str()));
	    return -1;
	}
    }

    return m_ndb->mset.get_matches_lower_bound();
}

bool Native::dbDataToRclDoc(std::string &data, Doc &doc, 
			    int qopts,
			    Xapian::docid docid, const list<string>& terms)
{
    LOGDEB1(("Db::dbDataToRclDoc: data: %s\n", data.c_str()));
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
    bool syntabs = false;
    if (doc.abstract.find(rclSyntAbs) == 0) {
	doc.abstract = doc.abstract.substr(rclSyntAbs.length());
	syntabs = true;
    }
    if ((qopts && Db::QO_BUILD_ABSTRACT) && !terms.empty()) {
	LOGDEB1(("dbDataToRclDoc:: building abstract from position data\n"));
	if (doc.abstract.empty() || syntabs || 
	    (qopts & Db::QO_REPLACE_ABSTRACT))
	    doc.abstract = makeAbstract(docid, terms);
    }
    parms.get(string("ipath"), doc.ipath);
    parms.get(string("fbytes"), doc.fbytes);
    parms.get(string("dbytes"), doc.dbytes);
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
bool Db::getDoc(int exti, Doc &doc, int *percent)
{
    LOGDEB1(("Db::getDoc: exti %d\n", exti));
    if (!m_ndb || !m_ndb->enquire) {
	LOGERR(("Db::getDoc: no query opened\n"));
	return false;
    }

    // For now the only post-query filter is on dir subtree
    bool postqfilter = !m_asdata.topdir.empty();
    LOGDEB1(("Topdir %s postqflt %d\n", m_asdata.topdir.c_str(), postqfilter));

    int xapi;
    if (postqfilter) {
	// There is a postquery filter, does this fall in already known area ?
	if (exti >= (int)m_dbindices.size()) {
	    // Have to fetch xapian docs and filter until we get
	    // enough or fail
	    m_dbindices.reserve(exti+1);
	    // First xapian doc we fetch is the one after last stored 
	    int first = m_dbindices.size() > 0 ? m_dbindices.back() + 1 : 0;
	    // Loop until we get enough docs
	    while (exti >= (int)m_dbindices.size()) {
		LOGDEB(("Db::getDoc: fetching %d starting at %d\n",
			qquantum, first));
		try {
		    m_ndb->mset = m_ndb->enquire->get_mset(first, qquantum);
		} catch (const Xapian::DatabaseModifiedError &error) {
		    m_ndb->db.reopen();
		    m_ndb->mset = m_ndb->enquire->get_mset(first, qquantum);
		} catch (const Xapian::Error & error) {
		  LOGERR(("enquire->get_mset: exception: %s\n", 
			  error.get_msg().c_str()));
		  abort();
		}

		if (m_ndb->mset.empty()) {
		    LOGDEB(("Db::getDoc: got empty mset\n"));
		    return false;
		}
		first = m_ndb->mset.get_firstitem();
		for (unsigned int i = 0; i < m_ndb->mset.size() ; i++) {
		    LOGDEB(("Db::getDoc: [%d]\n", i));
		    Xapian::Document xdoc = m_ndb->mset[i].get_document();
		    if (m_ndb->filterMatch(this, xdoc)) {
			m_dbindices.push_back(first + i);
		    }
		}
		first = first + m_ndb->mset.size();
	    }
	}
	xapi = m_dbindices[exti];
    } else {
	xapi = exti;
    }


    // From there on, we work with a xapian enquire item number. Fetch it
    int first = m_ndb->mset.get_firstitem();
    int last = first + m_ndb->mset.size() -1;

    if (!(xapi >= first && xapi <= last)) {
	LOGDEB(("Fetching for first %d, count %d\n", xapi, qquantum));
	try {
	  m_ndb->mset = m_ndb->enquire->get_mset(xapi, qquantum);
	} catch (const Xapian::DatabaseModifiedError &error) {
	    m_ndb->db.reopen();
	    m_ndb->mset = m_ndb->enquire->get_mset(xapi, qquantum);
	} catch (const Xapian::Error & error) {
	  LOGERR(("enquire->get_mset: exception: %s\n", 
		  error.get_msg().c_str()));
	  abort();
	}
	if (m_ndb->mset.empty())
	    return false;
	first = m_ndb->mset.get_firstitem();
	last = first + m_ndb->mset.size() -1;
    }

    LOGDEB1(("Db::getDoc: Qry [%s] win [%d-%d] Estimated results: %d",
	     m_ndb->query.get_description().c_str(), 
	     first, last,
	     m_ndb->mset.get_matches_lower_bound()));

    Xapian::Document xdoc = m_ndb->mset[xapi-first].get_document();
    Xapian::docid docid = *(m_ndb->mset[xapi-first]);
    if (percent)
	*percent = m_ndb->mset.convert_to_percent(m_ndb->mset[xapi-first]);

    // Parse xapian document's data and populate doc fields
    string data = xdoc.get_data();
    list<string> terms;
    getQueryTerms(terms);
    return m_ndb->dbDataToRclDoc(data, doc, m_qOpts, docid, terms);
}

// Retrieve document defined by file name and internal path. Very inefficient,
// used only for history display. We'd need to enter path+ipath terms in the
// db if we wanted to make this more efficient.
bool Db::getDoc(const string &fn, const string &ipath, Doc &doc, int *pc)
{
    LOGDEB(("Db:getDoc: [%s] (%d) [%s]\n", fn.c_str(), fn.length(),
	    ipath.c_str()));
    if (m_ndb == 0)
	return false;

    // Initialize what we can in any case. If this is history, caller
    // will make partial display in case of error
    doc.ipath = ipath;
    doc.url = string("file://") + fn;
    if (*pc)
	*pc = 100;

    string hash;
    pathHash(fn, hash, PATHHASHLEN);
    string pathterm  = "P" + hash;
    // Look for all documents with this path, searching for the one
    // with the appropriate ipath. This is very inefficient.
    const char *ermsg = "";
    try {
	if (!m_ndb->db.term_exists(pathterm)) {
	    // Document found in history no longer in the database.
	    // We return true (because their might be other ok docs further)
	    // but indicate the error with pc = -1
	    if (*pc) 
		*pc = -1;
	    LOGINFO(("Db:getDoc: no such path in index: [%s] (len %d)\n",
		     pathterm.c_str(), pathterm.length()));
	    return true;
	}
	for (Xapian::PostingIterator docid = 
		 m_ndb->db.postlist_begin(pathterm);
	     docid != m_ndb->db.postlist_end(pathterm); docid++) {

	    Xapian::Document xdoc = m_ndb->db.get_document(*docid);
	    string data = xdoc.get_data();
	    list<string> terms;
	    if (m_ndb->dbDataToRclDoc(data, doc, QO_NONE, *docid, terms) 
		&& doc.ipath == ipath)
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
	LOGERR(("Db::getDoc: %s\n", ermsg));
    }
    return false;
}

// Width of a sample extract around a query term
//
// We build a possibly full size but sparsely populated (only around
// the search term) reconstruction of the document. It would be
// possible to compress the array, by having only multiple chunks
// around the terms, but this would seriously complicate the data
// structure.
string Native::makeAbstract(Xapian::docid docid, const list<string>& terms)
{
    Chrono chron;
    // A buffer that we populate with the document terms, at their position
    vector<string> buf;

    // Go through the list of query terms. For each entry in each
    // position list, populate the slot in the document buffer, and
    // remember the position and its neigbours
    vector<unsigned int> qtermposs; // The term positions
    set<unsigned int> chunkposs; // All the positions we shall populate
    for (list<string>::const_iterator qit = terms.begin(); qit != terms.end();
	 qit++) {
	Xapian::PositionIterator pos;
	// There may be query terms not in this doc. This raises an
	// exception when requesting the position list, we just catch it.
	try {
	    unsigned int occurrences = 0;
	    for (pos = db.positionlist_begin(docid, *qit); 
		 pos != db.positionlist_end(docid, *qit); pos++) {
		unsigned int ipos = *pos;
		LOGDEB1(("Abstract: [%s] at %d\n", qit->c_str(), ipos));
		// Possibly extend the array. Do it in big chunks
		if (ipos + MA_EXTRACT_WIDTH >= buf.size()) {
		    buf.resize(ipos + MA_EXTRACT_WIDTH + 1000);
		}
		buf[ipos] = *qit;
		// Remember the term position
		qtermposs.push_back(ipos);
		// Add adjacent slots to the set to populate at next step
		for (unsigned int ii = MAX(0, ipos-MA_EXTRACT_WIDTH); 
		     ii <= MIN(ipos+MA_EXTRACT_WIDTH, buf.size()-1); ii++) {
		    chunkposs.insert(ii);
		}
		// Limit the number of occurences we keep for each
		// term. The abstract has a finite length anyway !
		if (occurrences++ > 10)
		    break;
	    }
	} catch (...) {
	}
    }

    LOGDEB1(("Abstract:%d:chosen number of positions %d. Populating\n", 
	    chron.millis(), qtermposs.size()));

    // Walk the full document position list and populate slots around
    // the query terms. We arbitrarily truncate the list to avoid
    // taking forever. If we do cutoff, the abstract may be
    // inconsistant, which is bad...
    { Xapian::TermIterator term;
	int cutoff = 500 * 1000;
	for (term = db.termlist_begin(docid);
	     term != db.termlist_end(docid); term++) {
	    Xapian::PositionIterator pos;
	    for (pos = db.positionlist_begin(docid, *term); 
		 pos != db.positionlist_end(docid, *term); pos++) {
		if (cutoff-- < 0)
		    break;
		unsigned int ipos = *pos;
		if (chunkposs.find(ipos) != chunkposs.end()) {
		    buf[ipos] = *term;
		}
	    }
	    if (cutoff-- < 0)
		break;
	}
    }

    LOGDEB1(("Abstract:%d: randomizing and extracting\n", chron.millis()));

    // We randomize the selection of term positions, from which we
    // shall pull, starting at the beginning, until the abstract is
    // big enough. The abstract is finally built in correct position
    // order, thanks to the position map.
    random_shuffle(qtermposs.begin(), qtermposs.end());
    map<unsigned int, string> mabs;
    unsigned int abslen = 0;
    LOGDEB1(("Abstract:%d: extracting\n", chron.millis()));
    // Extract data around the first (in random order) term positions,
    // and store the chunks in the map
    for (vector<unsigned int>::const_iterator it = qtermposs.begin();
	 it != qtermposs.end(); it++) {
	unsigned int ipos = *it;
	unsigned int start = MAX(0, ipos-MA_EXTRACT_WIDTH);
	unsigned int end = MIN(ipos+MA_EXTRACT_WIDTH, buf.size()-1);
	string chunk;
	for (unsigned int ii = start; ii <= end; ii++) {
	    if (!buf[ii].empty()) {
		chunk += buf[ii] + " ";
		abslen += buf[ii].length();
	    }
	    if (abslen > MA_ABSTRACT_SIZE)
		break;
	}
	if (end != buf.size()-1)
	    chunk += "... ";
	mabs[ipos] = chunk;
	if (abslen > MA_ABSTRACT_SIZE)
	    break;
    }

    // Build the abstract by walking the map (in order of position)
    string abstract;
    for (map<unsigned int, string>::const_iterator it = mabs.begin();
	 it != mabs.end(); it++) {
	abstract += (*it).second;
    }
    LOGDEB(("Abtract: done in %d mS\n", chron.millis()));
    return abstract;
}
#ifndef NO_NAMESPACES
}
#endif
