/* Copyright (C) 2005 J.F.Dockes
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
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>
#include <vector>


#include "log.h"
#include "transcode.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

#define UTF8ITER_CHECK
#include "utf8iter.h"
#include "readfile.h"
#include "textsplit.h"

void tryempty()
{
    Utf8Iter it("");
    cout << "EOF ? " << it.eof() << endl;
    TextSplit::isCJK(*it);
    exit(0);
}

const char *thisprog;
static char usage [] =
                                    "utf8iter [opts] infile outfile\n"
                                    " converts infile to 32 bits unicode (processor order), for testing\n"
                                    "-v : print stuff as we go\n"
                                    ;

void Usage() {
    fprintf(stderr, "%s:%s\n", thisprog, usage);
    exit(1);
}
static int     op_flags;
#define OPT_v     0x2 

FILE *infout = stdout;

int main(int argc, char **argv)
{
    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
        (*argv)++;
        if (!(**argv))
            Usage();
        while (**argv)
            switch (*(*argv)++) {
            case 'v':   op_flags |= OPT_v; break;

            default: Usage();   break;
            }
        argc--;argv++;
    }
    string infile, outfile;
    if (argc == 2) {
        infile = *argv++;argc--;
        outfile = *argv++;argc--;
        Usage();
    } else if (argc != 0) {
        Usage();
    }
    string in;
    if (!file_to_string(infile, in)) {
        cerr << "Cant read file\n" << endl;
        exit(1);
    }
    
    vector<unsigned int>ucsout1;
    string out, out1;
    Utf8Iter it(in);
    FILE *fp = 0;
    if (!outfile.empty()) {
        fp = fopen(outfile.c_str(), "w");
        if (fp == 0) {
            cerr << "Can't create " << outfile << endl;
            exit(1);
        }
    }

    int nchars = 0;
    for (;!it.eof(); it++) {
        unsigned int value = *it;
        if (value == (unsigned int)-1) {
            cerr << "Conversion error occurred at position " << it.getBpos()
                 << endl;
            exit(1);
        }
        if (op_flags & OPT_v) {
            fprintf(infout, "Value: 0x%04x", value);
            if (value < 0x7f)
                fprintf(stdout, " (%c) ", value);
            fprintf(infout, "\n");
        }
        // UTF-32LE or BE array
        ucsout1.push_back(value);
        if (fp) {
            // UTF-32LE or BE file
            fwrite(&value, 4, 1, fp);
        }

        // Reconstructed utf8 strings (2 methods)
        if (!it.appendchartostring(out))
            break;
        // conversion to string
        out1 += it;
        
        // fprintf(stderr, "%s", string(it).c_str());
        nchars++;
    }
    if (fp) {
        fclose(fp);
    }

    fprintf(infout, "Found %d Unicode characters\n", nchars);
    if (in.compare(out)) {
        fprintf(stderr, "error: out != in\n");
        exit(1);
    }
    if (in != out1) {
        fprintf(stderr, "error: out1 != in\n");
        exit(1);
    }

    // Rewind and do it a second time
    vector<unsigned int>ucsout2;
    it.rewind();
    for (int i = 0; ; i++) {
        unsigned int value;
        if ((value = it[i]) == (unsigned int)-1) {
            break;
        }
        it++;
        ucsout2.push_back(value);
    }

    if (ucsout1 != ucsout2) {
        fprintf(stderr, "error: ucsout1 != ucsout2\n");
        exit(1);
    }

    ucsout2.clear();
    int ercnt;
    const char *encoding = "UTF-32LE"; // note : use BE on high-endian machine
    string ucs, ucs1;
    for (const unsigned int i : ucsout1) {
        ucs.append((const char *)&i, 4);
    }
    if (!transcode(ucs, ucs1, encoding, encoding, &ercnt) || ercnt) {
        fprintf(stderr, "Transcode check failed, ercount: %d\n", ercnt);
        exit(1);
    }
    if (ucs.compare(ucs1)) {
        fprintf(stderr, "error: ucsout1 != ucsout2 after iconv\n");
        exit(1);
    }

    if (!transcode(ucs, ucs1, encoding, "UTF-8", &ercnt) || ercnt) {
        fprintf(stderr, "Transcode back to utf-8 check failed, ercount: %d\n",
                ercnt);
        exit(1);
    }
    if (ucs1.compare(in)) {
        fprintf(stderr, "Transcode back to utf-8 compare to in failed\n");
        exit(1);
    }
    exit(0);
}
