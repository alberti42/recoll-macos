/* Copyright (C) 2014 J.F.Dockes
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
#ifndef TEST_SYNGROUPS
#include "autoconfig.h"

#include "syngroups.h"

#include "debuglog.h"
#include "smallut.h"

#include <errno.h>
#include UNORDERED_MAP_INCLUDE
#include <fstream>
#include <iostream>
#include <cstring>

using namespace std;

class SynGroups::Internal {
public:
    Internal() : ok(false) {
    }
    bool ok;
    // Term to group num 
    STD_UNORDERED_MAP<string, int> terms;
    // Group num to group
    STD_UNORDERED_MAP<int, vector<string> > groups;
};
bool SynGroups::ok() 
{
    return m && m->ok;
}
SynGroups::~SynGroups()
{
    delete m;
}

const int LL = 1024;

SynGroups::SynGroups(const string& fn)
    : m(new Internal)
{
    if (!m) {
	LOGERR(("SynGroups::SynGroups:: new Internal failed: no mem ?\n"));
	return;
    }

    ifstream input;
    input.open(fn.c_str(), ios::in);
    if (!input.is_open()) {
	LOGERR(("SynGroups::SynGroups:: could not open %s errno %d\n",
		fn.c_str(), errno));
	return;
    }	    

    char cline[LL];
    bool appending = false;
    string line;
    bool eof = false;
    int lnum = 0;

    for (;;) {
        cline[0] = 0;
	input.getline(cline, LL-1);
	if (!input.good()) {
	    if (input.bad()) {
                LOGDEB(("Parse: input.bad()\n"));
		return;
	    }
	    // Must be eof ? But maybe we have a partial line which
	    // must be processed. This happens if the last line before
	    // eof ends with a backslash, or there is no final \n
            eof = true;
	}
	lnum++;

        {
            int ll = strlen(cline);
            while (ll > 0 && (cline[ll-1] == '\n' || cline[ll-1] == '\r')) {
                cline[ll-1] = 0;
                ll--;
            }
        }

	if (appending)
	    line += cline;
	else
	    line = cline;

	// Note that we trim whitespace before checking for backslash-eol
	// This avoids invisible whitespace problems.
	trimstring(line);
	if (line.empty() || line.at(0) == '#') {
            if (eof)
                break;
	    continue;
	}
	if (line[line.length() - 1] == '\\') {
	    line.erase(line.length() - 1);
	    appending = true;
	    continue;
	}
	appending = false;

	vector<string> words;
	if (!stringToStrings(line, words)) {
	    LOGERR(("SynGroups::SynGroups: %s: bad line %d: %s\n",
		    fn.c_str(), lnum, line.c_str()));
	    continue;
	}

	if (words.empty())
	    continue;
	if (words.size() == 1) {
	    LOGDEB(("SynGroups::SynGroups: single term group at line %d ??\n",
		    lnum));
	    continue;
	}

	m->groups[lnum] = words;
	for (vector<string>::const_iterator it = words.begin();
	     it != words.end(); it++) {
	    m->terms[*it] = lnum;
	}
    }
    m->ok = true;
}

vector<string> SynGroups::getgroup(const string& term)
{
    vector<string> ret;
    if (!ok())
	return ret;

    STD_UNORDERED_MAP<string, int>::const_iterator it1 = m->terms.find(term);
    if (it1 == m->terms.end()) {
	LOGDEB1(("SynGroups::getgroup: [%s] not found in direct map\n", 
		 term.c_str()));
	return ret;
    }

    // Group num to group
    STD_UNORDERED_MAP<int, vector<string> >::const_iterator it2 = 
	m->groups.find(it1->second);
    
    if (it2 == m->groups.end()) {
	LOGERR(("SynGroups::getgroup: %s found in direct map but no group\n",
		term.c_str()));
	return ret;
    }
    return it2->second;
}

#else

#include "syngroups.h"

#include <string>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstdio>

using namespace std;

static char *thisprog;

static char usage [] =
    "syngroups <synfilename> <word>\n"
    "  \n\n"
    ;
static void Usage(void)
{
    fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_s     0x2
#define OPT_b     0x4

int main(int argc, char **argv)
{
    thisprog = argv[0];
    argc--; argv++;

    if (argc != 2) {
        Usage();
    }
    string fn = *argv++;argc--;
    string word = *argv++;argc--;

    SynGroups syns(fn);
    if (!syns.ok()) {
	cerr << "Initialization failed\n";
	return 1;
    }

    vector<string> group = syns.getgroup(word);
    cout << group.size() << " terms in group\n";
    for (vector<string>::const_iterator it = group.begin();
	 it != group.end(); it++) {
	cout << *it << " ";
    }
    cout << endl;
    return 0;
}

#endif
