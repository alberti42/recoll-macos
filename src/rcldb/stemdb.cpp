#ifndef lint
static char rcsid[] = "@(#$Id: stemdb.cpp,v 1.1 2006-04-13 09:50:03 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include <algorithm>
#include <map>

#include <xapian.h>
#include <xapian/stem.h>

#include "stemdb.h"
#include "wipedir.h"
#include "pathut.h"
#include "debuglog.h"
#include "smallut.h"

using namespace std;

namespace Rcl {
namespace StemDb {


const static string stemdirstem = "stem_";

/// Compute name of stem db for given base database and language
static string stemdbname(const string& dbdir, const string& lang)
{
    return path_cat(dbdir, stemdirstem + lang);
}

list<string> getLangs(const string& dbdir)
{
    string pattern = stemdirstem + "*";
    list<string> dirs = path_dirglob(dbdir, pattern);
    for (list<string>::iterator it = dirs.begin(); it != dirs.end(); it++) {
	*it = path_basename(*it);
	*it = it->substr(stemdirstem.length(), string::npos);
    }
    return dirs;
}

bool deleteDb(const string& dbdir, const string& lang)
{
    string dir = stemdbname(dbdir, lang);
    if (wipedir(dir) == 0 && rmdir(dir.c_str()) == 0)
	return true;
    return false;
}

// Autoclean/delete directory 
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
bool createDb(Xapian::Database& xdb, const string& dbdir, const string& lang)
{
    LOGDEB(("StemDb::createDb(%s)\n", lang.c_str()));

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
	for (it = xdb.allterms_begin(); 
	     it != xdb.allterms_end(); it++) {
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

    // Create xapian database for stem relations
    string stemdbdir = stemdbname(dbdir, lang);
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
		if (xdb.term_exists(stem))
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
 * Expand term to list of all terms which stem to the same term.
 */
list<string> stemExpand(const string& dbdir, const string& lang,
			const string& term)
{
    list<string> explist;
    try {
	Xapian::Stem stemmer(lang);
	string stem = stemmer.stem_word(term);
	LOGDEB(("stemExpand: [%s] stem-> [%s]\n", term.c_str(), stem.c_str()));
	// Try to fetch the doc from the stem db
	string stemdbdir = stemdbname(dbdir, lang);
	Xapian::Database sdb(stemdbdir);
	LOGDEB1(("stemExpand: %s lastdocid: %d\n", 
		stemdbdir.c_str(), sdb.get_lastdocid()));
	if (!sdb.term_exists(stem)) {
	    LOGDEB1(("Db::stemExpand: no term for %s\n", stem.c_str()));
	    explist.push_back(term);
	    return explist;
	}
	Xapian::PostingIterator did = sdb.postlist_begin(stem);
	if (did == sdb.postlist_end(stem)) {
	    LOGDEB1(("stemExpand: no term(1) for %s\n",stem.c_str()));
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
	LOGDEB(("stemExpand: %s ->  %s\n", stem.c_str(),
		stringlistdisp(explist).c_str()));
    } catch (...) {
	LOGERR(("stemExpand: error accessing stem db\n"));
	explist.push_back(term);
	return explist;
    }
    return explist;
}

}
}
