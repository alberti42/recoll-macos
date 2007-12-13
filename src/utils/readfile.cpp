#ifndef lint
static char rcsid[] = "@(#$Id: readfile.cpp,v 1.7 2007-12-13 06:58:22 dockes Exp $ (C) 2004 J.F.Dockes";
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
#include <cstring>

#include <string>
#ifndef NO_NAMESPACES
using std::string;
#endif /* NO_NAMESPACES */

#include "readfile.h"

static void caterrno(string *reason)
{
#define ERRBUFSZ 200    
    char errbuf[ERRBUFSZ];
  if (reason) {
#ifdef sun
    // Note: sun strerror is noted mt-safe ??
    *reason += string("file_to_string: open failed: ") + strerror(errno);
#else
    strerror_r(errno, errbuf, ERRBUFSZ);
    *reason += string("file_to_string: open failed: ") + errbuf;
#endif
  }
}

bool file_to_string(const string &fn, string &data, string *reason)
{
    bool ret = false;

    int fd = open(fn.c_str(), O_RDONLY|O_STREAMING);
    if (fd < 0) {
        caterrno(reason);
	return false;
    }
    char buf[4096];
    for (;;) {
	int n = read(fd, buf, 4096);
	if (n < 0) {
	    caterrno(reason);
	    goto out;
	}
	if (n == 0)
	    break;

	try {
	    data.append(buf, n);
	} catch (...) {
	    caterrno(reason);
	    goto out;
	}
    }

    ret = true;
 out:
    if (fd >= 0)
	close(fd);
    return ret;
}
