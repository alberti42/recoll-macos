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
#ifndef TEST_COPYFILE
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/times.h>

#include "copyfile.h"
#include "debuglog.h"

#define CPBSIZ 8192
#define COPYFILE_NOERRUNLINK 1

bool copyfile(const char *src, const char *dst, string &reason, int flags)
{
  int sfd = -1;
  int dfd = -1;
  bool ret = false;
  char buf[CPBSIZ];

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

bool renameormove(const char *src, const char *dst, string &reason)
{
    // First try rename(2). If this succeeds we're done. If this fails
    // with EXDEV, try to copy. Unix really should have a library function
    // for this.
    if (rename(src, dst) == 0) {
        return true;
    }
    if (errno != EXDEV) {
        reason += string("rename(2) failed: ") + strerror(errno);
        return false;
    } 

    struct stat st;
    if (stat(src, &st) < 0) {
        reason += string("Can't stat ") + src + " : " + strerror(errno);
        return false;
    }
    if (!copyfile(src, dst, reason))
        return false;

    struct stat st1;
    if (stat(dst, &st1) < 0) {
        reason += string("Can't stat ") + dst + " : " + strerror(errno);
        return false;
    }

    // Try to preserve modes, owner, times. This may fail for a number
    // of reasons
    if ((st1.st_mode & 0777) != (st.st_mode & 0777)) {
        chmod(dst, st.st_mode&0777);
    }
    if (st.st_uid != st1.st_uid || st.st_gid != st1.st_gid) {
        chown(dst, st.st_uid, st.st_gid);
    }
    struct timeval times[2];
    times[0].tv_sec = st.st_atime;
    times[0].tv_usec = 0;
    times[1].tv_sec = st.st_mtime;
    times[1].tv_usec = 0;
    utimes(dst, times);

    // All ok, get rid of origin
    if (unlink(src) < 0) {
        reason += string("Can't unlink ") + src + "Error : " + strerror(errno);
    }

    return true;
}


#else 

#include <stdlib.h>
#include <unistd.h>

#include <string>
#include <iostream>
using namespace std;

#include "copyfile.h"

static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_m     0x2

static const char *thisprog;
static char usage [] =
"trcopyfile [-m] src dst\n"
" -m : move instead of copying\n"
"\n"
;
static void
Usage(void)
{
    fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

int main(int argc, const char **argv)
{
    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
        (*argv)++;
        if (!(**argv))
            /* Cas du "adb - core" */
            Usage();
        while (**argv)
            switch (*(*argv)++) {
            case 'm':	op_flags |= OPT_m; break;
            default: Usage();	break;
            }
        argc--; argv++;
    }

    if (argc != 2)
        Usage();
    string src = *argv++;argc--;
    string dst = *argv++;argc--;
    bool ret;
    string reason;
    if (op_flags & OPT_m) {
        ret = renameormove(src.c_str(), dst.c_str(), reason);
    } else {
        ret = copyfile(src.c_str(), dst.c_str(), reason);
    }
    if (!ret) {
        cerr << reason << endl;
        exit(1);
    } 
    exit(0);
}


#endif
