#ifndef lint
static char rcsid[] = "@(#$Id: mimetype.cpp,v 1.5 2005-02-09 13:34:08 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <ctype.h>

#include <string>
using std::string;

#include "mimetype.h"
#include "debuglog.h"

string mimetype(const string &fn, ConfTree *mtypes)
{
    if (mtypes == 0)
	return "";

    static list<string> stoplist;
    if (stoplist.empty()) {
	string stp;
	if (mtypes->get(string("recoll_noindex"), stp, "")) {
	    ConfTree::stringToStrings(stp, stoplist);
	}
    }

    if (!stoplist.empty()) {
	for (list<string>::const_iterator it = stoplist.begin();
	     it != stoplist.end(); it++) {
	    if (it->length() > fn.length())
		continue;
	    if (!fn.compare(fn.length() - it->length(), string::npos, *it)) {
		LOGDEB1(("mimetype: fn %s in stoplist (%s)\n", fn.c_str(), 
			 it->c_str()));
		return "";
	    }
	}
    }

    // If the file name has a suffix and we find it in the map, we're done
    string::size_type dot = fn.find_last_of(".");
    if (dot != string::npos) {
	string suff = fn.substr(dot);
	for (unsigned int i = 0; i < suff.length(); i++)
	    suff[i] = tolower(suff[i]);

	string mtype;
	if (mtypes->get(suff, mtype, ""))
	    return mtype;
    }

    // Look at file data ? One day maybe
    return "";
}



#ifdef _TEST_MIMETYPE_
#include <iostream>
const char *tvec[] = {
    "/toto/tutu",
    "/",
    "toto.txt",
    "toto.TXT",
    "toto.C.txt",
    "toto.C1",
    "",
};
const int n = sizeof(tvec) / sizeof(char*);
using namespace std;
int main(int argc, const char **argv)
{
    map<string, string>mtypes;
    mtypes[".txt"] = "text/plain";

    for (int i = 0; i < n; i++) {
	cout << tvec[i] << " -> " << mimetype(string(tvec[i]), mtypes) << endl;
    }
}
#endif
