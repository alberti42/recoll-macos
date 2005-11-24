#ifndef lint
static char rcsid[] = "@(#$Id: readfile.cpp,v 1.2 2005-11-24 07:16:16 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <unistd.h>
#include <fcntl.h>
#ifndef O_STREAMING
#define O_STREAMING 0
#endif
#include <errno.h>

#include <string>
#ifndef NO_NAMESPACES
using std::string;
#endif /* NO_NAMESPACES */

#include "readfile.h"

bool file_to_string(const string &fn, string &data)
{
    bool ret = false;

    int fd = open(fn.c_str(), O_RDONLY|O_STREAMING);
    if (fd < 0) {
	// perror("open");
	return false;
    }
    char buf[4096];
    for (;;) {
	int n = read(fd, buf, 4096);
	if (n < 0) {
	    // perror("read");
	    goto out;
	}
	if (n == 0)
	    break;

	try {
	    data.append(buf, n);
	} catch (...) {
	    //	    fprintf(stderr, "file_to_string: out of memory\n");
	    goto out;
	}
    }

    ret = true;
 out:
    if (fd >= 0)
	close(fd);
    return ret;
}
