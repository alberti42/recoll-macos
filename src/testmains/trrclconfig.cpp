/* Copyright (C) 2017-2019 J.F.Dockes
 *
 * License: GPL 2.1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <stdio.h>
#include <signal.h>

#include <iostream>
#include <vector>
#include <string>

using namespace std;

#include "log.h"

#include "rclinit.h"
#include "rclconfig.h"
#include "cstr.h"

static char *thisprog;

static char usage [] = "\n"
    "-c: check a few things in the configuration files\n"
    "[-s subkey] -q param : query parameter value\n"
    "-f : print some field data\n"
    "  : default: print parameters\n"

    ;
static void
Usage(void)
{
    fprintf(stderr, "%s: usage: %s\n", thisprog, usage);
    exit(1);
}

static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_s     0x2 
#define OPT_q     0x4 
#define OPT_c     0x8
#define OPT_f     0x10

int main(int argc, char **argv)
{
    string pname, skey;
    
    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
        (*argv)++;
        if (!(**argv))
            /* Cas du "adb - core" */
            Usage();
        while (**argv)
            switch (*(*argv)++) {
            case 'c':   op_flags |= OPT_c; break;
            case 'f':   op_flags |= OPT_f; break;
            case 's':   op_flags |= OPT_s; if (argc < 2)  Usage();
                skey = *(++argv);
                argc--; 
                goto b1;
            case 'q':   op_flags |= OPT_q; if (argc < 2)  Usage();
                pname = *(++argv);
                argc--; 
                goto b1;
            default: Usage();   break;
            }
    b1: argc--; argv++;
    }

    if (argc != 0)
        Usage();

    string reason;
    RclConfig *config = recollinit(0, 0, 0, reason);
    if (config == 0 || !config->ok()) {
        cerr << "Configuration problem: " << reason << endl;
        exit(1);
    }
    if (op_flags & OPT_s)
        config->setKeyDir(skey);
    if (op_flags & OPT_q) {
        string value;
        if (!config->getConfParam(pname, value)) {
            fprintf(stderr, "getConfParam failed for [%s]\n", pname.c_str());
            exit(1);
        }
        printf("[%s] -> [%s]\n", pname.c_str(), value.c_str());
    } else if (op_flags & OPT_f) {
        set<string> stored = config->getStoredFields();
        set<string> indexed = config->getIndexedFields();
        cout << "Stored fields: ";
        for (set<string>::const_iterator it = stored.begin(); 
             it != stored.end(); it++) {
            cout << "[" << *it << "] ";
        }
        cout << endl;   
        cout << "Indexed fields: ";
        for (set<string>::const_iterator it = indexed.begin(); 
             it != indexed.end(); it++) {
            const FieldTraits *ftp;
            config->getFieldTraits(*it, &ftp);
            if (ftp)
                cout << "[" << *it << "]" << " -> [" << ftp->pfx << "] ";
            else 
                cout << "[" << *it << "]" << " -> [" << "(none)" << "] ";

        }
        cout << endl;   
    } else if (op_flags & OPT_c) {
        // Checking the configuration consistency
    
        // Find and display category names
        vector<string> catnames;
        config->getMimeCategories(catnames);
        cout << "Categories: ";
        for (vector<string>::const_iterator it = catnames.begin(); 
             it != catnames.end(); it++) {
            cout << *it << " ";
        }
        cout << endl;

        // Compute union of all types from each category. Check that there
        // are no duplicates while we are at it.
        set<string> allmtsfromcats;
        for (vector<string>::const_iterator it = catnames.begin(); 
             it != catnames.end(); it++) {
            vector<string> cts;
            config->getMimeCatTypes(*it, cts);
            for (vector<string>::const_iterator it1 = cts.begin(); 
                 it1 != cts.end(); it1++) {
                // Already in map -> duplicate
                if (allmtsfromcats.find(*it1) != allmtsfromcats.end()) {
                    cout << "Duplicate: [" << *it1 << "]" << endl;
                }
                allmtsfromcats.insert(*it1);
            }
        }

        // Retrieve complete list of mime types 
        vector<string> mtypes = config->getAllMimeTypes();

        // And check that each mime type is found in exactly one category
        // And have an icon
        for (const auto& mtype: mtypes) {
            if (allmtsfromcats.find(mtype) == allmtsfromcats.end()) {
                cout << "Not found in catgs: [" << mtype << "]" << endl;
            }
            // We'd like to check for types with no icons, but
            // getMimeIconPath() returns the valid 'document' by
            // default, we'd have to go look into the confsimple
            // directly.
            // string path = config->getMimeIconPath(mtype, string());
            // cout << mtype << " -> " << path << endl;
        }
        
        // List mime types not in mimeview
        for (vector<string>::const_iterator it = mtypes.begin();
             it != mtypes.end(); it++) {
            if (config->getMimeViewerDef(*it, "", false).empty()) {
                cout << "No viewer: [" << *it << "]" << endl;
            }
        }

        // Check that each mime type has an indexer
        for (vector<string>::const_iterator it = mtypes.begin();
             it != mtypes.end(); it++) {
            if (config->getMimeHandlerDef(*it, false).empty()) {
                cout << "No filter: [" << *it << "]" << endl;
            }
        }

        // Check that each mime type has a defined icon
        for (vector<string>::const_iterator it = mtypes.begin();
             it != mtypes.end(); it++) {
            if (config->getMimeIconPath(*it, "") == "document") {
                cout << "No or generic icon: [" << *it << "]" << endl;
            }
        }

    } else {
        config->setKeyDir(cstr_null);
        vector<string> names = config->getConfNames();
        for (vector<string>::iterator it = names.begin(); 
             it != names.end();it++) {
            string value;
            config->getConfParam(*it, value);
            cout << *it << " -> [" << value << "]" << endl;
        }
    }
    exit(0);
}
