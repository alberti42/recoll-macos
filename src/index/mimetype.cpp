#ifndef lint
static char rcsid[] = "@(#$Id: mimetype.cpp,v 1.12 2005-11-21 17:18:58 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#ifndef TEST_MIMETYPE
#include <ctype.h>
#include <string>
#include <list>

using std::string;
using std::list;

#include "mimetype.h"
#include "debuglog.h"
#include "execmd.h"
#include "rclconfig.h"
#include "smallut.h"
#include "idfile.h"

// Solaris8's 'file' command doesnt understand -i
#ifndef sun
#define USE_SYSTEM_FILE_COMMAND
#endif

/// Identification of file from contents. This is called for files with
/// unrecognized extensions (none, or not known either for indexing or
/// stop list)
///
/// The system 'file' utility is not that great for us. For exemple it
/// will mistake mail folders for simple text files if there is no
/// 'Received' header, which would be the case, for exemple in a 'Sent'
/// folder. Also "file -i" does not exist on all systems, and it's
/// quite costly.  
/// So we first call the internal file identifier, which currently
/// only knows about mail, but in which we can add the more
/// current/interesting file types.
/// As a last resort we execute 'file'

static string mimetypefromdata(const string &fn, bool usfc)
{
    string mime;

    // In any case first try the internal identifier
    mime = idFile(fn.c_str());

#ifdef USE_SYSTEM_FILE_COMMAND
    if (usfc && mime == "") {
	// Last resort: use "file -i"
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

	// The result of 'file' execution begins with the file name
	// which may contain spaces. We happen to know its size, so
	// strip it:
	result = result.substr(fn.size());
	// Now looks like ": text/plain; charset=us-ascii"
	// Split it, and take second field
	list<string> res;
	ConfTree::stringToStrings(result, res);
	if (res.size() <= 1) 
	    return "";
	list<string>::iterator it = res.begin();
	it++;
	mime = *it;
	// Remove possible punctuation at the end
	if (mime.length() > 0 && !isalpha(mime[mime.length() - 1]))
	    mime.erase(mime.length() -1);
    }
#endif

    return mime;
}

/// Guess mime type, first from suffix, then from file data. We also
/// have a list of suffixes that we don't touch at all (ie: .jpg,
/// etc...)
string mimetype(const string &fn, RclConfig *cfg, bool usfc)
{
    if (cfg == 0)
	return "";

    list<string> stoplist;
    cfg->getStopSuffixes(stoplist);
    if (!stoplist.empty()) {
	for (list<string>::const_iterator it = stoplist.begin();
	     it != stoplist.end(); it++) {
	    if (it->length() > fn.length())
		continue;
	    if (!stringicmp(fn.substr(fn.length() - it->length(),
				      string::npos), *it)) {
		LOGDEB(("mimetype: fn %s in stoplist (%s)\n", fn.c_str(), 
			it->c_str()));
		return "";
	    }
	}
    }

    // Look for suffix in mimetype map
    string::size_type dot = fn.find_last_of(".");
    string suff;
    if (dot != string::npos) {
        suff = fn.substr(dot);
	for (unsigned int i = 0; i < suff.length(); i++)
	    suff[i] = tolower(suff[i]);

	string mtype = cfg->getMimeTypeFromSuffix(suff);
	if (!mtype.empty())
	    return mtype;
    }

    return mimetypefromdata(fn, usfc);
}



#else // TEST->

#include <iostream>

#include "debuglog.h"
#include "rclconfig.h"
#include "rclinit.h"
#include "mimetype.h"

using namespace std;
int main(int argc, const char **argv)
{
    string reason;
    RclConfig *config = recollinit(0, 0, reason);

    if (config == 0 || !config->ok()) {
	string str = "Configuration problem: ";
	str += reason;
	fprintf(stderr, "%s\n", str.c_str());
	exit(1);
    }

    while (--argc > 0) {
	string filename = *++argv;
	cout << filename << " -> " << 
	    mimetype(filename, config, true) << endl;

    }
    return 0;
}


#endif // TEST
