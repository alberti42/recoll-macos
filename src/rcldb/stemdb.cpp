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
#include "unacpp.h"

#include <iostream>

using namespace std;

namespace Rcl {

// Fast raw detection of non-natural-language words: look for ascii
// chars which are not lowercase letters. Not too sure what islower()
// would do with 8 bit values, so not using it here. If we want to be
// more complete we'd need to go full utf-8
inline static bool p_notlowerascii(unsigned int c)
{
    if (c < 'a' || (c > 'z' && c < 128))
	return true;
    return false;
}

/**
 * Create database of stem to parents associations for a given language.
 */
bool createExpansionDbs(Xapian::WritableDatabase& wdb, 
			const vector<string>& langs)
{
    LOGDEB(("StemDb::createExpansionDbs\n"));
    Chrono cron;

    vector<XapWritableSynFamily> stemdbs;
    for (unsigned int i = 0; i < langs.size(); i++) {
	stemdbs.push_back(XapWritableSynFamily(wdb, synFamStem));
	stemdbs[i].deleteMember(langs[i]);
	stemdbs[i].createMember(langs[i]);
	stemdbs[i].setCurrentMemberName(langs[i]);
    }

    // We walk the list of all terms, and stem each. We skip terms which
    // don't look like natural language.
    // If the stem is not identical to the term, we add a synonym entry.
    // Statistics
    int nostem = 0; // Dont even try: not-alphanum (incomplete for now)
    int stemconst = 0; // Stem == term
    int allsyns = 0; // Total number of entries created

    string ermsg;
    try {
	vector<Xapian::Stem> stemmers;
	for (unsigned int i = 0; i < langs.size(); i++) {
	    stemmers.push_back(Xapian::Stem(langs[i]));
	}

        for (Xapian::TermIterator it = wdb.allterms_begin(); 
	     it != wdb.allterms_end(); it++) {
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

	    // Create stemming synonym for every lang
	    for (unsigned int i = 0; i < langs.size(); i++) {
		string stem = stemmers[i](*it);
		if (stem == *it) {
		    ++stemconst;
		} else {
		    stemdbs[i].addSynonym(stem, *it);
		    LOGDEB0(("Db::createExpansiondbs: [%s] (%s) -> [%s]\n", 
			     (*it).c_str(), langs[i].c_str(), stem.c_str()));
		    ++allsyns;
		}
	    }

        }
    } XCATCHERROR(ermsg);
    if (!ermsg.empty()) {
        LOGERR(("Db::createStemDb: map build failed: %s\n", ermsg.c_str()));
        return false;
    }

    LOGDEB(("StemDb::createExpansionDbs: done: %.2f S\n", cron.secs()));
    LOGDEB(("StemDb::createDb: nostem %d stemconst %d allsyns %d\n", 
	    nostem, stemconst, allsyns));
    return true;
}

/**
 * Expand term to list of all terms which stem to the same term, for one
 * expansion language
 */
bool StemDb::expandOne(const std::string& lang,
		       const std::string& term,
		       vector<string>& result)
{
    try {
	Xapian::Stem stemmer(lang);
	string stem = stemmer(term);
	LOGDEB(("stemExpand:%s: [%s] stem-> [%s]\n", 
                lang.c_str(), term.c_str(), stem.c_str()));

	if (!synExpand(lang, stem, result)) {
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
 * Expand for one or several languages
 */
bool StemDb::stemExpand(const std::string& langs,
			const std::string& term,
			vector<string>& result)
{
    vector<string> llangs;
    stringToStrings(langs, llangs);
    for (vector<string>::const_iterator it = llangs.begin();
	 it != llangs.end(); it++) {
	vector<string> oneexp;
	expandOne(*it, term, oneexp);
	result.insert(result.end(), oneexp.begin(), oneexp.end());
    }
    sort(result.begin(), result.end());
    unique(result.begin(), result.end());
    return true;
}


}
