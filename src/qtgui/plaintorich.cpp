#ifndef lint
static char rcsid[] = "@(#$Id: plaintorich.cpp,v 1.7 2006-01-19 12:01:42 dockes Exp $ (C) 2005 J.F.Dockes";
#endif


#include <string>
#include <utility>
#include <list>
#ifndef NO_NAMESPACES
using std::list;
using std::pair;
#endif /* NO_NAMESPACES */

#include "rcldb.h"
#include "rclconfig.h"
#include "debuglog.h"
#include "textsplit.h"
#include "utf8iter.h"
#include "transcode.h"
#include "smallut.h"

// Text splitter callback used to take note of the position of query terms 
// inside the result text. This is then used to post highlight tags. 
class myTextSplitCB : public TextSplitCB {
 public:
    const list<string>    *terms;  // in: query terms
    list<pair<int, int> > tboffs;  // out: begin and end positions of
                                   // query terms in text

    myTextSplitCB(const list<string>& terms) 
	: terms(&terms) {
    }

    // Callback called by the text-to-words breaker for each word
    virtual bool takeword(const std::string& term, int, int bts, int bte) {
	string dumb;
	Rcl::dumb_string(term, dumb);
	//LOGDEB(("Input dumbbed term: '%s' %d %d %d\n", dumb.c_str(), 
	// pos, bts, bte));
	for (list<string>::const_iterator it = terms->begin(); 
	     it != terms->end(); it++) {
	    if (!stringlowercmp(*it, dumb)) {
		tboffs.push_back(pair<int, int>(bts, bte));
		break;
	    }
	}
	     
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
string plaintorich(const string &in,  const list<string>& terms,
		   list<pair<int, int> >&termoffsets)
{
    LOGDEB(("plaintorich: terms: %s\n", 
	    stringlistdisp(terms).c_str()));

    termoffsets.erase(termoffsets.begin(), termoffsets.end());

    // We first use the text splitter to break the text into words,
    // and compare the words to the search terms, which yields the
    // query terms positions inside the text
    myTextSplitCB cb(terms);
    TextSplit splitter(&cb, true);
    // Note that splitter returns the term locations in byte, not
    // character offset
    splitter.text_to_words(in);

    LOGDEB(("Split done\n"));


    // Rich text output
    string out = "<qt><head><title></title></head><body><p>";

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
	// If we still have terms, check (byte) position
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
	    if (*chariter == ' ' || *chariter == '	') {
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
    return out;
}
