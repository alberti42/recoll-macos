#ifndef lint
static char rcsid[] = "@(#$Id: recollindex.cpp,v 1.1 2004-12-13 15:42:16 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <iostream>

#include "pathut.h"
#include "conftree.h"
#include "rclconfig.h"
#include "fstreewalk.h"
#include "mimetype.h"

using namespace std;

class DirIndexer {
    FsTreeWalker walker;
    RclConfig *config;
    string topdir;
 public:
    DirIndexer(RclConfig *cnf, const string &top) 
	: config(cnf), topdir(top)
    {
    }
    friend FsTreeWalker::Status 
      indexfile(void *, const std::string &, const struct stat *, 
		FsTreeWalker::CbFlag);
    void index()
    {
	walker.walk(topdir, indexfile, this);
    }
};

FsTreeWalker::Status 
indexfile(void *cdata, const std::string &fn, 
	  const struct stat *stp, FsTreeWalker::CbFlag flg)
{
    DirIndexer *me = (DirIndexer *)cdata;
    if (flg == FsTreeWalker::FtwDirEnter || flg == FsTreeWalker::FtwDirReturn) {
	// Possibly adjust defaults
	cout << "indexfile: [" << fn << "]" << endl;
	return FsTreeWalker::FtwOk;
    }
    string mtype = mimetype(fn, me->config->getMimeMap());
    if (mtype.length() > 0) 
	cout << "indexfile: " << mtype << " " << fn << endl;
    else
	cout << "indexfile: " << "(nomime)" << " " << fn << endl;

}


int main(int argc, const char **argv)
{
    RclConfig *config = new RclConfig;

    if (!config->ok())
	cerr << "Config could not be built" << endl;

    ConfTree *conf = config->getConfig();
    
    string topdirs;
    if (conf->get("topdirs", topdirs, "") == 0) {
	cerr << "No top directories in configuration" << endl;
	exit(1);
    }
    list<string> tdl;
    if (ConfTree::stringToStrings(topdirs, tdl)) {
	for (list<string>::iterator it = tdl.begin(); it != tdl.end(); it++) {
	    cout << *it << endl;
	    DirIndexer indexer(config, *it);
	    indexer.index();
	}
    }
}
