#ifndef lint
static char rcsid[] = "@(#$Id: copyfile.cpp,v 1.1 2005-12-05 14:09:16 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

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
