#ifndef lint
static char rcsid[] = "@(#$Id: recollindex.cpp,v 1.5 2005-01-25 14:37:21 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <sys/stat.h>

#include <strings.h>

#include <iostream>

#include "pathut.h"
#include "conftree.h"
#include "rclconfig.h"
#include "fstreewalk.h"
#include "mimetype.h"
#include "rcldb.h"
#include "readfile.h"
#include "indexer.h"
#include "csguess.h"
#include "transcode.h"
#include "mimehandler.h"

using namespace std;


/**
 * Bunch holder for data used while indexing a directory tree
 */
class DirIndexer {
    FsTreeWalker walker;
    RclConfig *config;
    string topdir;
    string dbdir;
    Rcl::Db db;
 public:
    DirIndexer(RclConfig *cnf, const string &dbd, const string &top) 
	: config(cnf), topdir(top), dbdir(dbd)
    { }

    friend FsTreeWalker::Status 
      indexfile(void *, const std::string &, const struct stat *, 
		FsTreeWalker::CbFlag);

    void index();
};

void DirIndexer::index()
{
    if (!db.open(dbdir, Rcl::Db::DbUpd)) {
	cerr << "Error opening database in " << dbdir << " for " <<
	    topdir << endl;
	return;
    }
    walker.walk(topdir, indexfile, this);
    if (!db.close()) {
	cerr << "Error closing database in " << dbdir << " for " <<
	    topdir << endl;
	return;
    }
}

/** 
 * This function gets called for every file and directory found by the
 * tree walker. It checks with the db is the file has changed and needs to
 * be reindexed. If so, it calls an appropriate handler depending on the mime
 * type, which is responsible for populating an Rcl::Doc.
 * Accent and majuscule handling are performed by the db module when doing
 * the actual indexing work.
 */
FsTreeWalker::Status 
indexfile(void *cdata, const std::string &fn, const struct stat *stp, 
	  FsTreeWalker::CbFlag flg)
{
    DirIndexer *me = (DirIndexer *)cdata;

    if (flg == FsTreeWalker::FtwDirEnter || 
	flg == FsTreeWalker::FtwDirReturn) {
	me->config->setKeyDir(fn);
	cout << "indexfile: [" << fn << "]" << endl;
	cout << "   defcharset: " << me->config->getDefCharset()
	     << " deflang: " << me->config->getDefLang() << endl;

	return FsTreeWalker::FtwOk;
    }

    string mime = mimetype(fn, me->config->getMimeMap());
    if (mime.length() == 0) {
	cout << "indexfile: " << "(no mime)" << " " << fn << endl;
	// No mime type ?? pass on.
	return FsTreeWalker::FtwOk;
    }

    cout << "indexfile: " << mime << " " << fn << endl;

    // Look for appropriate handler
    MimeHandlerFunc fun = getMimeHandler(mime, me->config->getMimeConf());
    if (!fun) {
	// No handler for this type, for now :(
	return FsTreeWalker::FtwOk;
    }

    if (!me->db.needUpdate(fn, stp))
	return FsTreeWalker::FtwOk;

    // Turn file into a document. The document has fields for title, body 
    // etc.,  all text converted to utf8
    Rcl::Doc doc;
    if (!fun(me->config, fn,  mime, doc))
	return FsTreeWalker::FtwOk;

    // Set up common fields:
    doc.mimetype = mime;
    char ascdate[20];
    sprintf(ascdate, "%ld", long(stp->st_mtime));
    doc.mtime = ascdate;

    // Set up xapian document, add postings and misc fields, 
    // add to or update database.
    if (!me->db.add(fn, doc))
	return FsTreeWalker::FtwError;

    return FsTreeWalker::FtwOk;
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
    vector<string> tdl;
    if (ConfTree::stringToStrings(topdirs, tdl)) {
	for (unsigned int i = 0; i < tdl.size(); i++) {
	    string topdir = tdl[i];
	    cout << topdir << endl;
	    string dbdir;
	    if (conf->get("dbdir", dbdir, topdir) == 0) {
		cerr << "No database directory in configuration for " 
		     << topdir << endl;
		exit(1);
	    }
	    DirIndexer indexer(config, dbdir, topdir);
	    indexer.index();
	}
    }
}
