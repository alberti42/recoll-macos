/* Copyright (C) 2005 J.F.Dockes 
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

/**
 * Management of the auxiliary databases listing stems and their expansion 
 * terms
 */
#include <unistd.h>

#include <algorithm>
#include <map>

#include <xapian.h>

#include "stemdb.h"
#include "pathut.h"
#include "debuglog.h"
#include "smallut.h"
#include "utf8iter.h"
#include "textsplit.h"
#include "rcldb.h"
#include "rcldb_p.h"
#include "synfamily.h"

#include <iostream>

using namespace std;

namespace Rcl {
namespace StemDb {


vector<string> getLangs(Xapian::Database& xdb)
{
    XapSynFamily fam(xdb, synFamStem);
    vector<string> langs;
    (void)fam.getMembers(langs);
    return langs;
}

bool deleteDb(Xapian::WritableDatabase& xdb, const string& lang)
{
    XapWritableSynFamily fam(xdb, synFamStem);
    return fam.deleteMember(lang);
}

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
bool createDb(Xapian::WritableDatabase& xdb, const string& lang)
{
    LOGDEB(("StemDb::createDb(%s)\n", lang.c_str()));
    Chrono cron;

    // First build the in-memory stem database:
    // We walk the list of all terms, and stem each. 
    //   If the stem is identical to the term, no need to create an entry
    // Else, we add an entry to the multimap.
    // At the end, we only save stem-terms associations with several terms, the
    // others are not useful
    // Note: a map<string, vector<string> > would probably be more efficient
    map<string, vector<string> > assocs;
    // Statistics
    int nostem=0; // Dont even try: not-alphanum (incomplete for now)
    int stemconst=0; // Stem == term
    int stemmultiple = 0; // Count of stems with multiple derivatives
    string ermsg;
    try {
        Xapian::Stem stemmer(lang);
        Xapian::TermIterator it;
        for (it = xdb.allterms_begin(); it != xdb.allterms_end(); it++) {
	    // If the term has any non-lowercase 7bit char (that is,
            // numbers, capitals and punctuation) dont stem.
            string::iterator sit = (*it).begin(), eit = sit + (*it).length();
            if ((sit = find_if(sit, eit, p_notlowerascii)) != eit) {
                ++nostem;
                LOGDEB1(("stemskipped: [%s], because of 0x%x\n", 
                         (*it).c_str(), *sit));
                continue;
            }

	    // Detect and skip CJK terms.
	    // We're still sending all other multibyte utf-8 chars to
            // the stemmer, which is not too well defined for
            // xapian<1.0 (very obsolete now), but seems to work
            // anyway. There shouldn't be too many in any case because
            // accents are stripped at this point. 
	    // The effect of stripping accents on stemming is not good, 
            // (e.g: in french partimes -> partim, parti^mes -> part)
	    // but fixing the issue would be complicated.
	    Utf8Iter utfit(*it);
	    if (TextSplit::isCJK(*utfit)) {
		// LOGDEB(("stemskipped: Skipping CJK\n"));
		continue;
	    }

            string stem = stemmer(*it);
            LOGDEB2(("Db::createStemDb: word [%s], stem [%s]\n", (*it).c_str(),
                     stem.c_str()));
            if (stem == *it) {
                ++stemconst;
                continue;
            }
            assocs[stem].push_back(*it);
        }
    } XCATCHERROR(ermsg);
    if (!ermsg.empty()) {
        LOGERR(("Db::createStemDb: map build failed: %s\n", ermsg.c_str()));
        return false;
    }

    LOGDEB1(("StemDb::createDb(%s): in memory map built: %.2f S\n", 
             lang.c_str(), cron.secs()));

    XapWritableSynFamily fam(xdb, synFamStem);
    fam.createMember(lang);

    for (map<string, vector<string> >::const_iterator it = assocs.begin();
         it != assocs.end(); it++) {
	LOGDEB2(("createStemDb: stem [%s]\n", it->first.c_str()));
	// We need an entry even if there is only one derivative
	// so that it is possible to search by entering the stem
	// even if it doesnt exist as a term
	if (it->second.size() > 1)
	    ++stemmultiple;
	if (!fam.addSynonyms(lang, it->first, it->second)) {
	    return false;
	}
    }

    LOGDEB1(("StemDb::createDb(%s): done: %.2f S\n", 
             lang.c_str(), cron.secs()));
    LOGDEB(("Stem map size: %d mult %d const %d no %d \n", 
	    assocs.size(), stemmultiple, stemconst, nostem));
    fam.listMap(lang);
    return true;
}

/**
 * Expand term to list of all terms which stem to the same term, for one
 * expansion language
 */
static bool stemExpandOne(Xapian::Database& xdb, 
			  const std::string& lang,
			  const std::string& term,
			  vector<string>& result)
{
    try {
	Xapian::Stem stemmer(lang);
	string stem = stemmer(term);
	LOGDEB(("stemExpand:%s: [%s] stem-> [%s]\n", 
                lang.c_str(), term.c_str(), stem.c_str()));

	XapSynFamily fam(xdb, synFamStem);
	if (!fam.synExpand(lang, stem, result)) {
	    // ?
	}

	// If the user term or stem are not in the list, add them
	if (find(result.begin(), result.end(), term) == result.end()) {
	    result.push_back(term);
	}
	if (find(result.begin(), result.end(), stem) == result.end()) {
	    result.push_back(stem);
	}
	LOGDEB0(("stemExpand:%s: %s ->  %s\n", lang.c_str(), stem.c_str(),
		 stringsToString(result).c_str()));

    } catch (...) {
	LOGERR(("stemExpand: error accessing stem db. lang [%s]\n",
		lang.c_str()));
	result.push_back(term);
	return false;
    }

    return true;
}
    
/**
 * Expand term to list of all terms which stem to the same term, add the
 * expansion sets for possibly multiple expansion languages
 */
bool stemExpand(Xapian::Database& xdb,
		const std::string& langs,
		const std::string& term,
		vector<string>& result)
{
    vector<string> llangs;
    stringToStrings(langs, llangs);
    for (vector<string>::const_iterator it = llangs.begin();
	 it != llangs.end(); it++) {
	vector<string> oneexp;
	stemExpandOne(xdb, *it, term, oneexp);
	result.insert(result.end(), oneexp.begin(), oneexp.end());
    }
    sort(result.begin(), result.end());
    unique(result.begin(), result.end());
    return true;
}


}
}
