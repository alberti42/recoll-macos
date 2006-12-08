#ifndef lint
static char rcsid[] = "@(#$Id: textsplit.cpp,v 1.27 2006-12-08 07:11:17 dockes Exp $ (C) 2004 J.F.Dockes";
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

//#define UTF8ITER_CHECK
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
    LOGDEB3(("TextSplit::emitterm: [%s] pos %d\n", w.c_str(), pos));

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
	if (pos != prevpos || l != prevlen) {
	    bool ret = cb->takeword(w, pos, btstart, btend);
	    prevlen = w.length();
	    prevpos = pos;
	    return ret;
	}
	LOGDEB2(("TextSplit::emitterm:dup: [%s] pos %d\n", w.c_str(), pos));
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
 * @param spanerase Set if the current span is at its end. Reset it.
 * @param bp        The current BYTE position in the stream
 */
inline bool TextSplit::doemit(bool spanerase, int bp)
{
    LOGDEB3(("TextSplit::doemit:spn [%s] sp %d wrdS %d wrdL %d spe %d bp %d\n",
	     span.c_str(), spanpos, wordStart, wordLen, spanerase, bp));

    // Emit span. When splitting for query, we only emit final spans
    bool spanemitted = false;
    if (spanerase && !(m_flags & TXTS_NOSPANS)) {
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
	spanemitted = true;
	if (!emitterm(true, span, spanpos, bp-span.length(), bp))
	    return false;
    }

    // Emit word if different from span and not 'no words' mode
    if (!(m_flags & TXTS_ONLYSPANS) && wordLen && 
	(!spanemitted || wordLen != span.length())) {
	string s(span.substr(wordStart, wordLen));
	if (!emitterm(false, s, wordpos, bp-wordLen, bp))
	    return false;
    }

    // Adjust state
    wordpos++;
    wordLen = 0;
    if (spanerase) {
	span.erase();
	spanpos = wordpos;
	wordStart = 0;
    } else {
	wordStart = span.length();
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
    LOGDEB2(("TextSplit::text_to_words: cb %p in [%s]\n", cb, 
	    in.substr(0,50).c_str()));

    setcharclasses();

    span.erase();
    number = false;
    wordStart = wordLen = prevpos = prevlen = wordpos = spanpos = 0;

    Utf8Iter it(in);

    for (; !it.eof(); it++) {
	unsigned int c = *it;

	if (c == (unsigned int)-1) {
	    LOGERR(("Textsplit: error occured while scanning UTF-8 string\n"));
	    return false;
	}
	int cc = whatcc(c);
	switch (cc) {
	case LETTER:
	    wordLen += it.appendchartostring(span);
	    break;

	case DIGIT:
	    if (wordLen == 0)
		number = true;
	    wordLen += it.appendchartostring(span);
	    break;

	case SPACE:
	SPACE:
	    if (wordLen || span.length()) {
		if (!doemit(true, it.getBpos()))
		    return false;
		number = false;
	    }
	    break;
	case '-':
	case '+':
	    if (wordLen == 0) {
		if (whatcc(it[it.getCpos()+1]) == DIGIT) {
		    number = true;
		    wordLen += it.appendchartostring(span);
		} else {
		    wordStart += it.appendchartostring(span);
		}
	    } else {
		if (!doemit(false, it.getBpos()))
		    return false;
		number = false;
		wordStart += it.appendchartostring(span);
	    }
	    break;
	case '.':
	case ',':
	    if (number) {
		// 132.jpg ?
		if (whatcc(it[it.getCpos()+1]) != DIGIT)
		    goto SPACE;
		wordLen += it.appendchartostring(span);
		break;
	    } else {
		// If . inside a word, keep it, else, this is whitespace. 
		// We also keep an initial '.' for catching .net, but this adds
		// quite a few spurious terms !
                // Another problem is that something like .x-errs 
		// will be split as .x-errs, x, errs but not x-errs
		// A final comma in a word will be removed by doemit
		if (cc == '.') {
		    if (wordLen) {
			if (!doemit(false, it.getBpos()))
			    return false;
			// span length could have been adjusted by trimming
			// inside doemit
			if (span.length())
			    wordStart += it.appendchartostring(span);
			break;
		    } else {
			wordStart += it.appendchartostring(span);
			break;
		    }
		}
	    }
	    goto SPACE;
	    break;
	case '@':
	    if (wordLen) {
		if (!doemit(false, it.getBpos()))
		    return false;
		number = false;
	    }
	    wordStart += it.appendchartostring(span);
	    break;
	case '\'':
	    // If in word, potential span: o'brien, else, this is more 
	    // whitespace
	    if (wordLen) {
		if (!doemit(false, it.getBpos()))
		    return false;
		number = false;
		wordStart += it.appendchartostring(span);
	    }
	    break;
	case '#': 
	    // Keep it only at end of word ... Special case for c# you see...
	    if (wordLen > 0) {
		int w = whatcc(it[it.getCpos()+1]);
		if (w == SPACE || w == '\n' || w == '\r') {
		    wordLen += it.appendchartostring(span);
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
	    wordLen += it.appendchartostring(span);
	    break;
	}
    }
    if (wordLen || span.length()) {
	if (!doemit(true, it.getBpos()))
	    return false;
    }
    return true;
}

// Callback class for utility function usage
class utSplitterCB : public TextSplitCB {
 public:
    int wcnt;
    utSplitterCB() : wcnt(0) {}
    bool takeword(const string &term, int pos, int bs, int be) {
	wcnt++;
	return true;
    }
};

int TextSplit::countWords(const string& s, TextSplit::Flags flgs)
{
    utSplitterCB cb;
    TextSplit splitter(&cb, flgs);
    splitter.text_to_words(s);
    return cb.wcnt;
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
    bool nooutput;
 public:
    mySplitterCB() : first(1), nooutput(false) {}
    void setNoOut(bool val) {nooutput = val;}
    bool takeword(const string &term, int pos, int bs, int be) {
	if (nooutput)
	    return true;
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
static string teststring1 = " nouvel-an ";

static string thisprog;

static string usage =
    " textsplit [opts] [filename]\n"
    "   -S: no output\n"
    "   -s:  only spans\n"
    "   -w:  only words\n"
    "   -c: just count words\n"
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
#define OPT_s	  0x1 
#define OPT_w	  0x2
#define OPT_S	  0x4
#define OPT_c     0x8

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
	    case 'c':	op_flags |= OPT_c; break;
	    case 's':	op_flags |= OPT_s; break;
	    case 'S':	op_flags |= OPT_S; break;
	    case 'w':	op_flags |= OPT_w; break;
	    default: Usage();	break;
	    }
	argc--; argv++;
    }
    DebugLog::getdbl()->setloglevel(DEBDEB1);
    DebugLog::setfilename("stderr");

    mySplitterCB cb;
    TextSplit::Flags flags = TextSplit::TXTS_NONE;

    if (op_flags&OPT_S)
	cb.setNoOut(true);

    if (op_flags&OPT_s)
	flags = TextSplit::TXTS_ONLYSPANS;
    else if (op_flags&OPT_w)
	flags = TextSplit::TXTS_NOSPANS;

    string data;
    if (argc == 1) {
	const char *filename = *argv++;	argc--;
	if (!strcmp(filename, "stdin")) {
	    char buf[1024];
	    int nread;
	    while ((nread = read(0, buf, 1024)) > 0) {
		data.append(buf, nread);
	    }
	} else if (!file_to_string(filename, data)) 
	    exit(1);
    } else {
	cout << endl << teststring << endl << endl;  
	data = teststring;
    }
    if (op_flags & OPT_c) {
	int n = TextSplit::countWords(data, flags);
	cout << n << " words" << endl;
    } else {
	TextSplit splitter(&cb,  flags);
	splitter.text_to_words(data);
    }    
}
#endif // TEST
