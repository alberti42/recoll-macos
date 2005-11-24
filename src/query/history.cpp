#ifndef lint
static char rcsid[] = "@(#$Id: history.cpp,v 1.1 2005-11-24 18:21:55 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#ifndef TEST_HISTORY
#include <time.h>
#include "history.h"
#include "base64.h"
#include "smallut.h"
#include "debuglog.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif

RclQHistory::RclQHistory(const string &fn, unsigned int mxs)
    : m_mlen(mxs), m_data(fn.c_str())
{
    
}
const char *docSubkey = "docs";

bool RclQHistory::enterDocument(const string fn, const string ipath)
{
    LOGDEB(("RclQHistory::enterDocument: [%s] [%s] into %s\n", 
	    fn.c_str(), ipath.c_str(), m_data.getFilename().c_str()));
    // How many do we have
    list<string> names = m_data.getNames(docSubkey);
    list<string>::const_iterator it = names.begin();
    if (names.size() >= m_mlen) {
	// Need to erase entries until we're back to size. Note that
	// we don't ever erase anything. Problems will arise when
	// history is 2 billion entries old
	for (unsigned int i = 0; i < names.size() - m_mlen + 1; i++, it++) {
	    m_data.erase(*it, docSubkey);
	    it++;
	}
    }

    // Increment highest number
    int hi = names.empty() ? 0 : atoi(names.back().c_str());
    hi++;
    char nname[20];
    sprintf(nname, "%010d", hi);

    string bfn, bipath;
    base64_encode(fn, bfn);
    base64_encode(ipath, bipath);
    string value = bfn + " " + bipath;
    if (!m_data.set(string(nname), value, docSubkey)) {
	LOGERR(("RclQHistory::enterDocument: set failed\n"));
	return false;
    }
    return true;
}

list< pair<string, string> > RclQHistory::getDocHistory()
{
    list< pair<string, string> > mlist;
    list<string> names = m_data.getNames(docSubkey);
    for (list<string>::const_iterator it = names.begin(); it != names.end();
	 it++) {
	string value;
	if (m_data.get(*it, value, docSubkey)) {
	    list<string> vall;
	    stringToStrings(value, vall);
	    list<string>::const_iterator it1 = vall.begin();
	    if (vall.size() < 1) 
		continue;
	    string fn, ipath;
	    LOGDEB(("RclQHistory::getDocHistory:b64: %s\n", (*it1).c_str()));
	    base64_decode(*it1++, fn);
	    LOGDEB(("RclQHistory::getDocHistory:fn: %s\n", fn.c_str()));
	    if (vall.size() == 2)
		base64_decode(*it1, ipath);
	    mlist.push_back(pair<string, string>(fn, ipath));
	}
    }
    return mlist;
}


#else
#include <string>

#include "history.h"
#include "debuglog.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif

int main(int argc, char **argv)
{
    RclQHistory hist("toto", 5);
    DebugLog::getdbl()->setloglevel(DEBDEB1);
    DebugLog::setfilename("stderr");

    for (int i = 0; i < 10; i++) {
	char docname[100];
	sprintf(docname, "A very long document document name is very long indeed and this is the end of it here and exactly here:\n%d", i);
	hist.enterDocument(string(docname), "ipathx");
    }

    list< pair<string, string> > hlist = hist.getDocHistory();
    for (list< pair<string, string> >::const_iterator it = hlist.begin();
	 it != hlist.end(); it++) {
	printf("[%s] [%s]\n", it->first.c_str(), it->second.c_str());
    }
}

#endif
