#ifndef lint
static char rcsid[] = "@(#$Id: wipedir.cpp,v 1.1 2005-02-09 12:07:30 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#ifndef TEST_WIPEDIR
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#include <string>
using namespace std;

#include "debuglog.h"
#include "pathut.h"
#include "wipedir.h"

int wipedir(const string& dir)
{
    struct stat st;
    int statret;
    int ret = -1;

    statret = stat(dir.c_str(), &st);
    if (statret == -1) {
	LOGERR(("wipedir: cant stat %s, errno %d\n", dir.c_str(), errno));
	return -1;
    }
    if (!S_ISDIR(st.st_mode)) {
	LOGERR(("wipedir: %s not a directory\n", dir.c_str()));
	return -1;
    }

    if (access(dir.c_str(), R_OK|W_OK|X_OK) < 0) {
	LOGERR(("wipedir: no write access to %s\n", dir.c_str()));
	return -1;
    }

    DIR *d = opendir(dir.c_str());
    if (d == 0) {
	LOGERR(("wipedir: cant opendir %s, errno %d\n", dir.c_str(), errno));
	return -1;
    }
    int remaining = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != 0) {
	if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) 
	    continue;

	string fn = dir;
	path_cat(fn, ent->d_name);

	struct stat st;
	int statret = stat(fn.c_str(), &st);
	if (statret == -1) {
	    LOGERR(("wipedir: cant stat %s, errno %d\n", fn.c_str(), errno));
	    goto out;
	}
	if (S_ISDIR(st.st_mode)) {
	    remaining++;
	} else {
	    if (unlink(fn.c_str()) < 0) {
		LOGERR(("wipedir: cant unlink %s, errno %d\n", 
			fn.c_str(), errno));
		goto out;
	    }
	}
    }

    ret = remaining;
 out:
    if (d)
	closedir(d);
    return ret;
}


#else // FILEUT_TEST

#include <sys/stat.h>

#include "wipedir.h"

using namespace std;

int main(int argc, const char **argv)
{
    if (argc != 2) {
	fprintf(stderr, "Usage: wipedir <dir>\n");
	exit(1);
    }
    string dir = argv[1];
    int cnt = wipedir(dir);
    printf("wipedir returned %d\n", cnt);
    exit(0);
}

#endif
