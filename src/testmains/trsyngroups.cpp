/* Copyright (C) 2014-2019 J.F.Dockes
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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "syngroups.h"
#include "log.h"

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


int main(int argc, char **argv)
{
    thisprog = argv[0];
    argc--; argv++;

    if (argc != 2) {
        Usage();
    }
    string fn = *argv++;argc--;
    string word = *argv++;argc--;

    Logger::getTheLog("")->setloglevel(Logger::LLDEB0);
    SynGroups syns;
    syns.setfile(fn);
    if (!syns.ok()) {
        cerr << "Initialization failed\n";
        return 1;
    }

    vector<string> group = syns.getgroup(word);
    cout << group.size() << " terms in group\n";
    for (const auto& trm : group) {
        cout << "[" << trm << "] ";
    }
    cout << endl;
    return 0;
}
