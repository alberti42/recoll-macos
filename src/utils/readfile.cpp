#ifndef lint
static char rcsid[] = "@(#$Id: readfile.cpp,v 1.3 2006-01-23 13:32:28 dockes Exp $ (C) 2004 J.F.Dockes";
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
