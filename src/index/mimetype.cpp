#ifndef lint
static char rcsid[] = "@(#$Id: mimetype.cpp,v 1.3 2004-12-15 15:00:37 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <ctype.h>

#include <string>
using std::string;

#include "mimetype.h"

string mimetype(const string &filename, ConfTree *mtypes)
{
    if (mtypes == 0)
	return "";

    // If filename has a suffix and we find it in the map, we're done
    string::size_type dot = filename.find_last_of(".");
    if (dot != string::npos) {
	string suff = filename.substr(dot);
	for (unsigned int i = 0; i < suff.length(); i++)
	    suff[i] = tolower(suff[i]);

	string mtype;
	if (mtypes->get(suff, mtype, ""))
	    return mtype;
    }
    // Look at file data
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
