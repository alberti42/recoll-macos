#ifndef lint
static char rcsid[] = "@(#$Id: plaintorich.cpp,v 1.14 2006-11-17 10:09:07 dockes Exp $ (C) 2005 J.F.Dockes";
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


#include <string>
#include <utility>
#include <list>
#include <set>
#include <vector>
#include <map>
#include <algorithm>

#ifndef NO_NAMESPACES
using std::vector;
using std::list;
using std::pair;
using std::set;
#endif /* NO_NAMESPACES */

#include "rcldb.h"
#include "rclconfig.h"
#include "debuglog.h"
#include "textsplit.h"
#include "utf8iter.h"
#include "transcode.h"
#include "smallut.h"
#include "plaintorich.h"
#include "cancelcheck.h"


static string vecStringToString(const vector<string>& t)
{
    string sterms;
    for (vector<string>::const_iterator it = t.begin(); it != t.end(); it++) {
	sterms += "[" + *it + "] ";
    }
    return sterms;
}

// Text splitter callback used to take note of the position of query terms 
// inside the result text. This is then used to insert highlight tags. 
class myTextSplitCB : public TextSplitCB {
 public:
    // In: user query terms
    set<string>    terms; 
    // 
    const vector<vector<string> >& m_groups;
    const vector<int>& m_slacks;
    set<string> gterms;

    // Out: first term found in text
    string firstTerm;
    int firstTermPos;

    // Out: begin and end byte positions of query terms/groups in text
    vector<pair<int, int> > tboffs;  

    // group/near terms word positions.
    map<string, vector<int> > m_plists;
    map<int, pair<int, int> > m_gpostobytes;

    myTextSplitCB(const vector<string>& its, vector<vector<string> >&groups, 
		  vector<int>& slacks) : m_groups(groups), m_slacks(slacks)
    {
	for (vector<string>::const_iterator it = its.begin(); 
	     it != its.end(); it++) {
	    terms.insert(*it);
	}
	for (vector<vector<string> >::const_iterator vit = m_groups.begin(); 
	     vit != m_groups.end(); vit++) {
	    for (vector<string>::const_iterator it = (*vit).begin(); 
		 it != (*vit).end(); it++) {
		gterms.insert(*it);
	    }
	}
    }

    // Callback called by the text-to-words breaker for each word
    virtual bool takeword(const std::string& term, int pos, int bts, int bte) {
	string dumb;
	Rcl::dumb_string(term, dumb);
	//LOGDEB(("Input dumbbed term: '%s' %d %d %d\n", dumb.c_str(), 
	// pos, bts, bte));

	// Single search term highlighting: if this word is a search term,
	// Note its byte-offset span. 
	if (terms.find(dumb) != terms.end()) {
	    tboffs.push_back(pair<int, int>(bts, bte));
	    if (firstTerm.empty()) {
		firstTerm = term;
		firstTermPos = pos;
	    }
	}
	
	if (gterms.find(dumb) != gterms.end()) {
	    // Term group (phrase/near) handling
	    m_plists[dumb].push_back(pos);
	    m_gpostobytes[pos] = pair<int,int>(bts, bte);
	    LOGDEB2(("Recorded bpos for %d: %d %d\n", pos, bts, bte));
	}

	CancelCheck::instance().checkCancel();
	return true;
    }
    virtual bool matchGroup(const vector<string>& terms, int dist);
    virtual bool matchGroups();
};

// Code for checking for a NEAR match comes out of xapian phrasepostlist.cc
/** Sort by shorter comparison class */
class VecIntCmpShorter {
    public:
	/** Return true if and only if a is strictly shorter than b.
	 */
        bool operator()(const vector<int> *a, const vector<int> *b) {
            return a->size() < b->size();
        }
};

bool do_test(int window, vector<vector<int>* >& plists, 
	     unsigned int i, int min, int max, int *sp, int *ep)
{
    int tmp = max + 1;
    // take care to avoid underflow
    if (window <= tmp) 
	tmp -= window; 
    else 
	tmp = 0;
    vector<int>::iterator it = plists[i]->begin();

    // Find 1st position bigger than window start
    while (it != plists[i]->end() && *it < tmp)
	it++;

    // Try each position inside window in turn for match with other lists
    while (it != plists[i]->end()) {
	int pos = *it;
	if (pos > min + window - 1) 
	    return false;
	if (i + 1 == plists.size()) {
	    *sp = min;
	    *ep = max;
	    return true;
	}
	if (pos < min) 
	    min = pos;
	else if (pos > max) 
	    max = pos;
	if (do_test(window, plists, i + 1, min, max, sp, ep)) 
	    return true;
	it++;
    }
    return false;
}

// Check if there is a NEAR match for the group of terms
bool myTextSplitCB::matchGroup(const vector<string>& terms, int window)
{
    LOGDEB(("myTextSplitCB::matchGroup:d %d: %s\n", window,
	    vecStringToString(terms).c_str()));
    vector<vector<int>* > plists;
    // Check that each of the group terms has a position list
    for (vector<string>::const_iterator it = terms.begin(); it != terms.end();
	 it++) {
	map<string, vector<int> >::iterator pl;
	if ((pl = m_plists.find(*it)) == m_plists.end()) {
	    LOGDEB(("myTextSplitCB::matchGroup: [%s] not found in m_plists\n",
		    (*it).c_str()));
	    return false;
	}
	plists.push_back(&(pl->second));
    }

    // Sort the positions lists so that the shorter is first
    std::sort(plists.begin(), plists.end(), VecIntCmpShorter());

    // Walk the shortest plist and look for matches
    int sta, sto;
    int pos;
    vector<int>::iterator it = plists[0]->begin();
    do {
	if (it == plists[0]->end())
	    return false;
	pos = *it++;
    } while (!do_test(window, plists, 1, pos, pos, &sta, &sto));

    LOGDEB(("myTextSplitCB::matchGroup: MATCH [%d,%d]\n", sta, sto)); 

    if (firstTerm.empty() || firstTermPos > sta) {
	// firsTerm is used to try an position the preview window over
	// the match. As it's difficult to divine byte/word positions,
	// we use a string search. Try to use the shortest plist for
	// this, which hopefully gives a better chance for the group
	// to be found (it's hopeless to try and match the whole
	// group)
	unsigned int minl = (unsigned int)10E9;
	for (vector<string>::const_iterator it = terms.begin(); 
	     it != terms.end(); it++) {
	    map<string, vector<int> >::iterator pl = m_plists.find(*it);
	    if (pl != m_plists.end() && pl->second.size() < minl) {
		firstTerm = *it;
		LOGDEB(("Firstterm->%s\n", firstTerm.c_str()));
		minl = pl->second.size();
	    }
	}
    }

    map<int, pair<int, int> >::iterator i1 =  m_gpostobytes.find(sta);
    map<int, pair<int, int> >::iterator i2 =  m_gpostobytes.find(sto);
    if (i1 != m_gpostobytes.end() && i2 != m_gpostobytes.end()) {
	LOGDEB(("myTextSplitCB::matchGroup: pushing %d %d\n",
		i1->second.first, i2->second.second));
	tboffs.push_back(pair<int, int>(i1->second.first, i2->second.second));
    } else {
	LOGDEB(("myTextSplitCB::matchGroup: no bpos found for %d or %d\n", 
		sta, sto));
    }
    return true;
}

class PairIntCmpFirst {
public:
    /** Return true if and only if a is strictly shorter than b.
     */
    bool operator()(pair<int,int> a, pair<int, int>b) {
	return a.first < b.first;
    }
};

bool myTextSplitCB::matchGroups()
{
    vector<vector<string> >::const_iterator vit = m_groups.begin();
    vector<int>::const_iterator sit = m_slacks.begin();
    for (; vit != m_groups.end() && sit != m_slacks.end(); vit++, sit++) {
	matchGroup(*vit, *sit + (*vit).size());
    }

    std::sort(tboffs.begin(), tboffs.end(), PairIntCmpFirst());
    return true;
}

// Fix result text for display inside the gui text window.
//
// To compute the term character positions in the output text, we used
// to emulate how qt's textedit counts chars (ignoring tags and
// duplicate whitespace etc...). This was tricky business, dependant
// on qtextedit internals, and we don't do it any more, so we finally
// don't know the term par/car positions in the editor text.  Instead,
// we return the first term encountered, and the caller will use the
// editor's find() function to position on it
bool plaintorich(const string& in, string& out, 
		 RefCntr<Rcl::SearchData> sdata,
		 string *firstTerm, bool noHeader)
{
    Chrono chron;
    out.erase();
    vector<string> terms;
    vector<vector<string> > groups;
    vector<int> slacks;

    sdata->getTerms(terms, groups, slacks);

    {
	LOGDEB(("plaintorich: terms: \n"));
	string sterms = vecStringToString(terms);
	LOGDEB(("  %s\n", sterms.c_str()));
	sterms = "\n";
	LOGDEB(("plaintorich: groups: \n"));
	for (vector<vector<string> >::iterator vit = groups.begin(); 
	     vit != groups.end(); vit++) {
	    sterms += vecStringToString(*vit);
	    sterms += "\n";
	}
	LOGDEB(("  %s", sterms.c_str()));
    }

    // We first use the text splitter to break the text into words,
    // and compare the words to the search terms, which yields the
    // query terms positions inside the text
    myTextSplitCB cb(terms, groups, slacks);
    TextSplit splitter(&cb, TextSplit::TXTS_ONLYSPANS);
    // Note that splitter returns the term locations in byte, not
    // character offset
    splitter.text_to_words(in);
    cb.matchGroups();

    if (firstTerm)
	*firstTerm = cb.firstTerm;
    LOGDEB(("plaintorich: split done %d mS\n", chron.millis()));

    // Rich text output
    if (noHeader)
	out = "";
    else 
	out = "<qt><head><title></title></head><body><p>";

    // Iterator for the list of input term positions. We use it to
    // output highlight tags and to compute term positions in the
    // output text
    vector<pair<int, int> >::iterator tPosIt = cb.tboffs.begin();

    for (vector<pair<int, int> >::const_iterator it = cb.tboffs.begin();
	 it != cb.tboffs.end(); it++) {
	LOGDEB(("plaintorich: region: %d %d\n", it->first, it->second));
    }
    // Input character iterator
    Utf8Iter chariter(in);
    // State variable used to limitate the number of consecutive empty lines 
    int ateol = 0;
    // State variable to update the char pos only for the first of
    // consecutive blank chars
    int atblank = 0;
    for (string::size_type pos = 0; pos != string::npos; pos = chariter++) {
	if (pos && (pos % 1000) == 0) {
	    CancelCheck::instance().checkCancel();
	}
	// If we still have terms positions, check (byte) position
	if (tPosIt != cb.tboffs.end()) {
	    int ibyteidx = chariter.getBpos();
	    if (ibyteidx == tPosIt->first) {
		out += "<termtag>";
	    } else if (ibyteidx == tPosIt->second) {
		if (tPosIt != cb.tboffs.end())
		    tPosIt++;
		out += "</termtag>";
	    }
	}
	switch(*chariter) {
	case '\n':
	    if (ateol < 2) {
		out += "<br>\n";
		ateol++;
	    }
	    break;
	case '\r': 
	    break;
	case '<':
	    ateol = 0;
	    out += "&lt;";
	    break;
	case '&':
	    ateol = 0;
	    out += "&amp;";
	    break;
	default:
	    // We don't change the eol status for whitespace, want a real line
	    if (*chariter == ' ' || *chariter == '\t') {
		atblank = 1;
	    } else {
		ateol = 0;
		atblank = 0;
	    }
	    chariter.appendchartostring(out);
	}
    }
#if 0
    {
	FILE *fp = fopen("/tmp/debugplaintorich", "w");
	fprintf(fp, "%s\n", out.c_str());
	fclose(fp);
    }
#endif
    LOGDEB(("plaintorich: done %d mS\n", chron.millis()));
    return true;
}
