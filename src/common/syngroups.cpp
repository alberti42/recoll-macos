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

// Note that we are storing each term twice. I don't think that the
// size could possibly be a serious issue, but if it was, we could
// reduce the storage a bit by storing (short hash)-> vector<int>
// correspondances in the direct map, and then checking all the
// resulting groups for the input word.
//
// As it is, a word can only index one group (the last it is found
// in). It can be part of several groups though (appear in
// expansions). I really don't know what we should do with multiple
// groups anyway
class SynGroups::Internal {
public:
    Internal() : ok(false) {
    }
    bool ok;
    // Term to group num 
    STD_UNORDERED_MAP<string, unsigned int> terms;
    // Group num to group
    vector<vector<string> > groups;
};

bool SynGroups::ok() 
{
    return m && m->ok;
}

SynGroups::~SynGroups()
{
    delete m;
}

SynGroups::SynGroups()
    : m(new Internal)
{
}

bool SynGroups::setfile(const string& fn)
{
    LOGDEB(("SynGroups::setfile(%s)\n", fn.c_str()));
    if (!m) {
        m = new Internal;
        if (!m) {
            LOGERR(("SynGroups:setfile:: new Internal failed: no mem ?\n"));
            return false;
        }
    }

    if (fn.empty()) {
        delete m;
        m = 0;
	return true;
    }

    ifstream input;
    input.open(fn.c_str(), ios::in);
    if (!input.is_open()) {
	LOGERR(("SynGroups:setfile:: could not open %s errno %d\n",
		fn.c_str(), errno));
	return false;
    }	    

    string cline;
    bool appending = false;
    string line;
    bool eof = false;
    int lnum = 0;

    for (;;) {
        cline.clear();
	getline(input, cline);
	if (!input.good()) {
	    if (input.bad()) {
                LOGERR(("Syngroup::setfile(%s):Parse: input.bad()\n",
                        fn.c_str()));
		return false;
	    }
	    // Must be eof ? But maybe we have a partial line which
	    // must be processed. This happens if the last line before
	    // eof ends with a backslash, or there is no final \n
            eof = true;
	}
	lnum++;

        {
            string::size_type pos = cline.find_last_not_of("\n\r");
            if (pos == string::npos) {
                cline.clear();
            } else if (pos != cline.length()-1) {
                cline.erase(pos+1);
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
	    LOGERR(("SynGroups:setfile: %s: bad line %d: %s\n",
		    fn.c_str(), lnum, line.c_str()));
	    continue;
	}

	if (words.empty())
	    continue;
	if (words.size() == 1) {
	    LOGERR(("Syngroup::setfile(%s):single term group at line %d ??\n",
		    fn.c_str(), lnum));
	    continue;
	}

	m->groups.push_back(words);
	for (vector<string>::const_iterator it = words.begin();
	     it != words.end(); it++) {
	    m->terms[*it] = m->groups.size()-1;
	}
	LOGDEB1(("SynGroups::setfile: group: [%s]\n", 
		stringsToString(m->groups.back()).c_str()));
    }
    m->ok = true;
    return true;
}

vector<string> SynGroups::getgroup(const string& term)
{
    vector<string> ret;
    if (!ok())
	return ret;

    STD_UNORDERED_MAP<string, unsigned int>::const_iterator it1 =
        m->terms.find(term);
    if (it1 == m->terms.end()) {
	LOGDEB1(("SynGroups::getgroup: [%s] not found in direct map\n", 
		 term.c_str()));
	return ret;
    }

    unsigned int idx = it1->second;
    if (idx >= m->groups.size()) {
        LOGERR(("SynGroups::getgroup: line index higher than line count !\n"));
        return ret;
    }
    return m->groups[idx];
}

#else

#include "syngroups.h"
#include "debuglog.h"

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

    DebugLog::getdbl()->setloglevel(DEBDEB1);
    DebugLog::setfilename("stderr");
    SynGroups syns;
    syns.setfile(fn);
    if (!syns.ok()) {
	cerr << "Initialization failed\n";
	return 1;
    }

    vector<string> group = syns.getgroup(word);
    cout << group.size() << " terms in group\n";
    for (vector<string>::const_iterator it = group.begin();
	 it != group.end(); it++) {
	cout << "[" << *it << "] ";
    }
    cout << endl;
    return 0;
}

#endif
