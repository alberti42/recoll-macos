#ifndef lint
static char rcsid[] = "@(#$Id: textsplit.cpp,v 1.4 2004-12-17 13:01:01 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
#ifndef TEST_TEXTSPLIT

#include <iostream>
#include <string>

#include "textsplit.h"

using namespace std;

/**
 * Splitting a text into words. The code in this file will work with any 
 * charset where the basic separators (.,- etc.) have their ascii values 
 * (ok for UTF-8, ascii, iso8859* and quite a few others).
 *
 * We work in a way which would make it quite difficult to handle non-ascii
 * separator chars (en-dash,etc.). We would then need to actually parse the 
 * utf-8 stream, and use a different way to classify the characters (instead 
 * of a 256 slot array).
 *
 * We are also not using capitalization information.
 */

// Character classes: we have three main groups, and then some chars
// are their own class because they want special handling.
// We have an array with 256 slots where we keep the character states. 
// The array could be fully static, but we use a small function to fill it 
// once.
enum CharClass {LETTER=256, SPACE=257, DIGIT=258};
static int charclasses[256];
static void setcharclasses()
{
    static int init = 0;
    if (init)
	return;
    unsigned int i;
    memset(charclasses, LETTER, sizeof(charclasses));

    char digits[] = "0123456789";
    for (i = 0; i  < sizeof(digits); i++)
	charclasses[int(digits[i])] = DIGIT;

    char blankspace[] = "\t\v\f ";
    for (i = 0; i < sizeof(blankspace); i++)
	charclasses[int(blankspace[i])] = SPACE;

    char seps[] = "!\"$%&()/<=>[\\]^{|}~:;,*";
    for (i = 0; i  < sizeof(seps); i++)
	charclasses[int(seps[i])] = SPACE;

    char special[] = ".@+-,#'\n\r";
    for (i = 0; i  < sizeof(special); i++)
	charclasses[int(special[i])] = special[i];

    init = 1;
}

bool TextSplit::emitterm(string &w, int pos, bool doerase = true)
{
    if (!termsink)
	return false;

    // Maybe trim end of word. These are chars that we would keep inside 
    // a word or span, but not at the end
    while (w.length() > 0) {
	switch (w[w.length()-1]) {
	case '.':
	case ',':
	case '@':
	    w.erase(w.length()-1);
	    break;
	default:
	    goto breakloop;
	}
    }
 breakloop:
    if (w.length() > 0 && w.length() < (unsigned)maxWordLength) {
	bool ret = termsink(cdata, w, pos);
	if (doerase)
	    w.erase();
	return ret;
    }
    return true;
}

/* 
 * We basically emit a word every time we see a separator, but some chars are
 * handled specially so that special cases, ie, c++ and dockes@okyz.com etc, 
 * are handled properly,
 */
bool TextSplit::text_to_words(const string &in)
{
    setcharclasses();
    string span;
    string word;
    bool number = false;
    int wordpos = 0;
    int spanpos = 0;

    for (unsigned int i = 0; i < in.length(); i++) {
	int c = in[i];
	int cc = charclasses[c]; 
	switch (cc) {
	case SPACE:
	SPACE:
	    if (word.length()) {
		if (span.length() != word.length()) {
		    if (!emitterm(span, spanpos)) 
			return false;
		}
		if (!emitterm(word, wordpos++))
		    return false;
		number = false;
	    }
	    spanpos = wordpos;
	    span.erase();
	    break;
	case '-':
	case '+':
	    if (word.length() == 0) {
		if (i < in.length() || charclasses[int(in[i+1])] == DIGIT) {
		    number = true;
		    word += c;
		    span += c;
		}
	    } else {
		if (span.length() != word.length()) {
		    if (!emitterm(span, spanpos, false))
			return false;
		}
		if (!emitterm(word, wordpos++))
		    return false;
		number = false;
		span += c;
	    }
	    break;
	case '\'':
	case '@':
	    if (word.length()) {
		if (span.length() != word.length()) {
		    if (!emitterm(span, spanpos, false))
			return false;
		}
		if (!emitterm(word, wordpos++))
		    return false;
		number = false;
	    } else
		word += c;
	    span += c;
	    break;
	case '.':
	    if (number) {
		word += c;
	    } else {
		if (word.length()) {
		    if (!emitterm(word, wordpos++))
			return false;
		    number = false;
		} else 
		    word += c;
	    }
	    span += c;
	    break;
	case '#': 
	    // Keep it only at end of word...
	    if (word.length() > 0 && 
		(i == in.length() -1 || charclasses[int(in[i+1])] == SPACE ||
		 in[i+1] == '\n' || in[i+1] == '\r')) {
		word += c;
		span += c;
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
	    word += (char)c;
	    span += (char)c;
	    break;
	}
    }
    if (word.length()) {
	if (span.length() != word.length())
	    if (!emitterm(span, spanpos))
		return false;
	return emitterm(word, wordpos);
    }
    return true;
}

#else  // TEST driver ->

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <iostream>

#include "textsplit.h"
#include "readfile.h"

using namespace std;

bool termsink(void *, const string &term, int pos)
{
    cout << pos << " " << term << endl;
    return true;
}


static string teststring = 
    "jfd@okyz.com "
    "Ceci. Est;Oui 1.24 n@d @net .net t@v@c c# c++ -10 o'brien l'ami "
    "a 134 +134 -14 -1.5 +1.5 1.54e10 a"
    "@^#$(#$(*)"
    "one\n\rtwo\nthree-\nfour"
    "[olala][ululu]"

;

int main(int argc, char **argv)
{
    TextSplit splitter(termsink, 0);
    if (argc == 2) {
	string data;
	if (!file_to_string(argv[1], data)) 
	    exit(1);
	splitter.text_to_words(data);
    } else {
	cout << teststring << endl;  
	splitter.text_to_words(teststring);
    }
    
}
#endif // TEST
