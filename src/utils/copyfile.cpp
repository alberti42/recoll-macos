#ifndef lint
static char rcsid[] = "@(#$Id: copyfile.cpp,v 1.4 2007-12-13 06:58:22 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>

#include <string>
using std::string;

#include "debuglog.h"

#define CPBSIZ 8192
#define COPYFILE_NOERRUNLINK 1

bool copyfile(const char *src, const char *dst, string &reason, int flags = 0)
{
  int sfd = -1;
  int dfd = -1;
  bool ret = false;
  char buf[CPBSIZ];

  reason.erase();

  LOGDEB(("copyfile: %s to %s\n", src, dst));

  if ((sfd = open(src, O_RDONLY)) < 0) {
      reason += string("open ") + src + ": " + strerror(errno);
      goto out;
  }

  if ((dfd = open(dst, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0) {
      reason += string("open/creat ") + dst + ": " + strerror(errno);
      // If we fail because of an open/truncate error, we do not want to unlink
      // the file, we might succeed...
      flags |= COPYFILE_NOERRUNLINK;
      goto out;
  }

  for (;;) {
      int didread;
      didread = read(sfd, buf, CPBSIZ);
      if (didread < 0) {
	  reason += string("read src ") + src + ": " + strerror(errno);
	  goto out;
      }
      if (didread == 0)
	  break;
      if (write(dfd, buf, didread) != didread) {
	  reason += string("write dst ") + src + ": " + strerror(errno);
	  goto out;
      }
  }
  ret = true;
 out:
  if (ret == false && !(flags&COPYFILE_NOERRUNLINK))
    unlink(dst);
  if (sfd >= 0)
    close(sfd);
  if (dfd >= 0)
    close(dfd);
  return ret;
}
