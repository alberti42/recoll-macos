#ifndef lint
static char rcsid[] = "@(#$Id: history.cpp,v 1.8 2007-06-20 13:16:11 dockes Exp $ (C) 2005 J.F.Dockes";
#endif
/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef TEST_HISTORY
#include <stdio.h>
#include <time.h>

#include "history.h"
#include "base64.h"
#include "smallut.h"
#include "debuglog.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif


// Encode/decode document history entry: Unix time + base64 of fn +
// base64 of ipath separated by a space. If ipath is not set, there
// are only 2 parts
bool RclDHistoryEntry::encode(string& value)
{
    char chartime[20];
    sprintf(chartime, "%ld", unixtime);
    string bfn, bipath;
    base64_encode(fn, bfn);
    base64_encode(ipath, bipath);
    value = string(chartime) + " " + bfn + " " + bipath;
    return true;
}
bool RclDHistoryEntry::decode(const string &value)
{
    list<string> vall;
    stringToStrings(value, vall);
    list<string>::const_iterator it1 = vall.begin();
    if (vall.size() < 2)
	return false;
    unixtime = atol((*it1++).c_str());
    base64_decode(*it1++, fn);
    if (vall.size() == 3)
	base64_decode(*it1, ipath);
    else
	ipath.erase();
    return true;
}
bool RclDHistoryEntry::equal(const HistoryEntry& other)
{
    const RclDHistoryEntry& e = dynamic_cast<const RclDHistoryEntry&>(other);
    return e.fn == fn && e.ipath == ipath;
}


// Encode/decode simple string. base64 used to avoid problems with
// strange chars
bool RclSListEntry::encode(string& enc)
{
    base64_encode(value, enc);
    return true;
}
bool RclSListEntry::decode(const string &enc)
{
    base64_decode(enc, value);
    return true;
}
bool RclSListEntry::equal(const HistoryEntry& other)
{
    const RclSListEntry& e = dynamic_cast<const RclSListEntry&>(other);
    return e.value == value;
}

bool RclHistory::insertNew(const string &sk, HistoryEntry &n, HistoryEntry &s)
{
    // Is this doc already in history ? If it is we remove the old entry
    list<string> names = m_data.getNames(sk);
    list<string>::const_iterator it;
    bool changed = false;
    for (it = names.begin(); it != names.end(); it++) {
	string oval;
	if (!m_data.get(*it, oval, sk)) {
	    LOGDEB(("No data for %s\n", (*it).c_str()));
	    continue;
	}
	s.decode(oval);

	if (s.equal(n)) {
	    LOGDEB(("Erasing old entry\n"));
	    m_data.erase(*it, sk);
	    changed = true;
	}
    }

    // Maybe reget list
    if (changed)
	names = m_data.getNames(sk);

    // How many do we have
    if (names.size() >= m_mlen) {
	// Need to erase entries until we're back to size. Note that
	// we don't ever reset numbers. Problems will arise when
	// history is 4 billion entries old
	it = names.begin();
	for (unsigned int i = 0; i < names.size() - m_mlen + 1; i++, it++) {
	    m_data.erase(*it, sk);
	}
    }

    // Increment highest number
    unsigned int hi = names.empty() ? 0 : 
	(unsigned int)atoi(names.back().c_str());
    hi++;
    char nname[20];
    sprintf(nname, "%010u", hi);

    string value;
    n.encode(value);
    LOGDEB1(("Encoded value [%s] (%d)\n", value.c_str(), value.size()));
    if (!m_data.set(string(nname), value, sk)) {
	LOGERR(("RclDHistory::insertNew: set failed\n"));
	return false;
    }
    return true;

}

bool RclHistory::eraseAll(const string &sk)
{
    // Is this doc already in history ? If it is we remove the old entry
    list<string> names = m_data.getNames(sk);
    list<string>::const_iterator it;
    for (it = names.begin(); it != names.end(); it++) {
	    m_data.erase(*it, sk);
    }
    return true;
}
bool RclHistory::truncate(const string &sk, unsigned int n)
{
    // Is this doc already in history ? If it is we remove the old entry
    list<string> names = m_data.getNames(sk);
    if (names.size() <= n)
	return true;
    unsigned int i = 0;
    for (list<string>::const_iterator it = names.begin(); 
	 it != names.end(); it++, i++) {
	if (i >= n)
	    m_data.erase(*it, sk);
    }
    return true;
}


bool RclHistory::enterString(const string sk, const string value)
{
    RclSListEntry ne(value);
    RclSListEntry scratch;
    return insertNew(sk, ne, scratch);
}
list<string> RclHistory::getStringList(const string sk) 
{
    list<RclSListEntry> el = getHistory<RclSListEntry>(sk);
    list<string> sl;
    for (list<RclSListEntry>::const_iterator it = el.begin(); 
	 it != el.end(); it++) 
	sl.push_back(it->value);
    return sl;
}

string RclHistory::docSubkey = "docs";

/// *************** History entries specific methods
bool RclHistory::enterDoc(const string fn, const string ipath)
{
    LOGDEB(("RclDHistory::enterDoc: [%s] [%s] into %s\n", 
	    fn.c_str(), ipath.c_str(), m_data.getFilename().c_str()));
    RclDHistoryEntry ne(time(0), fn, ipath);
    RclDHistoryEntry scratch;
    return insertNew(docSubkey, ne, scratch);
}

list<RclDHistoryEntry> RclHistory::getDocHistory()
{
    return getHistory<RclDHistoryEntry>(docSubkey);
}



#else
#include <string>
#include <iostream>

#include "history.h"
#include "debuglog.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif

static string thisprog;

static string usage =
    "trhist [opts] <filename>\n"
    " [-s <subkey>]: specify subkey (default: RclHistory::docSubkey)\n"
    " [-e] : erase all\n"
    " [-a <string>] enter string (needs -s, no good for history entries\n"
    "\n"
    ;

static void
Usage(void)
{
    cerr << thisprog  << ": usage:\n" << usage;
    exit(1);
}

static int        op_flags;
#define OPT_e     0x2
#define OPT_s     0x4
#define OPT_a     0x8

int main(int argc, char **argv)
{
    string sk = RclHistory::docSubkey;
    string value;

    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
	(*argv)++;
	if (!(**argv))
	    /* Cas du "adb - core" */
	    Usage();
	while (**argv)
	    switch (*(*argv)++) {
	    case 'a':	op_flags |= OPT_a; if (argc < 2)  Usage();
		value = *(++argv); argc--; 
		goto b1;
	    case 's':	op_flags |= OPT_s; if (argc < 2)  Usage();
		sk = *(++argv);	argc--; 
		goto b1;
	    case 'e':	op_flags |= OPT_e; break;
	    default: Usage();	break;
	    }
    b1: argc--; argv++;
    }
    if (argc != 1)
	Usage();
    string filename = *argv++;argc--;

    RclHistory hist(filename, 5);
    DebugLog::getdbl()->setloglevel(DEBDEB1);
    DebugLog::setfilename("stderr");

    if (op_flags & OPT_e) {
	hist.eraseAll(sk);
    } else if (op_flags & OPT_a) {
	if (!(op_flags & OPT_s)) 
	    Usage();
	hist.enterString(sk, value);
    } else {
	for (int i = 0; i < 10; i++) {
	    char docname[100];
	    sprintf(docname, "A very long document document name"
		    "is very long indeed and this is the end of "
		    "it here and exactly here:\n%d",	i);
	    hist.enterDoc(string(docname), "ipathx");
	}

	list<RclDHistoryEntry> hlist = hist.getDocHistory();
	for (list<RclDHistoryEntry>::const_iterator it = hlist.begin();
	     it != hlist.end(); it++) {
	    printf("[%ld] [%s] [%s]\n", it->unixtime, 
		   it->fn.c_str(), it->ipath.c_str());
	}
    }
}

#endif
