#ifndef lint
static char rcsid[] = "@(#$Id: indexer.cpp,v 1.1 2005-01-31 14:31:09 dockes Exp $ (C) 2004 J.F.Dockes";
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

#ifndef deleteZ
#define deleteZ(X) {delete X;X = 0;}
#endif

/**
 * Bunch holder for data used while indexing a directory tree
 */
class DbIndexer {
    FsTreeWalker walker;
    RclConfig *config;
    string dbdir;
    list<string> *topdirs;
    Rcl::Db db;
 public:
    DbIndexer(RclConfig *cnf, const string &dbd, list<string> *top) 
	: config(cnf), dbdir(dbd), topdirs(top)
    { }

    friend FsTreeWalker::Status 
      indexfile(void *, const std::string &, const struct stat *, 
		FsTreeWalker::CbFlag);

    bool index();
};

bool DbIndexer::index()
{
    if (!db.open(dbdir, Rcl::Db::DbUpd)) {
	LOGERR(("DbIndexer::index: error opening database in %s\n", 
		dbdir.c_str()));
	return false;
    }
    for (list<string>::const_iterator it = topdirs->begin();
	 it != topdirs->end(); it++) {
	LOGDEB(("DbIndexer::index: Indexing %s into %s\n", it->c_str(), 
		dbdir.c_str()));
	if (walker.walk(*it, indexfile, this) != FsTreeWalker::FtwOk) {
	    LOGERR(("DbIndexer::index: error while indexing %s\n", 
		    it->c_str()));
	    db.close();
	    return false;
	}
    }
    db.purge();
    if (!db.close()) {
	LOGERR(("DbIndexer::index: error closing database in %s\n", 
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
    DbIndexer *me = (DbIndexer *)cdata;

    // If we're changing directories, possibly adjust parameters.
    if (flg == FsTreeWalker::FtwDirEnter || 
	flg == FsTreeWalker::FtwDirReturn) {
	me->config->setKeyDir(fn);
	return FsTreeWalker::FtwOk;
    }

    string mime = mimetype(fn, me->config->getMimeMap());
    if (mime.empty()) {
	// No mime type ?? pass on.
	LOGDEB(("indexfile: (no mime) [%s]\n", fn.c_str()));
	return FsTreeWalker::FtwOk;
    }

    // Look for appropriate handler
    MimeHandlerFunc fun = getMimeHandler(mime, me->config->getMimeConf());
    if (!fun) {
	// No handler for this type, for now :(
	LOGDEB(("indexfile: %s : no handler\n", mime.c_str()));
	return FsTreeWalker::FtwOk;
    }

    LOGDEB(("indexfile: %s [%s]\n", mime.c_str(), fn.c_str()));

    // Check db up to date ?
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

    // Do database-specific work to update document data
    if (!me->db.add(fn, doc))
	return FsTreeWalker::FtwError;

    return FsTreeWalker::FtwOk;
}

ConfIndexer::~ConfIndexer() 
{
    deleteZ(indexer);
}

bool ConfIndexer::index()
{
    ConfTree *conf = config->getConfig();

    // Retrieve the list of directories to be indexed.
    string topdirs;
    if (conf->get("topdirs", topdirs, "") == 0) {
	LOGERR(("ConfIndexer::index: no top directories in configuration\n"));
	return false;
    }

    // Group the directories by database: it is important that all
    // directories for a database be indexed at once so that deleted
    // file cleanup works 
    vector<string> tdl; // List of directories to be indexed
    if (!ConfTree::stringToStrings(topdirs, tdl)) {
	LOGERR(("ConfIndexer::index: parse error for directory list\n"));
	return false;
    }

    vector<string>::iterator dirit;
    map<string, list<string> > dbmap;
    map<string, list<string> >::iterator dbit;
    for (dirit = tdl.begin(); dirit != tdl.end(); dirit++) {
	string db;
	string dir = path_tildexpand(*dirit);
	if (conf->get("dbdir", db, dir) == 0) {
	    LOGERR(("ConfIndexer::index: no database directory in "
		    "configuration for %s\n", dir.c_str()));
	    return false;
	}
	db = path_tildexpand(db);
	dbit = dbmap.find(db);
	if (dbit == dbmap.end()) {
	    list<string> l;
	    l.push_back(dir);
	    dbmap[db] = l;
	} else {
	    dbit->second.push_back(dir);
	}
    }

    for (dbit = dbmap.begin(); dbit != dbmap.end(); dbit++) {
	//cout << dbit->first << " -> ";
	//list<string>::const_iterator dit;
	//for (dit = dbit->second.begin(); dit != dbit->second.end(); dit++) {
	//    cout << *dit << " ";
	//}
	//cout << endl;

	indexer = new DbIndexer(config, dbit->first, &dbit->second);
	if (!indexer->index()) {
	    deleteZ(indexer);
	    return false;
	}
	deleteZ(indexer);
    }
    return true;
}
