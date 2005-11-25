#ifndef lint
static char rcsid[] = "@(#$Id: smallut.cpp,v 1.9 2005-11-25 08:50:39 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
#ifndef TEST_SMALLUT
#include <string>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "smallut.h"
#include "debuglog.h"
#include "pathut.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

#define MIN(A,B) ((A)<(B)?(A):(B))

bool maketmpdir(string& tdir)
{
    const char *tmpdir = getenv("RECOLL_TMPDIR");
    if (!tmpdir)
	tmpdir = getenv("TMPDIR");
    if (!tmpdir)
	tmpdir = "/tmp";
    tdir = tmpdir;
    path_cat(tdir, "rcltmpXXXXXX");
    {
	char *cp = strdup(tdir.c_str());
	if (!cp) {
	    LOGERR(("maketmpdir: out of memory (for file name !)\n"));
	    tdir.erase();
	    return false;
	}
#ifdef HAVE_MKDTEMP
	if (!mkdtemp(cp)) {
#else
	if (!mktemp(cp)) {
#endif // HAVE_MKDTEMP
	    free(cp);
	    LOGERR(("maketmpdir: mktemp failed\n"));
	    tdir.erase();
	    return false;
	}	
	tdir = cp;
	free(cp);
    }
#ifndef HAVE_MKDTEMP
    if (mkdir(tdir.c_str(), 0700) < 0) {
	LOGERR(("maketmpdir: mkdir %s failed\n", tdir.c_str()));
	tdir.erase();
	return false;
    }
#endif
    return true;
}

string stringlistdisp(const list<string>& sl)
{
    string s;
    for (list<string>::const_iterator it = sl.begin(); it!= sl.end(); it++)
	s += "[" + *it + "] ";
    if (!s.empty())
	s.erase(s.length()-1);
    return s;
}
	    

int stringicmp(const string & s1, const string& s2) 
{
    string::const_iterator it1 = s1.begin();
    string::const_iterator it2 = s2.begin();
    int size1 = s1.length(), size2 = s2.length();
    char c1, c2;

    if (size1 > size2) {
	while (it1 != s1.end()) { 
	    c1 = ::toupper(*it1);
	    c2 = ::toupper(*it2);
	    if (c1 != c2) {
		return c1 > c2 ? 1 : -1;
	    }
	    ++it1; ++it2;
	}
	return size1 == size2 ? 0 : 1;
    } else {
	while (it2 != s2.end()) { 
	    c1 = ::toupper(*it1);
	    c2 = ::toupper(*it2);
	    if (c1 != c2) {
		return c1 > c2 ? 1 : -1;
	    }
	    ++it1; ++it2;
	}
	return size1 == size2 ? 0 : -1;
    }
}

//  s1 is already lowercase
int stringlowercmp(const string & s1, const string& s2) 
{
    string::const_iterator it1 = s1.begin();
    string::const_iterator it2 = s2.begin();
    int size1 = s1.length(), size2 = s2.length();
    char c2;

    if (size1 > size2) {
	while (it1 != s1.end()) { 
	    c2 = ::tolower(*it2);
	    if (*it1 != c2) {
		return *it1 > c2 ? 1 : -1;
	    }
	    ++it1; ++it2;
	}
	return size1 == size2 ? 0 : 1;
    } else {
	while (it2 != s2.end()) { 
	    c2 = ::tolower(*it2);
	    if (*it1 != c2) {
		return *it1 > c2 ? 1 : -1;
	    }
	    ++it1; ++it2;
	}
	return size1 == size2 ? 0 : -1;
    }
}

//  s1 is already uppercase
int stringuppercmp(const string & s1, const string& s2) 
{
    string::const_iterator it1 = s1.begin();
    string::const_iterator it2 = s2.begin();
    int size1 = s1.length(), size2 = s2.length();
    char c2;

    if (size1 > size2) {
	while (it1 != s1.end()) { 
	    c2 = ::toupper(*it2);
	    if (*it1 != c2) {
		return *it1 > c2 ? 1 : -1;
	    }
	    ++it1; ++it2;
	}
	return size1 == size2 ? 0 : 1;
    } else {
	while (it2 != s2.end()) { 
	    c2 = ::toupper(*it2);
	    if (*it1 != c2) {
		return *it1 > c2 ? 1 : -1;
	    }
	    ++it1; ++it2;
	}
	return size1 == size2 ? 0 : -1;
    }
}

// Compare charset names, removing the more common spelling variations
bool samecharset(const string &cs1, const string &cs2)
{
    string mcs1, mcs2;
    // Remove all - and _, turn to lowecase
    for (unsigned int i = 0; i < cs1.length();i++) {
	if (cs1[i] != '_' && cs1[i] != '-') {
	    mcs1 += ::tolower(cs1[i]);
	}
    }
    for (unsigned int i = 0; i < cs2.length();i++) {
	if (cs2[i] != '_' && cs2[i] != '-') {
	    mcs2 += ::tolower(cs2[i]);
	}
    }
    return mcs1 == mcs2;
}


bool stringToStrings(const string &s, std::list<string> &tokens)
{
    string current;
    tokens.clear();
    enum states {SPACE, TOKEN, INQUOTE, ESCAPE};
    states state = SPACE;
    for (unsigned int i = 0; i < s.length(); i++) {
	switch (s[i]) {
	    case '"': 
	    switch(state) {
	      case SPACE: 
		state=INQUOTE; continue;
	      case TOKEN: 
	        current += '"';
		continue;
	      case INQUOTE: 
		tokens.push_back(current);
		current = "";
		state = SPACE;
		continue;
	      case ESCAPE:
	        current += '"';
	        state = INQUOTE;
	      continue;
	    }
	    break;
	    case '\\': 
	    switch(state) {
	      case SPACE: 
	      case TOKEN: 
		  current += '\\';
		  state=TOKEN; 
		  continue;
	      case INQUOTE: 
		  state = ESCAPE;
		  continue;
	      case ESCAPE:
		  current += '\\';
		  state = INQUOTE;
		  continue;
	    }
	    break;

	    case ' ': 
	    case '\t': 
	    switch(state) {
	      case SPACE: 
		  continue;
	      case TOKEN: 
		tokens.push_back(current);
		current = "";
		state = SPACE;
		continue;
	      case INQUOTE: 
	      case ESCAPE:
		  current += s[i];
		  continue;
	    }
	    break;

	    default:
	    switch(state) {
	      case ESCAPE:
		  state = INQUOTE;
		  break;
	      case SPACE: 
		  state = TOKEN;
		  break;
	      case TOKEN: 
	      case INQUOTE: 
		  break;
	    }
	    current += s[i];
	}
    }
    switch(state) {
    case SPACE: 
	break;
    case TOKEN: 
	tokens.push_back(current);
	break;
    case INQUOTE: 
    case ESCAPE:
	return false;
    }
    return true;
}

bool stringToBool(const string &s)
{
    if (s.empty())
	return false;
    if (isdigit(s[0])) {
	int val = atoi(s.c_str());
	return val ? true : false;
    }
    if (strchr("yYoOtT", s[0]))
	return true;
    return false;
}

#else

#include <string>
#include "smallut.h"

struct spair {
    const char *s1;
    const char *s2;
};
struct spair pairs[] = {
    {"", ""},
    {"", "a"},
    {"a", ""},
    {"a", "a"},
    {"A", "a"},
    {"a", "A"},
    {"A", "A"},
    {"12", "12"},
    {"a", "ab"},
    {"ab", "a"},
    {"A", "Ab"},
    {"a", "Ab"},
};
int npairs = sizeof(pairs) / sizeof(struct spair);

int main(int argc, char **argv)
{
    for (int i = 0; i < npairs; i++) {
	{
	    int c = stringicmp(pairs[i].s1, pairs[i].s2);
	    printf("'%s' %s '%s' ", pairs[i].s1, 
		   c == 0 ? "==" : c < 0 ? "<" : ">", pairs[i].s2);
	}
	{
	    int cl = stringlowercmp(pairs[i].s1, pairs[i].s2);
	    printf("L '%s' %s '%s' ", pairs[i].s1, 
		   cl == 0 ? "==" : cl < 0 ? "<" : ">", pairs[i].s2);
	}
	{
	    int cu = stringuppercmp(pairs[i].s1, pairs[i].s2);
	    printf("U '%s' %s '%s' ", pairs[i].s1, 
		   cu == 0 ? "==" : cu < 0 ? "<" : ">", pairs[i].s2);
	}
	printf("\n");
    }
}

#endif
