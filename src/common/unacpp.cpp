#ifndef lint
static char rcsid[] = "@(#$Id: unacpp.cpp,v 1.4 2005-11-24 07:16:15 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#ifndef TEST_UNACPP
#include <stdio.h>

#include <errno.h>

#include <string>

#ifndef NO_NAMESPACES
using std::string;
#endif /* NO_NAMESPACES */

#include "unacpp.h"
#include "unac.h"


bool unac_cpp(const std::string &in, std::string &out, const char *encoding)
{
    char *cout = 0;
    size_t out_len;

    if (unac_string(encoding, in.c_str(), in.length(), &cout, &out_len) < 0) {
	char cerrno[20];
	sprintf(cerrno, "%d", errno);
	out = string("unac_string failed, errno : ") + cerrno;
	return false;
    }
    out.assign(cout, out_len);
    free(cout);
    return true;
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
    if (!unac_cpp(odata, ndata, encoding)) {
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
