#ifndef lint
static char rcsid[] = "@(#$Id: textsplit.cpp,v 1.21 2006-04-11 06:49:45 dockes Exp $ (C) 2004 J.F.Dockes";
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
 * Splitting a text into words. The code in this file works with utf-8
 * in a semi-clean way (see uproplist.h)
 *
 * We are also not using capitalization information.
 *
 * There are a few remnants of the initial utf8-ignorant version in this file.
 */

// Character classes: we have three main groups, and then some chars
// are their own class because they want special handling.
// 
// We have an array with 256 slots where we keep the character types. 
// The array could be fully static, but we use a small function to fill it 
// once.
// The array is actually a remnant of the original version which did no utf8
// It could be reduced to 128, because real (over 128) utf8 chars are now 
// handled with a set holding all the separator values.
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

    char seps[] = "!\"$%&()/<=>[\\]^{|}~:;*`?";
    for (i = 0; i  < strlen(seps); i++)
	charclasses[int(seps[i])] = SPACE;

    char special[] = ".@+-,#'\n\r";
    for (i = 0; i  < strlen(special); i++)
	charclasses[int(special[i])] = special[i];

    for (i = 0; i < sizeof(uniign); i++) 
	unicign.insert(uniign[i]);
    unicign.insert((unsigned int)-1);

    init = 1;
}

// Do some checking (the kind which is simpler to do here than in the
// main loop), then send term to our client.
inline bool TextSplit::emitterm(bool isspan, string &w, int pos, 
			 int btstart, int btend)
{
    LOGDEB2(("TextSplit::emitterm: '%s' pos %d\n", w.c_str(), pos));

    unsigned int l = w.length();
    if (l > 0 && l < (unsigned)maxWordLength) {
	// 1 char word: we index single letters and digits, but
	// nothing else. We might want to turn this into a test for a single
	// utf8 character instead.
	if (l == 1) {
	    int c = (int)w[0];
	    if (charclasses[c] != LETTER && charclasses[c] != DIGIT) {
		//cerr << "ERASING single letter term " << c << endl;
		return true;
	    }
	}
	if (pos != prevpos || l != prevterm.length() || w != prevterm) {
	    bool ret = cb->takeword(w, pos, btstart, btend);
	    prevterm = w;
	    prevpos = pos;
	    return ret;
	}
    }
    return true;
}

/**
 * A routine called from different places in text_to_words(), to
 * adjust the current state of the parser, and call the word
 * handler/emitter. Emit and reset the current word, possibly emit the current
 * span (if different). In query mode, words are not emitted, only final spans
 * 
 * This is purely for factoring common code from different places
 * text_to_words(). 
 * 
 * @return true if ok, false for error. Splitting should stop in this case.
 * @param word      Word value. This will be empty on return in ALL non-error
 *                   cases
 * @param wordpos   Term position for word. Always ++ by us.
 * @param span      Span value
 * @param spanpos   Term position for the current span
 * @param spanerase Set if the current span is at its end. Reset it.
 * @param bp        The current BYTE position in the stream
 */
inline bool TextSplit::doemit(bool spanerase, int bp)
{
#if 0
    cerr << "doemit: " << "w: '" << word << "' wp: "<< wordpos << " s: '" <<
	span << "' sp: " << spanpos << " spe: " << spanerase << " bp: " << bp 
	 << endl;
#endif

    // Emit span. When splitting for query, we only emit final spans
    if (spanerase) {
	// Maybe trim at end These are chars that we would keep inside 
	// a span, but not at the end
	while (span.length() > 0) {
	    switch (span[span.length()-1]) {
	    case '.':
	    case ',':
	    case '@':
	    case '\'':
		span.resize(span.length()-1);
		if (--bp < 0) 
		    bp=0;
		break;
	    default:
		goto breakloop1;
	    }
	}
    breakloop1:
	if (!emitterm(true, span, spanpos, bp-span.length(), bp))
	    return false;
    }


    // Emit word if different from span and not query mode
    if (!fq && (!spanerase || (word.length() != span.length())))
	if (!emitterm(false, word, wordpos, bp-word.length(), bp))
	    return false;

    // Adjust state
    wordpos++;
    word.erase();
    if (spanerase) {
	span.erase();
	spanpos = wordpos;
    }

    return true;
}

static inline int whatcc(unsigned int c)
{
    if (c <= 127) {
	return charclasses[c]; 
    } else {
	if (unicign.find(c) != unicign.end())
	    return SPACE;
	else
	    return LETTER;
    }
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

    span.erase();
    word.erase(); // Current word: no punctuation at all in there
    number = false;
    wordpos = spanpos = charpos = 0;

    Utf8Iter it(in);

    for (; !it.eof(); it++, charpos++) {
	unsigned int c = *it;
	if (c == (unsigned int)-1) {
	    LOGERR(("Textsplit: error occured while scanning UTF-8 string\n"));
	    return false;
	}
	int cc = whatcc(c);
	switch (cc) {
	case LETTER:
	    word += it;
	    span += it;
	    break;

	case DIGIT:
	    if (word.length() == 0)
		number = true;
	    word += it;
	    span += it;
	    break;

	case SPACE:
	SPACE:
	    if (word.length() || span.length()) {
		if (!doemit(true, it.getBpos()))
		    return false;
		number = false;
	    }
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
		if (!doemit(false, it.getBpos()))
		    return false;
		number = false;
		span += it;
	    }
	    break;
	case '.':
	case ',':
	    if (number) {
		word += it;
		span += it;
		break;
	    } else {
		// If . inside a word, keep it, else, this is whitespace. 
		// We also keep an initial '.' for catching .net, but this adds
		// quite a few spurious terms !
                // Another problem is that something like .x-errs 
		// will be split as .x-errs, x, errs but not x-errs
		// A final comma in a word will be removed by doemit
		if (cc == '.') {
		    if (word.length()) {
			if (!doemit(false, it.getBpos()))
			    return false;
			// span length could have been adjusted by trimming
			// inside doemit
			if (span.length())
			    span += it;
			break;
		    } else {
			span += it;
			break;
		    }
		}
	    }
	    goto SPACE;
	    break;
	case '@':
	    if (word.length()) {
		if (!doemit(false, it.getBpos()))
		    return false;
		number = false;
	    }
	    span += it;
	    break;
	case '\'':
	    // If in word, potential span: o'brien, else, this is more 
	    // whitespace
	    if (word.length()) {
		if (!doemit(false, it.getBpos()))
		    return false;
		number = false;
		span += it;
	    }
	    break;
	case '#': 
	    // Keep it only at end of word... Special case for c# you see...
	    if (word.length() > 0) {
		int w = whatcc(it[charpos+1]);
		if (w == SPACE || w == '\n' || w == '\r') {
		    word += it;
		    span += it;
		    break;
		}
	    }
	    goto SPACE;
	    break;
	case '\n':
	case '\r':
	    if (span.length() && span[span.length() - 1] == '-') {
		// if '-' is the last char before end of line, just
		// ignore the line change. This is the right thing to
		// do almost always. We'd then need a way to check if
		// the - was added as part of the word hyphenation, or was 
		// there in the first place, but this would need a dictionary.
		// Also we'd need to check for a soft-hyphen and remove it,
		// but this would require more utf-8 magic
	    } else {
		// Handle like a normal separator
		goto SPACE;
	    }
	    break;

	default:
	    word += it;
	    span += it;
	    break;
	}
    }
    if (word.length() || span.length()) {
	if (!doemit(true, it.getBpos()))
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
	    "Un bout de texte \nnormal. 2eme phrase.3eme;quatrieme.\n"
	    "\"Jean-Francois Dockes\" <jfd@okyz.com>\n"
	    "n@d @net .net t@v@c c# c++ o'brien 'o'brien' l'ami\n"
	    "134 +134 -14 -1.5 +1.5 1.54e10 1,2 1,2e30\n"
	    "@^#$(#$(*)\n"
	    "192.168.4.1 one\n\rtwo\r"
	    "Debut-\ncontinue\n" 
	    "[olala][ululu]  (valeur) (23)\n"
	    "utf-8 ucs-4© \\nodef\n"
	    "','this\n"
	    " ,able,test-domain "
	    " -wl,--export-dynamic "
	    " ~/.xsession-errors "
;
static string teststring1 = " 124, ";

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
