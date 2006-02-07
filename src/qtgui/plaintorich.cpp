#ifndef lint
static char rcsid[] = "@(#$Id: plaintorich.cpp,v 1.10 2006-02-07 09:44:33 dockes Exp $ (C) 2005 J.F.Dockes";
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
#ifndef NO_NAMESPACES
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

// Text splitter callback used to take note of the position of query terms 
// inside the result text. This is then used to post highlight tags. 
class myTextSplitCB : public TextSplitCB {
 public:
    string firstTerm;
    set<string>    terms;          // in: user query terms
    list<pair<int, int> > tboffs;  // out: begin and end positions of
                                   // query terms in text

    myTextSplitCB(const list<string>& its) {
	for (list<string>::const_iterator it = its.begin(); it != its.end();
	     it++) {
	    string s;
	    Rcl::dumb_string(*it, s);
	    terms.insert(s);
	}
    }

    // Callback called by the text-to-words breaker for each word
    virtual bool takeword(const std::string& term, int, int bts, int bte) {
	string dumb;
	Rcl::dumb_string(term, dumb);
	//LOGDEB(("Input dumbbed term: '%s' %d %d %d\n", dumb.c_str(), 
	// pos, bts, bte));
	if (terms.find(dumb) != terms.end()) {
	    tboffs.push_back(pair<int, int>(bts, bte));
	    if (firstTerm.empty())
		firstTerm = term;
	}
	CancelCheck::instance().checkCancel();
	return true;
    }
};

// Fix result text for display inside the gui text window.
//
// To compute the term character positions in the output text, we used
// to emulate how qt's textedit counts chars (ignoring tags and
// duplicate whitespace etc...). This was tricky business, dependant
// on qtextedit internals, and we don't do it any more, so we finally
// don't know the term par/car positions in the editor text.  Instead,
// we return the first term encountered, and the caller will use the
// editor's find() function to position on it
bool plaintorich(const string& in, string& out, const list<string>& terms,
		 string *firstTerm)
{
    Chrono chron;
    LOGDEB(("plaintorich: terms: %s\n", 
	    stringlistdisp(terms).c_str()));
    out.erase();

    // We first use the text splitter to break the text into words,
    // and compare the words to the search terms, which yields the
    // query terms positions inside the text
    myTextSplitCB cb(terms);
    TextSplit splitter(&cb, true);
    // Note that splitter returns the term locations in byte, not
    // character offset
    splitter.text_to_words(in);

    if (firstTerm)
	*firstTerm = cb.firstTerm;
    LOGDEB(("plaintorich: split done %d mS\n", chron.millis()));

    // Rich text output
    out = "<qt><head><title></title></head><body><p>";

    // Iterator for the list of input term positions. We use it to
    // output highlight tags and to compute term positions in the
    // output text
    list<pair<int, int> >::iterator tPosIt = cb.tboffs.begin();

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
