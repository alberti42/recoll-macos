#ifndef lint
static char	rcsid[] = "@(#$Id: transcode.cpp,v 1.4 2005-11-24 07:16:16 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#ifndef TEST_TRANSCODE

#include <errno.h>

#include <string>
#include <iostream>
#ifndef NO_NAMESPACES
using std::string;
#endif /* NO_NAMESPACES */

#include <iconv.h>

#include "transcode.h"


bool transcode(const string &in, string &out, const string &icode,
	       const string &ocode)
{
    iconv_t ic;
    bool ret = false;
    const int OBSIZ = 8192;
    char obuf[OBSIZ], *op;
    bool icopen = false;

    out.erase();
    size_t isiz = in.length();
    out.reserve(isiz);
    const char *ip = in.c_str();

    if((ic = iconv_open(ocode.c_str(), icode.c_str())) == (iconv_t)-1) {
	out = string("iconv_open failed for ") + icode
	    + " -> " + ocode;
	goto error;
    }
    icopen = true;

    while (isiz > 0) {
	size_t osiz;
	op = obuf;
	osiz = OBSIZ;
	if(iconv(ic, 
#if defined(_LIBICONV_VERSION)
		 &ip, 
#else
		 (char **)&ip, 
#endif
		 &isiz, &op, &osiz) == (size_t)-1 && errno != E2BIG){
	    out.erase();
	    out = string("iconv failed for ") + icode + " -> " + ocode +
		" : " + strerror(errno);
	    goto error;
	}
	out.append(obuf, OBSIZ - osiz);
    }

    if(iconv_close(ic) == -1) {
	out.erase();
	out = string("iconv_close failed for ") + icode
	    + " -> " + ocode;
	goto error;
    }
    icopen = false;
    ret = true;
 error:
    if (icopen)
	iconv_close(ic);
    return ret;
}


#else

#include <errno.h>

#include <string>
#include <iostream>

#include <unistd.h>
#include <fcntl.h>

using namespace std;

#include "readfile.h"
#include "transcode.h"

int main(int argc, char **argv)
{
    if (argc != 5) {
	cerr << "Usage: trcsguess ifilename icode ofilename ocode" << endl;
	exit(1);
    }
    const string ifilename = argv[1];
    const string icode = argv[2];
    const string ofilename = argv[3];
    const string ocode = argv[4];

    string text;
    if (!file_to_string(ifilename, text)) {
	cerr << "Couldnt read file, errno " << errno << endl;
	exit(1);
    }
    string out;
    if (!transcode(text, out, icode, ocode)) {
	cerr << out << endl;
	exit(1);
    }
    int fd = open(ofilename.c_str(), O_CREAT|O_TRUNC|O_WRONLY, 0666);
    if (fd < 0) {
	perror("Open/create output");
	exit(1);
    }
    if (write(fd, out.c_str(), out.length()) != (int)out.length()) {
	perror("write");
	exit(1);
    }
    close(fd);
    exit(0);
}
#endif
