#ifndef lint
static char rcsid[] = "@(#$Id: textsplit.cpp,v 1.1 2004-12-13 15:42:16 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <iostream>
#include <string>

using namespace std;

// Character classes: we have three main groups, and then some chars
// are their own class because they want special handling.
enum CharClass {LETTER=256, SPACE=257, DIGIT=258};
static int charclasses[256];
static void setcharclasses()
{
    static int init = 0;
    if (init)
	return;
    int i;
    memset(charclasses, LETTER, sizeof(charclasses));

    char digits[] = "0123456789";
    for (i = 0; i  < sizeof(digits); i++)
	charclasses[digits[i]] = DIGIT;

    char blankspace[] = "\t\v\f ";
    for (i = 0; i < sizeof(blankspace); i++)
	charclasses[blankspace[i]] = SPACE;

    char seps[] = "!\"$%&()/<=>[\\]^{|}~:;,*";
    for (i = 0; i  < sizeof(seps); i++)
	charclasses[seps[i]] = SPACE;

    char special[] = ".@+-,#'\n\r";
    for (i = 0; i  < sizeof(special); i++)
	charclasses[special[i]] = special[i];

    init = 1;
}

static void emitterm(string &w, int *posp, bool doerase = true)
{
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
    if (w.length()) {
	if (posp)
	    *posp++;
	cout << w << endl;
    }
    if (doerase)
	w.erase();
}

void text_to_words(const string &in)
{
    setcharclasses();
    string span;
    string word;
    bool number = false;
    int pos = 0;
    int spanpos = 0;
    for (int i = 0; i < in.length(); i++) {
	int c = in[i];
	int cc = charclasses[c]; 
	switch (cc) {
	case SPACE:
	SPACE:
	    if (word.length()) {
		if (span.length() != word.length())
		    emitterm(span, &spanpos);
		emitterm(word, &pos);
		number = false;
	    }
	    span.erase();
	    break;
	case '-':
	case '+':
	    if (word.length() == 0) {
		if (i < in.length() || charclasses[in[i+1]] == DIGIT) {
		    number = true;
		    word += c;
		    span += c;
		}
	    } else {
		if (span.length() != word.length())
		    emitterm(span, &spanpos, false);
		emitterm(word, &pos);
		number = false;
		span += c;
	    }
	    break;
	case '\'':
	case '@':
	    if (word.length()) {
		if (span.length() != word.length())
		    emitterm(span, &spanpos, false);
		emitterm(word, &pos);
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
		    emitterm(word, &pos);
		    number = false;
		} else 
		    word += c;
	    }
	    span += c;
	    break;
	case '#': 
	    // Keep it only at end of word...
	    if (word.length() > 0 && 
		(i == in.length() -1 || charclasses[in[i+1]] == SPACE)) {
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
		// the - was added as part of the sleep or was really there, 
		// but this would need a dictionary.
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
	    emitterm(span, &spanpos);
	emitterm(word, &pos);
    }
}

#if 1 || TEST_TEXTSPLIT
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
int
file_to_string(const string &fn, string &data)
{
    int fd = open(fn.c_str(), 0);
    if (fd < 0) {
	perror("open");
	return -1;
    }
    char buf[4096];
    for (;;) {
	int n = read(fd, buf, 4096);
	if (n < 0) {
	    perror("read");
	    close(fd);
	    return -1;
	}
	if (n == 0)
	    break;
	data.append(buf, n);
    }
    close(fd);
    return 0;
}

static string teststring = 
    "jfd@okyz.com "
    "Ceci. Est;Oui 1.24 n@d @net .net t@v@c c# c++ -10 o'brien l'ami "
    "@^#$(#$(*)"
    "one\n\rtwo\nthree-\nfour"
    "[olala][ululu]"

;

int main(int argc, char **argv)
{
    if (argc == 2) {
	string data;
	if (file_to_string(argv[1], data) < 0) 
	    exit(1);
	text_to_words(data);
    } else {
	cout << teststring << endl;  text_to_words(teststring);
    }
    
}
#endif // TEST

