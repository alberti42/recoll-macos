#ifndef lint
static char rcsid[] = "@(#$Id: plaintorich.cpp,v 1.9 2006-01-27 13:42:02 dockes Exp $ (C) 2005 J.F.Dockes";
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
	if (terms.find(dumb) != terms.end()) 
	    tboffs.push_back(pair<int, int>(bts, bte));
	CancelCheck::instance().checkCancel();
	return true;
    }
};

// Fix result text for display inside the gui text window.
//
// To compute the term character positions in the output text, we have
// to emulate how qt's textedit counts chars (ignoring tags and
// duplicate whitespace etc...). This is tricky business and it might
// be better to insert the text char by char, taking note of where qt
// thinks it is at each term.
bool plaintorich(const string& in, string& out, const list<string>& terms,
		 list<pair<int, int> >&termoffsets)
{
    Chrono chron;
    LOGDEB(("plaintorich: terms: %s\n", 
	    stringlistdisp(terms).c_str()));
    out.erase();
    termoffsets.erase(termoffsets.begin(), termoffsets.end());

    // We first use the text splitter to break the text into words,
    // and compare the words to the search terms, which yields the
    // query terms positions inside the text
    myTextSplitCB cb(terms);
    TextSplit splitter(&cb, true);
    // Note that splitter returns the term locations in byte, not
    // character offset
    splitter.text_to_words(in);

    LOGDEB(("plaintorich: split done %d mS\n", chron.millis()));

    // Rich text output
    out = "<qt><head><title></title></head><body><p>";

    // Iterator for the list of input term positions. We use it to
    // output highlight tags and to compute term positions in the
    // output text
    list<pair<int, int> >::iterator it = cb.tboffs.begin();

    // Storage for the current term _character_ position in output.
    pair<int, int> otermcpos;
    // Current char position in output, excluding tags
    int outcpos=0; 
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
	if (it != cb.tboffs.end()) {
	    int ibyteidx = chariter.getBpos();
	    if (ibyteidx == it->first) {
		out += "<termtag>";
		otermcpos.first = outcpos;
	    } else if (ibyteidx == it->second) {
		if (it != cb.tboffs.end())
		    it++;
		otermcpos.second = outcpos;
		termoffsets.push_back(otermcpos);
		out += "</termtag>";
	    }
	}
	switch(*chariter) {
	case '\n':
	    if (ateol < 2) {
		out += "<br>\n";
		ateol++;
		outcpos++;
	    }
	    break;
	case '\r': 
	    break;
	case '<':
	    ateol = 0;
	    out += "&lt;";
	    outcpos++;
	    break;
	case '&':
	    ateol = 0;
	    out += "&amp;";
	    outcpos++;
	    break;
	default:
	    // We don't change the eol status for whitespace, want a real line
	    if (*chariter == ' ' || *chariter == '\t') {
		if (!atblank)
		    outcpos++;
		atblank = 1;
	    } else {
		ateol = 0;
		atblank = 0;
		outcpos++;
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
