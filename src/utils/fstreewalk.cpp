#ifndef lint
static char rcsid[] = "@(#$Id: fstreewalk.cpp,v 1.4 2005-04-04 13:18:47 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

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

using namespace std;

class FsTreeWalker::Internal {
    Options options;
    stringstream reason;
    list<string> skippedNames;
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
    data->skippedNames.push_back(pattern);
    return true;
}

void FsTreeWalker::clearSkippedNames()
{
    data->skippedNames.clear();
}


FsTreeWalker::Status FsTreeWalker::walk(const string &top, 
					FsTreeWalkerCB& cb)
{
    Status status = FtwOk;
    struct stat st;
    int statret;

    // Handle the top entry
    statret = (data->options & FtwFollow) ? stat(top.c_str(), &st) :
	lstat(top.c_str(), &st);
    if (statret == -1) {
	data->logsyserr("stat", top);
	return FtwError;
    }
    if (S_ISDIR(st.st_mode)) {
	if ((status = cb.processone(top, &st, FtwDirEnter)) & 
	    (FtwStop|FtwError)) {
	    return status;
	}
    } else if (S_ISREG(st.st_mode)) {
	return cb.processone(top, &st, FtwRegular);
    } else {
	return status;
    }

    // Handle directory entries
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
	// We do process hidden files for now, only skip . and ..
	if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) 
	    continue;

	if (!data->skippedNames.empty()) {
	    list<string>::const_iterator it;
	    for (it = data->skippedNames.begin(); 
		 it != data->skippedNames.end(); it++) {
		if (fnmatch(it->c_str(), ent->d_name, 0) == 0) {
		    //fprintf(stderr, 
		    //"Skipping [%s] because of pattern match\n", ent->d_name);
		    goto skip;
		}
	    }
	}

	{
	    string fn = top;
	    path_cat(fn, ent->d_name);

	    struct stat st;
	    int statret = (data->options & FtwFollow) ? stat(fn.c_str(), &st) :
		lstat(fn.c_str(), &st);
	    if (statret == -1) {
		data->logsyserr("stat", fn);
		continue;
	    }
	    if (S_ISDIR(st.st_mode)) {
		if (data->options & FtwNoRecurse) {
		    status = cb.processone(fn, &st, FtwDirEnter);
		} else {
		    status=walk(fn, cb);
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

int main(int argc, const char **argv)
{
    if (argc != 2) {
	cerr << "Usage: fstreewalk <topdir>" << endl;
	exit(1);
    }
    FsTreeWalker walker;
    myCB cb;
    walker.walk(argv[1], cb);
    if (walker.getErrCnt() > 0)
	cout << walker.getReason();
}

#endif // TEST_FSTREEWALK
