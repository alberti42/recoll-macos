#ifndef lint
static char rcsid[] = "@(#$Id: smallut.cpp,v 1.1 2005-01-31 14:31:10 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#ifndef TEST_SMALLUT
#include <string>
#include "smallut.h"

#include <ctype.h>

#define MIN(A,B) ((A)<(B)?(A):(B))

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
