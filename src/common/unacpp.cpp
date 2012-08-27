/* Copyright (C) 2004 J.F.Dockes
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

#ifndef TEST_UNACPP
#include <stdio.h>
#include <cstdlib>
#include <errno.h>

#include <string>

#include "unacpp.h"
#include "unac.h"
#include "debuglog.h"
#include "utf8iter.h"

bool unacmaybefold(const string &in, string &out, 
		   const char *encoding, UnacOp what)
{
    char *cout = 0;
    size_t out_len;
    int status = -1;

    switch (what) {
    case UNACOP_UNAC:
	status = unac_string(encoding, in.c_str(), in.length(), 
			     &cout, &out_len);
	break;
    case UNACOP_UNACFOLD:
	status = unacfold_string(encoding, in.c_str(), in.length(), 
				 &cout, &out_len);
	break;
    case UNACOP_FOLD:
	status = fold_string(encoding, in.c_str(), in.length(), 
			     &cout, &out_len);
	break;
    }

    if (status < 0) {
	if (cout)
	    free(cout);
	char cerrno[20];
	sprintf(cerrno, "%d", errno);
	out = string("unac_string failed, errno : ") + cerrno;
	return false;
    }
    out.assign(cout, out_len);
    if (cout)
	free(cout);
    return true;
}

bool unaciscapital(const string& in)
{
    if (in.empty())
	return false;
    Utf8Iter it(in);
    string shorter;
    it.appendchartostring(shorter);

    string noacterm, noaclowterm;
    if (!unacmaybefold(shorter, noacterm, "UTF-8", UNACOP_UNAC)) {
	LOGINFO(("unaciscapital: unac failed for [%s]\n", in.c_str()));
	return false;
    } 
    if (!unacmaybefold(noacterm, noaclowterm, "UTF-8", UNACOP_UNACFOLD)) {
	LOGINFO(("unaciscapital: unacfold failed for [%s]\n", in.c_str()));
	return false;
    }
    Utf8Iter it1(noacterm);
    Utf8Iter it2(noaclowterm);
    if (*it1 != *it2)
	return true;
    else
	return false;
}

#else // not testing

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <iostream>

using namespace std;

#include "unacpp.h"
#include "readfile.h"
#include "rclinit.h"

static char *thisprog;

static char usage [] = "\n"
    "[-c|-C] <encoding> <infile> <outfile>\n"
    " Default : unaccent\n"
    " -c : unaccent and casefold\n"
    " -C : casefold only\n"
    "\n";

;
static void
Usage(void)
{
    fprintf(stderr, "%s: usage: %s\n", thisprog, usage);
    exit(1);
}

static int     op_flags;
#define OPT_c	  0x2 
#define OPT_C	  0x4 

int main(int argc, char **argv)
{
    UnacOp op = UNACOP_UNAC;

    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
	(*argv)++;
	if (!(**argv))
	    /* Cas du "adb - core" */
	    Usage();
	while (**argv)
	    switch (*(*argv)++) {
	    case 'c':	op_flags |= OPT_c; break;
	    case 'C':	op_flags |= OPT_C; break;
	    default: Usage();	break;
	    }
	argc--; argv++;
    }

    if (op_flags & OPT_c) {
	op = UNACOP_UNACFOLD;
    } else if (op_flags & OPT_C) {
	op = UNACOP_FOLD;
    }

    if (argc != 3) {
	Usage();
    }

    const char *encoding = *argv++; argc--;
    string ifn = *argv++; argc--;
    if (!ifn.compare("stdin"))
	ifn.clear();
    const char *ofn = *argv++; argc--;

    string reason;
    (void)recollinit(RCLINIT_NONE, 0, 0, reason, 0);

    string odata;
    if (!file_to_string(ifn, odata)) {
	cerr << "file_to_string " << ifn << " : " << odata << endl;
	return 1;
    }
    string ndata;
    if (!unacmaybefold(odata, ndata, encoding, op)) {
	cerr << "unac: " << ndata << endl;
	return 1;
    }
    
    int fd;
    if (strcmp(ofn, "stdout")) {
	fd = open(ofn, O_CREAT|O_EXCL|O_WRONLY, 0666);
    } else {
	fd = 1;
    }
    if (fd < 0) {
	cerr << "Open/Create " << ofn << " failed: " << strerror(errno) 
	     << endl;
	return 1;
    }
    if (write(fd, ndata.c_str(), ndata.length()) != (int)ndata.length()) {
	cerr << "Write(2) failed: " << strerror(errno)  << endl;
	return 1;
    }
    close(fd);
    return 0;
}

#endif
