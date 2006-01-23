#ifndef lint
static char rcsid[] = "@(#$Id: textsplit.cpp,v 1.16 2006-01-23 13:32:28 dockes Exp $ (C) 2004 J.F.Dockes";
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
#ifndef TEST_TEXTSPLIT

#include <iostream>
#include <string>
#include <set>
#include "textsplit.h"
#include "debuglog.h"
#include "utf8iter.h"
#include "uproplist.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

/**
 * Splitting a text into words. The code in this file will work with any 
 * charset where the basic separators (.,- etc.) have their ascii values 
 * (ok for UTF-8, ascii, iso8859* and quite a few others).
 *
 * We work in a way which would make it quite difficult to handle non-ascii
 * separator chars (en-dash, etc.). We would then need to actually parse the 
 * utf-8 stream, and use a different way to classify the characters (instead 
 * of a 256 slot array).
 *
 * We are also not using capitalization information.
 *
 * How to fix: use some kind of utf-8 aware iterator, or convert to UCS4 first.
 * Then specialcase all 'real' utf chars, by checking for the few
 * punctuation ones we're interested in (put them in a map). Then
 * classify all other non-ascii as letter, and use the current method
 * for chars < 127.
 */

// Character classes: we have three main groups, and then some chars
// are their own class because they want special handling.
// We have an array with 256 slots where we keep the character types. 
// The array could be fully static, but we use a small function to fill it 
// once.
enum CharClass {LETTER=256, SPACE=257, DIGIT=258};
static int charclasses[256];

static set<unsigned int> unicign;
static void setcharclasses()
{
    static int init = 0;
    if (init)
	return;
    unsigned int i;
    for (i = 0 ; i < 256 ; i ++)
	charclasses[i] = LETTER;

    for (i = 0; i < ' ';i++)
	charclasses[i] = SPACE;

    char digits[] = "0123456789";
    for (i = 0; i  < strlen(digits); i++)
	charclasses[int(digits[i])] = DIGIT;

    char blankspace[] = "\t\v\f ";
    for (i = 0; i < strlen(blankspace); i++)
	charclasses[int(blankspace[i])] = SPACE;

    char seps[] = "!\"$%&()/<=>[\\]^{|}~:;,*`?";
    for (i = 0; i  < strlen(seps); i++)
	charclasses[int(seps[i])] = SPACE;

    char special[] = ".@+-,#'\n\r";
    for (i = 0; i  < strlen(special); i++)
	charclasses[int(special[i])] = special[i];

    init = 1;
    //for (i=0;i<256;i++)cerr<<i<<" -> "<<charclasses[i]<<endl;
    for (i = 0; i < sizeof(uniign); i++) 
	unicign.insert(uniign[i]);
}

// Do some cleanup (the kind which is simpler to do here than in the
// main loop, then send term to our client.
bool TextSplit::emitterm(bool isspan, string &w, int pos, 
			 int btstart, int btend)
{
    LOGDEB2(("TextSplit::emitterm: '%s' pos %d\n", w.c_str(), pos));

    if (!cb)
	return false;

    // Maybe trim end of word. These are chars that we would keep inside 
    // a word or span, but not at the end
    while (w.length() > 0) {
	switch (w[w.length()-1]) {
	case '.':
	case ',':
	case '@':
	case '\'':
	    w.erase(w.length()-1);
	    btend--; if (btend < 0) btend=0;
	    break;
	default:
	    goto breakloop1;
	}
    }
 breakloop1:

    // In addition, it doesn't make sense currently to keep ' at the beginning
    while (w.length() > 0) {
	switch (w[0]) {
	case ',':
	case '\'':
	    w.erase(w.length()-1);
	    btstart++;
	    break;
	default:
	    goto breakloop2;
	}
    }
 breakloop2:

    // 1 char word: we index single letters, but nothing else
    if (w.length() == 1) {
	int c = (int)w[0];
	if (charclasses[c] != LETTER && charclasses[c] != DIGIT) {
	    //cerr << "ERASING single letter term " << c << endl;
	    w.erase();
	}
    }
    if (w.length() > 0 && w.length() < (unsigned)maxWordLength) {
	if (w != prevterm || pos != prevpos) {
	    bool ret = cb->takeword(w, pos, btstart, btend);
	    prevterm = w;
	    prevpos = pos;
	    return ret;
	}
    }
    return true;
}

// A routine called from different places in text_to_words(), to adjust
// the current state and call the word handler. This is purely for
// factoring common code from different places text_to_words()
bool TextSplit::doemit(string &word, int &wordpos, string &span, int spanpos,
		       bool spanerase, int bp)
{
#if 0
    cerr << "doemit: " << "w: '" << word << "' wp: "<< wordpos << " s: '" <<
	span << "' sp: " << spanpos << " spe: " << spanerase << " bp: " << bp 
	 << endl;
#endif

    // When splitting for query, we only emit final spans
    if (fq && !spanerase) {
	wordpos++;
	word.erase();
	return true;
    }

    // Emit span or both word and span if they are different
    if (!emitterm(true, span, spanpos, bp-span.length(), bp))
	return false;
    if (word.length() != span.length() && !fq)
	if (!emitterm(false, word, wordpos, bp-word.length(), bp))
	    return false;

    // Adjust state
    wordpos++;
    if (spanerase)
	span.erase();
    word.erase();

    return true;
}

static inline int whatcc(unsigned int c)
{
    int cc;
    if (c <= 127) {
	cc = charclasses[c]; 
    } else {
	if (c == (unsigned int)-1)
	    cc = SPACE;
	else if (unicign.find(c) != unicign.end())
	    cc = SPACE;
	else
	    cc = LETTER;
    }
    return cc;
}

/** 
 * Splitting a text into terms to be indexed.
 * We basically emit a word every time we see a separator, but some chars are
 * handled specially so that special cases, ie, c++ and dockes@okyz.com etc, 
 * are handled properly,
 */
bool TextSplit::text_to_words(const string &in)
{
    LOGDEB2(("TextSplit::text_to_words: cb %p\n", cb));

    setcharclasses();

    string span; // Current span. Might be jf.dockes@wanadoo.f
    string word; // Current word: no punctuation at all in there
    bool number = false;
    int wordpos = 0; // Term position of current word
    int spanpos = 0; // Term position of current span
    int charpos = 0; // Character position

    Utf8Iter it(in);

    for (; !it.eof(); it++, charpos++) {
	unsigned int c = *it;
	if (c == (unsigned int)-1) {
	    LOGERR(("Textsplit: error occured while scanning UTF-8 string\n"));
	    return false;
	}
	int cc = whatcc(c);
	switch (cc) {
	case SPACE:
	SPACE:
	    if (word.length() || span.length()) {
		if (!doemit(word, wordpos, span, spanpos, true, it.getBpos()))
		    return false;
		number = false;
	    }
	    spanpos = wordpos;
	    span.erase();
	    break;
	case '-':
	case '+':
	    if (word.length() == 0) {
		if (whatcc(it[charpos+1]) == DIGIT) {
		    number = true;
		    word += it;
		    span += it;
		} else
		    span += it;
	    } else {
		if (!doemit(word, wordpos, span, spanpos, false, it.getBpos()))
		    return false;
		number = false;
		span += it;
	    }
	    break;
	case '@':
	    if (word.length()) {
		if (!doemit(word, wordpos, span, spanpos, false, it.getBpos()))
		    return false;
		number = false;
	    } else
		word += it;
	    span += it;
	    break;
	case '\'':
	    if (word.length()) {
		if (!doemit(word, wordpos, span, spanpos, false, it.getBpos()))
		    return false;
		number = false;
		span += it;
	    }
	    break;
	case '.':
	    if (number) {
		word += it;
	    } else {
		//cerr<<"Got . span: '"<<span<<"' word: '"<<word<<"'"<<endl;
		if (word.length()) {
		    if (!doemit(word, wordpos, span, spanpos, false, 
				it.getBpos()))
			return false;
		    number = false;
		} else 
		    word += it;
	    }
	    span += it;
	    break;
	case '#': 
	    // Keep it only at end of word...
	    if (word.length() > 0 && 
		(whatcc(it[charpos+1]) == SPACE ||
		 whatcc(it[charpos+1]) == '\n' || 
		 whatcc(it[charpos+1]) == '\r')) {
		word += it;
		span += it;
	    }
		
	    break;
	case '\n':
	case '\r':
	    if (span.length() && span[span.length() - 1] == '-') {
		// if '-' is the last char before end of line, just
		// ignore the line change. This is the right thing to
		// do almost always. We'd then need a way to check if
		// the - was added as part of the word hyphenation, or was 
		// there in the first place, but this would need a dictionary.
	    } else {
		// Handle like a normal separator
		goto SPACE;
	    }
	    break;
	case LETTER:
	case DIGIT:
	default:
	    if (word.length() == 0) {
		if (cc == DIGIT)
		    number = true;
		else
		    number = false;
	    }
	    word += it;
	    span += it;
	    break;
	}
    }
    if (word.length() || span.length()) {
	if (!doemit(word, wordpos, span, spanpos, true, it.getBpos()))
	    return false;
    }
    return true;
}

#else  // TEST driver ->

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include <iostream>

#include "textsplit.h"
#include "readfile.h"
#include "debuglog.h"

using namespace std;

// A small class to hold state while splitting text
class mySplitterCB : public TextSplitCB {
    int first;
 public:
    mySplitterCB() : first(0) {}

    bool takeword(const std::string &term, int pos, int bs, int be) {
	if (first) {
	    printf("%3s %-20s %4s %4s\n", "pos", "Term", "bs", "be");
	    first = 0;
	}
	printf("%3d %-20s %4d %4d\n", pos, term.c_str(), bs, be);
	return true;
    }
};

static string teststring = 
    "Un bout de texte \n" 
    "normal. "
    "jfd@okyz.com "
    "Ceci. Est;Oui 1.24 n@d @net .net t@v@c c# c++ -10 o'brien l'ami "
    "a 134 +134 -14 -1.5 +1.5 1.54e10 a "
    "@^#$(#$(*) "
    "192.168.4.1 "
    "one\n\rtwo\nthree-\nfour "
    "[olala][ululu] "
    "'o'brien' "
    "utf-8 ucs-4©"
    "\n"							      
;
static string teststring1 = "c++ ";

static string thisprog;

static string usage =
    " textsplit [opts] [filename]\n"
    "   -q: query mode\n"
    " if filename is 'stdin', will read stdin for data (end with ^D)\n"
    "  \n\n"
    ;

static void
Usage(void)
{
    cerr << thisprog  << ": usage:\n" << usage;
    exit(1);
}

static int        op_flags;
#define OPT_q	  0x1 

int main(int argc, char **argv)
{
    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
	(*argv)++;
	if (!(**argv))
	    /* Cas du "adb - core" */
	    Usage();
	while (**argv)
	    switch (*(*argv)++) {
	    case 'q':	op_flags |= OPT_q; break;
	    default: Usage();	break;
	    }
	argc--; argv++;
    }
    DebugLog::getdbl()->setloglevel(DEBDEB1);
    DebugLog::setfilename("stderr");
    mySplitterCB cb;
    TextSplit splitter(&cb, (op_flags&OPT_q) ? true: false);
    if (argc == 1) {
	string data;
	const char *filename = *argv++;	argc--;
	if (!strcmp(filename, "stdin")) {
	    char buf[1024];
	    int nread;
	    while ((nread = read(0, buf, 1024)) > 0) {
		data.append(buf, nread);
	    }
	} else if (!file_to_string(filename, data)) 
	    exit(1);
	splitter.text_to_words(data);
    } else {
	cout << endl << teststring << endl << endl;  
	splitter.text_to_words(teststring);
    }
    
}
#endif // TEST
