#ifndef lint
static char rcsid[] = "@(#$Id: utf8iter.cpp,v 1.4 2006-01-23 13:32:28 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include <stdio.h>
#include <string>
#include <iostream>
#include <list>
#include <vector>
#include "debuglog.h"
#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

#include "utf8iter.h"
#include "readfile.h"



int main(int argc, char **argv)
{
    if (argc != 3) {
	cerr << "Usage: utf8iter infile outfile" << endl;
	exit(1);
    }
    const char *infile = argv[1];
    const char *outfile = argv[2];
    string in;
    if (!file_to_string(infile, in)) {
	cerr << "Cant read file\n" << endl;
	exit(1);
    }
    
    vector<unsigned int>ucsout1;
    string out, out1;
    Utf8Iter it(in);
    FILE *fp = fopen(outfile, "w");
    if (fp == 0) {
	fprintf(stderr, "cant create %s\n", outfile);
	exit(1);
    }
    int nchars = 0;
    for (;!it.eof(); it++) {
	unsigned int value = *it;
	if (value == (unsigned int)-1) {
	    fprintf(stderr, "Conversion error occurred\n");
	    exit(1);
	}
	ucsout1.push_back(value);
	fwrite(&value, 4, 1, fp);
	if (!it.appendchartostring(out))
	    break;
	out1 += it;
	nchars++;
    }
    fprintf(stderr, "nchars1 %d\n", nchars);
    if (in != out) {
	fprintf(stderr, "error: out != in\n");
	exit(1);
    }
    if (in != out1) {
	fprintf(stderr, "error: out1 != in\n");
	exit(1);
    }

    vector<unsigned int>ucsout2;
    it.rewind();
    for (int i = 0; ; i++) {
	unsigned int value;
	if ((value = it[i]) == (unsigned int)-1) {
	    fprintf(stderr, "%d chars\n", i);
	    break;
	}
	it++;
	ucsout2.push_back(value);
    }

    if (ucsout1 != ucsout2) {
	fprintf(stderr, "error: ucsout1 != ucsout2\n");
	exit(1);
    }

    fclose(fp);
    exit(0);
}

