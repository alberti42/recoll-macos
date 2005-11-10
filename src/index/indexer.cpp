#ifndef lint
static char rcsid[] = "@(#$Id: indexer.cpp,v 1.14 2005-11-10 08:47:49 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <strings.h>

#include <iostream>
#include <list>
#include <map>

#include "pathut.h"
#include "conftree.h"
#include "rclconfig.h"
#include "fstreewalk.h"
#include "rcldb.h"
#include "readfile.h"
#include "indexer.h"
#include "csguess.h"
#include "transcode.h"
#include "debuglog.h"
#include "internfile.h"
#include "smallut.h"
#include "wipedir.h"

using namespace std;

#ifndef deleteZ
#define deleteZ(X) {delete X;X = 0;}
#endif

/// A class to index a list of top directories into one database. 
///
/// Inherits FsTreeWalkerCB so that its processone() method is
/// called by the file-system tree walk code for each file and
/// directory, and keeps all state used while indexing a
/// directory tree. 
class DbIndexer : public FsTreeWalkerCB {
    FsTreeWalker walker;
    RclConfig *config;
    string dbdir;
    list<string> *topdirs;
    Rcl::Db db;
    string tmpdir;
 public:
    /// Constructor does nothing but store parameters
    DbIndexer(RclConfig *cnf, const string &dbd, list<string> *top) 
	: config(cnf), dbdir(dbd), topdirs(top) {}

    virtual ~DbIndexer() {
	// Maybe clean up temporary directory
	if (tmpdir.length()) {
	    wipedir(tmpdir);
	    if (rmdir(tmpdir.c_str()) < 0) {
		LOGERR(("DbIndexer::~DbIndexer: cant clear temp dir %s\n",
			tmpdir.c_str()));
	    }
	}
    }

    /// Start indexing.
    bool index();

    /// Tree walker callback method
    FsTreeWalker::Status 
    processone(const std::string &, const struct stat *, FsTreeWalker::CbFlag);
};


/// Top level file system tree index method for updating a given database.
///
/// We create the temporary directory, open the database, then call a
/// file system walk for each top-level directory.
/// When walking is done, we create the stem databases and close the
/// main db.
bool DbIndexer::index()
{
    string tdir;

    if (!maketmpdir(tmpdir)) {
	LOGERR(("DbIndexer: cant create temp directory\n"));
	return false;
    }
    if (!db.open(dbdir, Rcl::Db::DbUpd)) {
	LOGERR(("DbIndexer::index: error opening database in %s\n", 
		dbdir.c_str()));
	return false;
    }
    for (list<string>::const_iterator it = topdirs->begin();
	 it != topdirs->end(); it++) {
	LOGDEB(("DbIndexer::index: Indexing %s into %s\n", it->c_str(), 
		dbdir.c_str()));

	// Set the current directory in config so that subsequent
	// getConfParams() will get local values
	config->setKeyDir(*it);

	// Set up skipped patterns for this subtree. This probably should be
	// done in the directory change code in processone() instead.
	{
	    walker.clearSkippedNames();
	    string skipped; 
	    if (config->getConfParam("skippedNames", skipped)) {
		list<string> skpl;
		ConfTree::stringToStrings(skipped, skpl);
		list<string>::const_iterator it;
		for (it = skpl.begin(); it != skpl.end(); it++) {
		    walker.addSkippedName(*it);
		}
	    }
	}

	// Walk the directory tree
	if (walker.walk(*it, *this) != FsTreeWalker::FtwOk) {
	    LOGERR(("DbIndexer::index: error while indexing %s\n", 
		    it->c_str()));
	    db.close();
	    return false;
	}
    }

    // Get rid of all database entries that don't exist in the
    // filesystem anymore.
    db.purge();

    // Create stemming databases
    string slangs;
    if (config->getConfParam("indexstemminglanguages", slangs)) {
	list<string> langs;
	ConfTree::stringToStrings(slangs, langs);
	for (list<string>::const_iterator it = langs.begin(); 
	     it != langs.end(); it++) {
	    db.createStemDb(*it);
	}
    }

    if (!db.close()) {
	LOGERR(("DbIndexer::index: error closing database in %s\n", 
		dbdir.c_str()));
	return false;
    }
    return true;
}


/// This method gets called for every file and directory found by the
/// tree walker. 
///
/// It checks with the db if the file has changed and needs to be
/// reindexed. If so, it calls internfile() which will identify the
/// file type and call an appropriate handler to convert the document into
/// internal format, which we then add to the database.
///
/// Accent and majuscule handling are performed by the db module when doing
/// the actual indexing work. The Rcl::Doc created by internfile()
/// contains pretty raw utf8 data.
FsTreeWalker::Status 
DbIndexer::processone(const std::string &fn, const struct stat *stp, 
		      FsTreeWalker::CbFlag flg)
{
    // If we're changing directories, possibly adjust parameters (set
    // the current directory in configuration object)
    if (flg == FsTreeWalker::FtwDirEnter || 
	flg == FsTreeWalker::FtwDirReturn) {
	config->setKeyDir(fn);
	return FsTreeWalker::FtwOk;
    }

    // Check db up to date ?
    if (!db.needUpdate(fn, stp)) {
	LOGDEB(("indexfile: up to date: %s\n", fn.c_str()));
	return FsTreeWalker::FtwOk;
    }

    FileInterner interner(fn, config, tmpdir);
    FileInterner::Status fis = FileInterner::FIAgain;
    while (fis == FileInterner::FIAgain) {
	Rcl::Doc doc;
	string ipath;
	fis = interner.internfile(doc, ipath);
	if (fis == FileInterner::FIError)
	    break;

	// Set the date if this was not done in the document handler
	if (doc.fmtime.empty()) {
	    char ascdate[20];
	    sprintf(ascdate, "%ld", long(stp->st_ctime));
	    doc.fmtime = ascdate;
	}
	// Internal access path for multi-document files
	doc.ipath = ipath;

	// Do database-specific work to update document data
	if (!db.add(fn, doc)) 
	    return FsTreeWalker::FtwError;
    }

    return FsTreeWalker::FtwOk;
}

////////////////////////////////////////////////////////////////////////////
// ConIndexer methods: ConfIndexer is the top-level object, that can index
// multiple directories to multiple databases.

ConfIndexer::~ConfIndexer()
{
     deleteZ(dbindexer);
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
    list<string> tdl; // List of directories to be indexed
    if (!ConfTree::stringToStrings(topdirs, tdl)) {
	LOGERR(("ConfIndexer::index: parse error for directory list\n"));
	return false;
    }

    // Each top level directory to be indexed can be associated with a
    // different database. We first group the directories by database:
    // it is important that all directories for a database be indexed
    // at once so that deleted file cleanup works
    list<string>::iterator dirit;
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

    // Index each directory group in turn
    for (dbit = dbmap.begin(); dbit != dbmap.end(); dbit++) {
	//cout << dbit->first << " -> ";
	//list<string>::const_iterator dit;
	//for (dit = dbit->second.begin(); dit != dbit->second.end(); dit++) {
	//    cout << *dit << " ";
	//}
	//cout << endl;

	dbindexer = new DbIndexer(config, dbit->first, &dbit->second);
	if (!dbindexer->index()) {
	    deleteZ(dbindexer);
	    return false;
	}
	deleteZ(dbindexer);
    }
    return true;
}
