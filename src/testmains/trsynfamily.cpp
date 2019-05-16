/* Copyright (C) 2012-2019 J.F.Dockes
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

#include "autoconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <strings.h>

#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "xapian.h"

#include "smallut.h"
#include "pathut.h"
#include "xmacros.h"
#include "synfamily.h"

static string thisprog;
static int        op_flags;
#define OPT_D     0x1
#define OPT_L     0x2
#define OPT_a     0x4
#define OPT_u     0x8
#define OPT_d     0x10
#define OPT_l     0x20
#define OPT_s     0x40
#define OPT_e     0x80

static string usage =
    " -d <dbdir> {-s|-a|-u} database dir and synfamily: stem accents/case ustem\n"
    " -l : list members\n"
    " -L <member>: list entries for given member\n"
    " -e <member> <key> : list expansion for given member and key\n"
    " -D <member>: delete member\n"
    "  \n\n"
    ;
static void Usage(void)
{
    cerr << thisprog << ": usage:\n" << usage;
    exit(1);
}

int main(int argc, char **argv)
{
    string dbdir(path_tildexpand("~/.recoll/xapiandb"));
    string outencoding = "UTF-8";
    string member;
    string key;

    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
        (*argv)++;
        if (!(**argv))
            /* Cas du "adb - core" */
            Usage();
        while (**argv)
            switch (*(*argv)++) {
            case 'a':   op_flags |= OPT_a; break;
            case 'D':   op_flags |= OPT_D; break;
            case 'd':   op_flags |= OPT_d; if (argc < 2)  Usage();
                dbdir = *(++argv); argc--; 
                goto b1;
            case 'e':   op_flags |= OPT_e; if (argc < 3)  Usage();
                member = *(++argv);argc--;
                key = *(++argv); argc--;
                goto b1;
            case 'l':   op_flags |= OPT_l; break;
            case 'L':   op_flags |= OPT_L; if (argc < 2)  Usage();
                member = *(++argv); argc--; 
                goto b1;
            case 's':   op_flags |= OPT_s; break;
            case 'u':   op_flags |= OPT_u; break;
            default: Usage();   break;
            }
    b1: argc--; argv++;
    }

    if (argc != 0)
        Usage();

    string familyname;
    if (op_flags & OPT_a) {
        familyname = Rcl::synFamDiCa;
    } else if (op_flags & OPT_u) {
        familyname = Rcl::synFamStemUnac;
    } else {
        familyname = Rcl::synFamStem;
    }
    if ((op_flags & (OPT_l|OPT_L|OPT_D|OPT_e)) == 0)
        Usage();

    string ermsg;
    try {
        if ((op_flags & (OPT_D)) == 0) { // Need write ?
            Xapian::Database db(dbdir);
            Rcl::XapSynFamily fam(db, familyname);
            if (op_flags & OPT_l) {
                vector<string> members;
                if (!fam.getMembers(members)) {
                    cerr << "getMembers error" << endl;
                    return 1;
                }
                string out;
                stringsToString(members, out);
                cout << "Family: " << familyname << " Members: " << out << endl;
            } else if (op_flags & OPT_L) {
                fam.listMap(member); 
            } else if (op_flags & OPT_e) {
                vector<string> exp;
                if (!fam.synExpand(member, key, exp)) {
                    cerr << "expand error" << endl;
                    return 1;
                }
                string out;
                stringsToString(exp, out);
                cout << "Family: " << familyname << " Key: " <<  key 
                     << " Expansion: " << out << endl;
            } else {
                Usage();
            }

        } else {
            Xapian::WritableDatabase db(dbdir, Xapian::DB_CREATE_OR_OPEN);
            Rcl::XapWritableSynFamily fam(db, familyname);
            if (op_flags & OPT_D) {
            } else {
                Usage();
            }
        }
    } XCATCHERROR (ermsg);
    if (!ermsg.empty()) {
        cerr << "Xapian Exception: " << ermsg << endl;
        return 1;
    }
    return 0;
}
