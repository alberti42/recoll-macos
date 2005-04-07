#ifndef lint
static char rcsid[] = "@(#$Id: mimetype.cpp,v 1.9 2005-04-07 09:05:39 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <ctype.h>

#include <string>
using std::string;
#include <list>
using std::list;

#include "mimetype.h"
#include "debuglog.h"
#include "execmd.h"
#include "conftree.h"
#include "smallut.h"
#include "idfile.h"

// The system 'file' utility is not that great for us. For exemple it
// will mistake mail folders for simple text files if there is no
// 'Received' header, which would be the case, for exemple in a 'Sent'
// folder. Also "file -i" does not exist on all systems
static string mimetypefromdata(const string &fn)
{
    string mime;
#ifdef USE_SYSTEM_FILE_UTILITY
    list<string> args;

    args.push_back("-i");
    args.push_back(fn);
    ExecCmd ex;
    string result;
    string cmd = "file";
    int status = ex.doexec(cmd, args, 0, &result);
    if (status) {
	LOGERR(("mimetypefromdata: doexec: status 0x%x\n", status));
	return "";
    }
    // LOGDEB(("mimetypefromdata: %s [%s]\n", result.c_str(), fn.c_str()));
    list<string> res;
    ConfTree::stringToStrings(result, res);
    if (res.size() <= 1) 
	return "";
    list<string>::iterator it = res.begin();
    it++;
    mime = *it;

    if (mime.length() > 0 && !isalpha(mime[mime.length() - 1]))
	mime.erase(mime.length() -1);
#else 
    mime = idFile(fn.c_str());
#endif
    return mime;
}

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
	    if (!stringicmp(fn.substr(fn.length() - it->length(),string::npos),
			    *it)) {
		LOGDEB(("mimetype: fn %s in stoplist (%s)\n", fn.c_str(), 
			it->c_str()));
		return "";
	    }
	}
    }

    // If the file name has a suffix and we find it in the map, we're done
    string::size_type dot = fn.find_last_of(".");
    string suff;
    if (dot != string::npos) {
        suff = fn.substr(dot);
	for (unsigned int i = 0; i < suff.length(); i++)
	    suff[i] = tolower(suff[i]);

	string mtype;
	if (mtypes->get(suff, mtype, ""))
	    return mtype;
    }

    // Look at file data ? Only when no suffix or always ?
    //if (suff.empty()) // causes problems with shifted files, like
		      // messages.1, messages.2 etc...
    return mimetypefromdata(fn);

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
