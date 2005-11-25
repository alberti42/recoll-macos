#ifndef lint
static char rcsid[] = "@(#$Id: history.cpp,v 1.2 2005-11-25 14:36:45 dockes Exp $ (C) 2005 J.F.Dockes";
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
    //Encode value part: 2 base64 of fn and ipath separated by a space
    string bfn, bipath;
    base64_encode(fn, bfn);
    base64_encode(ipath, bipath);
    string value = bfn + " " + bipath;

    // We trim whitespace from the value (if there is no ipath it now
    // ends with a space), else the value later returned by conftree
    // will be different because conftree does trim white space, and
    // this breaks the comparisons below
    trimstring(value);

    LOGDEB1(("Encoded value [%s] (%d)\n", value.c_str(), value.size()));
    // Is this doc already in history ? If it is we remove the old entry
    list<string> names = m_data.getNames(docSubkey);
    list<string>::const_iterator it;
    bool changed = false;
    for (it = names.begin();it != names.end(); it++) {
	string oval;
	if (!m_data.get(*it, oval, docSubkey)) {
	    LOGDEB(("No data for %s\n", (*it).c_str()));
	    continue;
	}
	LOGDEB1(("Look at %s [%s] (%d)\n", 
		(*it).c_str(), oval.c_str(), oval.length()));
	if (oval == value) {
	    LOGDEB1(("Erasing old entry\n"));
	    m_data.erase(*it, docSubkey);
	    changed = true;
	}
    }
    // Maybe reget list
    if (changed)
	names = m_data.getNames(docSubkey);

    // How many do we have
    if (names.size() >= m_mlen) {
	// Need to erase entries until we're back to size. Note that
	// we don't ever reset numbers. Problems will arise when
	// history is 4 billion entries old
	it = names.begin();
	for (unsigned int i = 0; i < names.size() - m_mlen + 1; i++, it++) {
	    m_data.erase(*it, docSubkey);
	}
    }

    // Increment highest number
    unsigned int hi = names.empty() ? 0 : 
	(unsigned int)atoi(names.back().c_str());
    hi++;
    char nname[20];
    sprintf(nname, "%010u", hi);

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
	    mlist.push_front(pair<string, string>(fn, ipath));
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
