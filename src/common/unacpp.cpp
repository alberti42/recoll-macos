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
		   const char *encoding, bool dofold)
{
    char *cout = 0;
    size_t out_len;
    int status;
    status = dofold ? 
	unacfold_string(encoding, in.c_str(), in.length(), &cout, &out_len) :
	unac_string(encoding, in.c_str(), in.length(), &cout, &out_len);
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
    if (!unacmaybefold(shorter, noacterm, "UTF-8", false)) {
	LOGINFO(("unaciscapital: unac failed for [%s]\n", in.c_str()));
	return false;
    } 
    if (!unacmaybefold(noacterm, noaclowterm, "UTF-8", true)) {
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

int main(int argc, char **argv)
{
    bool dofold = true;
    if (argc != 4) {
	cerr << "Usage: unacpp  <encoding> <infile> <outfile>" << endl;
	exit(1);
    }
    const char *encoding = argv[1];
    string ifn = argv[2];
    const char *ofn = argv[3];

    string odata;
    if (!file_to_string(ifn, odata)) {
	cerr << "file_to_string: " << odata << endl;
	exit(1);
    }
    string ndata;
    if (!unacmaybefold(odata, ndata, encoding, dofold)) {
	cerr << "unac: " << ndata << endl;
	exit(1);
    }
    
    int fd = open(ofn, O_CREAT|O_EXCL|O_WRONLY, 0666);
    if (fd < 0) {
	cerr << "Open/Create " << ofn << " failed: " << strerror(errno) 
	     << endl;
	exit(1);
    }
    if (write(fd, ndata.c_str(), ndata.length()) != (int)ndata.length()) {
	cerr << "Write(2) failed: " << strerror(errno)  << endl;
	exit(1);
    }
    close(fd);
    exit(0);
}

#endif
