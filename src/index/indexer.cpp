#ifndef lint
static char rcsid[] = "@(#$Id: indexer.cpp,v 1.21 2006-01-09 16:53:31 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>

#include <iostream>
#include <list>
#include <map>
#include <algorithm>

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

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

#ifndef deleteZ
#define deleteZ(X) {delete X;X = 0;}
#endif

DbIndexer::~DbIndexer() {
    // Maybe clean up temporary directory
    if (tmpdir.length()) {
	wipedir(tmpdir);
	if (rmdir(tmpdir.c_str()) < 0) {
	    LOGERR(("DbIndexer::~DbIndexer: cannot clear temp dir %s\n",
		    tmpdir.c_str()));
	}
    }
    db.close();
}


bool DbIndexer::indexDb(bool resetbefore, list<string> *topdirs)
{
    if (!init(resetbefore))
	return false;

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
		stringToStrings(skipped, skpl);
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
	    return false;
	}
    }

    // Get rid of all database entries that don't exist in the
    // filesystem anymore.
    db.purge();

    // Create stemming databases. We also remove those which are not
    // configured.
    string slangs;
    if (config->getConfParam("indexstemminglanguages", slangs)) {
	list<string> langs;
	stringToStrings(slangs, langs);

	// Get the list of existing stem dbs from the database (some may have 
	// been manually created, we just keep those from the config
	list<string> dblangs = db.getStemLangs();
	list<string>::const_iterator it;
	for (it = dblangs.begin(); it != dblangs.end(); it++) {
	    if (find(langs.begin(), langs.end(), *it) == langs.end())
		db.deleteStemDb(*it);
	}
	for (it = langs.begin(); it != langs.end(); it++) {
	    db.createStemDb(*it);
	}
    }

    // The close would be done in our destructor, but we want status here
    if (!db.close()) {
	LOGERR(("DbIndexer::index: error closing database in %s\n", 
		dbdir.c_str()));
	return false;
    }
    return true;
}

bool DbIndexer::init(bool resetbefore)
{
    if (!maketmpdir(tmpdir)) {
	LOGERR(("DbIndexer: cannot create temporary directory\n"));
	return false;
    }
    if (!db.open(dbdir, resetbefore ? Rcl::Db::DbTrunc : Rcl::Db::DbUpd)) {
	LOGERR(("DbIndexer: error opening database in %s\n", dbdir.c_str()));
	return false;
    }
    return true;
}

bool DbIndexer::createStemDb(const string &lang)
{
    if (!init())
	return false;
    return db.createStemDb(lang);
}

/** 
 Index individual files, out of a full tree run. No database purging
*/
bool DbIndexer::indexFiles(const list<string> &filenames)
{
    if (!init())
	return false;

    list<string>::const_iterator it;
    for (it = filenames.begin(); it != filenames.end();it++) {
	config->setKeyDir(path_getfather(*it));
	struct stat stb;
	if (stat(it->c_str(), &stb) != 0) {
	    LOGERR(("DbIndexer::indexFiles: stat(%s): %s", it->c_str(),
		    strerror(errno)));
	    continue;
	}
	if (!S_ISREG(stb.st_mode)) {
	    LOGERR(("DbIndexer::indexFiles: %s: not a regular file\n", 
		    it->c_str()));
	    continue;
	}
	if (processone(*it, &stb, FsTreeWalker::FtwRegular) != 
	    FsTreeWalker::FtwOk) {
	    LOGERR(("DbIndexer::indexFiles: Database error\n"));
	    return false;
	}
    }
    // The close would be done in our destructor, but we want status here
    if (!db.close()) {
	LOGERR(("DbIndexer::indexfiles: error closing database in %s\n", 
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

    // Check db up to date ? Doing this before file type
    // identification means that, if usesystemfilecommand is switched
    // from on to off it may happen that some files which are now
    // without mime type will not be purged from the db, resulting
    // into possible 'cannot intern file' messages at query time...
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

bool ConfIndexer::index(bool resetbefore)
{
    // Retrieve the list of directories to be indexed.
    string topdirs;
    if (!config->getConfParam("topdirs", topdirs)) {
	LOGERR(("ConfIndexer::index: no top directories in configuration\n"));
	return false;
    }
    list<string> tdl; // List of directories to be indexed
    if (!stringToStrings(topdirs, tdl)) {
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
	string dbdir;
	string doctopdir = path_tildexpand(*dirit);
	config->setKeyDir(doctopdir);
	if (!config->getConfParam("dbdir", dbdir)) {
	    LOGERR(("ConfIndexer::index: no database directory in "
		    "configuration for %s\n", doctopdir.c_str()));
	    return false;
	}
	dbdir = path_tildexpand(dbdir);
	dbit = dbmap.find(dbdir);
	if (dbit == dbmap.end()) {
	    list<string> l;
	    l.push_back(doctopdir);
	    dbmap[dbdir] = l;
	} else {
	    dbit->second.push_back(doctopdir);
	}
    }
    config->setKeyDir("");

    // The dbmap now has dbdir as key and directory lists as values.
    // Index each directory group in turn
    for (dbit = dbmap.begin(); dbit != dbmap.end(); dbit++) {
	//cout << dbit->first << " -> ";
	//list<string>::const_iterator dit;
	//for (dit = dbit->second.begin(); dit != dbit->second.end(); dit++) {
	//    cout << *dit << " ";
	//}
	//cout << endl;
	dbindexer = new DbIndexer(config, dbit->first);
	if (!dbindexer->indexDb(resetbefore, &dbit->second)) {
	    deleteZ(dbindexer);
	    return false;
	}
	deleteZ(dbindexer);
    }
    return true;
}
