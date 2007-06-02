#ifndef lint
static char rcsid[] = "@(#$Id: readfile.cpp,v 1.4 2007-06-02 08:30:42 dockes Exp $ (C) 2004 J.F.Dockes";
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

bool file_to_string(const string &fn, string &data, string *reason)
{
#define ERRBUFSZ 200    
    char errbuf[ERRBUFSZ];
    bool ret = false;

    int fd = open(fn.c_str(), O_RDONLY|O_STREAMING);
    if (fd < 0) {
	if (reason) {
	    strerror_r(errno, errbuf, ERRBUFSZ);
	    *reason += string("file_to_string: open failed: ") + errbuf;
	}
	return false;
    }
    char buf[4096];
    for (;;) {
	int n = read(fd, buf, 4096);
	if (n < 0) {
	    if (reason) {
		strerror_r(errno, errbuf, ERRBUFSZ);
		*reason += string("file_to_string: read failed: ") + errbuf;
	    }
	    goto out;
	}
	if (n == 0)
	    break;

	try {
	    data.append(buf, n);
	} catch (...) {
	    if (reason) {
		strerror_r(errno, errbuf, ERRBUFSZ);
		*reason += string("file_to_string: out of memory? : ") +errbuf;
	    }
	    goto out;
	}
    }

    ret = true;
 out:
    if (fd >= 0)
	close(fd);
    return ret;
}
