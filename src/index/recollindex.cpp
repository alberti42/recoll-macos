#ifndef lint
static char rcsid[] = "@(#$Id: recollindex.cpp,v 1.7 2005-01-29 15:41:11 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <sys/stat.h>

#include <strings.h>

#include <iostream>
#include <list>
#include <map>

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
#include "debuglog.h"

using namespace std;


/**
 * Bunch holder for data used while indexing a directory tree
 */
class DirIndexer {
    FsTreeWalker walker;
    RclConfig *config;
    list<string> *topdirs;
    string dbdir;
    Rcl::Db db;
 public:
    DirIndexer(RclConfig *cnf, const string &dbd, list<string> *top) 
	: config(cnf), topdirs(top), dbdir(dbd)
    { }

    friend FsTreeWalker::Status 
      indexfile(void *, const std::string &, const struct stat *, 
		FsTreeWalker::CbFlag);

    bool index();
};

bool DirIndexer::index()
{
    if (!db.open(dbdir, Rcl::Db::DbUpd)) {
	LOGERR(("DirIndexer::index: error opening database in %s\n", 
		dbdir.c_str()));
	return false;
    }
    for (list<string>::const_iterator it = topdirs->begin();
	 it != topdirs->end(); it++) {
	LOGDEB(("DirIndexer::index: Indexing %s into %s\n", it->c_str(), 
		dbdir.c_str()));
	if (walker.walk(*it, indexfile, this) != FsTreeWalker::FtwOk) {
	    LOGERR(("DirIndexer::index: error while indexing %s\n", 
		    it->c_str()));
	    db.close();
	    return false;
	}
    }
    db.purge();
    if (!db.close()) {
	LOGERR(("DirIndexer::index: error closing database in %s\n", 
		dbdir.c_str()));
	return false;
    }
    return true;
}

/** 
 * This function gets called for every file and directory found by the
 * tree walker. It checks with the db if the file has changed and needs to
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
	return FsTreeWalker::FtwOk;
    }

    string mime = mimetype(fn, me->config->getMimeMap());
    if (mime.length() == 0) {
	LOGDEB(("indexfile: (no mime) [%s]\n", fn.c_str()));
	// No mime type ?? pass on.
	return FsTreeWalker::FtwOk;
    }

    LOGDEB(("indexfile: %s [%s]\n", mime.c_str(), fn.c_str()));

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

DirIndexer *indexer;

static void cleanup()
{
    delete indexer;
    indexer = 0;
}

static void sigcleanup(int sig)
{
    fprintf(stderr, "sigcleanup\n");
    cleanup();
    exit(1);
}

int main(int argc, const char **argv)
{
    atexit(cleanup);
    if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
	signal(SIGHUP, sigcleanup);
    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	signal(SIGINT, sigcleanup);
    if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
	signal(SIGQUIT, sigcleanup);
    if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
	signal(SIGTERM, sigcleanup);

    RclConfig config;
    if (!config.ok())
	cerr << "Config could not be built" << endl;

    ConfTree *conf = config.getConfig();

    // Retrieve the list of directories to be indexed.
    string topdirs;
    if (conf->get("topdirs", topdirs, "") == 0) {
	cerr << "No top directories in configuration" << endl;
	exit(1);
    }

    // Group the directories by database: it is important that all
    // directories for a database be indexed at once so that deleted
    // file cleanup works 
    vector<string> tdl; // List of directories to be indexed
    if (!ConfTree::stringToStrings(topdirs, tdl)) {
	cerr << "Parse error for directory list" << endl;
	exit(1);
    }

    vector<string>::iterator dirit;
    map<string, list<string> > dbmap;
    map<string, list<string> >::iterator dbit;
    for (dirit = tdl.begin(); dirit != tdl.end(); dirit++) {
	string db;
	if (conf->get("dbdir", db, *dirit) == 0) {
	    cerr << "No database directory in configuration for " 
		 << *dirit << endl;
	    exit(1);
	}
	dbit = dbmap.find(db);
	if (dbit == dbmap.end()) {
	    list<string> l;
	    l.push_back(*dirit);
	    dbmap[db] = l;
	} else {
	    dbit->second.push_back(*dirit);
	}
    }

    for (dbit = dbmap.begin(); dbit != dbmap.end(); dbit++) {
	cout << dbit->first << " -> ";
	list<string>::const_iterator dit;
	for (dit = dbit->second.begin(); dit != dbit->second.end(); dit++) {
	    cout << *dit << " ";
	}
	cout << endl;
	indexer = new DirIndexer(&config, dbit->first, &dbit->second);
	if (!indexer->index()) {
	    delete indexer;
	    indexer = 0;
	    exit(1);
	}
	delete indexer;
	indexer = 0;
    }
}
