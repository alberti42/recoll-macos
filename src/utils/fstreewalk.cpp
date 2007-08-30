#ifndef lint
static char rcsid[] = "@(#$Id: fstreewalk.cpp,v 1.14 2007-08-30 09:01:52 dockes Exp $ (C) 2004 J.F.Dockes";
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

#ifndef TEST_FSTREEWALK

#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <fnmatch.h>

#include <sstream>
#include <list>

#include "debuglog.h"
#include "pathut.h"
#include "fstreewalk.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

class FsTreeWalker::Internal {
    Options options;
    stringstream reason;
    list<string> skippedNames;
    list<string> skippedPaths;
    int errors;
    void logsyserr(const char *call, const string &param) 
    {
	errors++;
	reason << call << "(" << param << ") : " << errno << " : " << 
	    strerror(errno) << endl;
    }
    friend class FsTreeWalker;
};

FsTreeWalker::FsTreeWalker(Options opts)
{
    data = new Internal;
    if (data) {
	data->options = opts;
	data->errors = 0;
    }
}

FsTreeWalker::~FsTreeWalker()
{
    delete data;
}

void FsTreeWalker::setOpts(Options opts)
{
    if (data) {
	data->options = opts;
    }
}

string FsTreeWalker::getReason()
{
    return data->reason.str();
}

int FsTreeWalker::getErrCnt()
{
    return data->errors;
}

bool FsTreeWalker::addSkippedName(const string& pattern)
{
    if (find(data->skippedNames.begin(), 
	     data->skippedNames.end(), pattern) == data->skippedNames.end())
	data->skippedNames.push_back(pattern);
    return true;
}
bool FsTreeWalker::setSkippedNames(const list<string> &patterns)
{
    data->skippedNames = patterns;
    data->skippedNames.sort();
    data->skippedNames.unique();
    return true;
}
bool FsTreeWalker::inSkippedNames(const string& name)
{
    list<string>::const_iterator it;
    for (it = data->skippedNames.begin(); 
	 it != data->skippedNames.end(); it++) {
	if (fnmatch(it->c_str(), name.c_str(), 0) == 0) {
	    return true;
	}
    }
    return false;
}

bool FsTreeWalker::addSkippedPath(const string& ipath)
{
    string path = path_canon(ipath);
    if (find(data->skippedPaths.begin(), 
	     data->skippedPaths.end(), path) == data->skippedPaths.end())
	data->skippedPaths.push_back(path);
    return true;
}
bool FsTreeWalker::setSkippedPaths(const list<string> &paths)
{
    data->skippedPaths = paths;
    for (list<string>::iterator it = data->skippedPaths.begin();
	 it != data->skippedPaths.end(); it++)
	*it = path_canon(*it);
    data->skippedPaths.sort();
    data->skippedPaths.unique();
    return true;
}
bool FsTreeWalker::inSkippedPaths(const string& path)
{
    list<string>::const_iterator it;
    for (it = data->skippedPaths.begin(); 
	 it != data->skippedPaths.end(); it++) {
	if (fnmatch(it->c_str(), path.c_str(), FNM_PATHNAME) == 0) 
	    return true;
    }
    return false;
}

FsTreeWalker::Status FsTreeWalker::walk(const string& _top, 
					FsTreeWalkerCB& cb)
{
    string top = path_canon(_top);

    struct stat st;
    int statret;
    // We always follow symlinks at this point. Makes more sense.
    statret = stat(top.c_str(), &st);
    if (statret == -1) {
	data->logsyserr("stat", top);
	return FtwError;
    }
    return iwalk(top, &st, cb);
}

// Note that the 'norecurse' flag is handled as part of the directory read. 
// This means that we always go into the top 'walk()' parameter if it is a 
// directory, even if norecurse is set. Bug or Feature ?
FsTreeWalker::Status FsTreeWalker::iwalk(const string &top, 
					 struct stat *stp,
					 FsTreeWalkerCB& cb)
{
    Status status = FtwOk;

    // Handle the parameter
    if (S_ISDIR(stp->st_mode)) {
	if ((status = cb.processone(top, stp, FtwDirEnter)) & 
	    (FtwStop|FtwError)) {
	    return status;
	}
    } else if (S_ISREG(stp->st_mode)) {
	return cb.processone(top, stp, FtwRegular);
    } else {
	return status;
    }

    // This is a directory, read it and process entries:
    DIR *d = opendir(top.c_str());
    if (d == 0) {
	data->logsyserr("opendir", top);
	switch (errno) {
	case EPERM:
	case EACCES:
	    goto out;
	default:
	    status = FtwError;
	    goto out;
	}
    }

    struct dirent *ent;
    while ((ent = readdir(d)) != 0) {
	// Skip . and ..
	if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) 
	    continue;

	// Skipped file names match ?
	if (!data->skippedNames.empty()) {
	    if (inSkippedNames(ent->d_name))
		goto skip;
	}

	{
	    string fn = path_cat(top, ent->d_name);

	    struct stat st;
	    int statret = (data->options & FtwFollow) ? stat(fn.c_str(), &st) :
		lstat(fn.c_str(), &st);
	    if (statret == -1) {
		data->logsyserr("stat", fn);
		continue;
	    }
	    if (!data->skippedPaths.empty()) {
		if (inSkippedPaths(fn))
		    goto skip;
	    }

	    if (S_ISDIR(st.st_mode)) {
		if (data->options & FtwNoRecurse) {
		    status = cb.processone(fn, &st, FtwDirEnter);
		} else {
		    status = iwalk(fn, &st, cb);
		}
		if (status & (FtwStop|FtwError))
		    goto out;
		if ((status = cb.processone(top, &st, FtwDirReturn)) 
		    & (FtwStop|FtwError))
		    goto out;
	    } else if (S_ISREG(st.st_mode)) {
		if ((status = cb.processone(fn, &st, FtwRegular)) & 
		    (FtwStop|FtwError)) {
		    goto out;
		}
	    }
	}

    skip: ;

	// We skip other file types (devices etc...)
    }

 out:
    if (d)
	closedir(d);
    return status;
}
	
#else // TEST_FSTREEWALK
#include <sys/stat.h>

#include <iostream>

#include "fstreewalk.h"

using namespace std;

class myCB : public FsTreeWalkerCB {
 public:
    FsTreeWalker::Status processone(const string &path, 
				 const struct stat *st,
				 FsTreeWalker::CbFlag flg)
    {
	if (flg == FsTreeWalker::FtwDirEnter) {
	    cout << "[Entering " << path << "]" << endl;
	} else if (flg == FsTreeWalker::FtwDirReturn) {
	    cout << "[Returning to " << path << "]" << endl;
	} else if (flg == FsTreeWalker::FtwRegular) {
	    cout << path << endl;
	}
	return FsTreeWalker::FtwOk;
    }
};

static const char *thisprog;

static char usage [] =
"trfstreewalk [-p pattern] [-P ignpath] topdir\n\n"
;
static void
Usage(void)
{
    fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_p	  0x2 
#define OPT_P	  0x4 

int main(int argc, const char **argv)
{
    list<string> patterns;
    list<string> paths;
    thisprog = argv[0];
    argc--; argv++;

  while (argc > 0 && **argv == '-') {
    (*argv)++;
    if (!(**argv))
      /* Cas du "adb - core" */
      Usage();
    while (**argv)
      switch (*(*argv)++) {
      case 'p':	op_flags |= OPT_p; if (argc < 2)  Usage();
	  patterns.push_back(*(++argv));
	  argc--; 
	  goto b1;
      case 'P':	op_flags |= OPT_P; if (argc < 2)  Usage();
	  paths.push_back(*(++argv));
	argc--; 
	goto b1;
      default: Usage();	break;
      }
  b1: argc--; argv++;
  }

  if (argc != 1)
    Usage();
  string topdir = *argv++;argc--;

  FsTreeWalker walker;
  walker.setSkippedNames(patterns);
  walker.setSkippedPaths(paths);
  myCB cb;
  walker.walk(topdir, cb);
  if (walker.getErrCnt() > 0)
      cout << walker.getReason();
}

#endif // TEST_FSTREEWALK
