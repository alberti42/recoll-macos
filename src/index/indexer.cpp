#ifndef lint
static char rcsid[] = "@(#$Id: indexer.cpp,v 1.28 2006-04-04 09:34:10 dockes Exp $ (C) 2004 J.F.Dockes";
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
    if (m_tmpdir.length()) {
	wipedir(m_tmpdir);
	if (rmdir(m_tmpdir.c_str()) < 0) {
	    LOGERR(("DbIndexer::~DbIndexer: cannot clear temp dir %s\n",
		    m_tmpdir.c_str()));
	}
    }
    m_db.close();
}

// Index each directory in the topdirs for a given db
bool DbIndexer::indexDb(bool resetbefore, list<string> *topdirs)
{
    if (!init(resetbefore))
	return false;

    for (list<string>::const_iterator it = topdirs->begin();
	 it != topdirs->end(); it++) {
	LOGDEB(("DbIndexer::index: Indexing %s into %s\n", it->c_str(), 
		m_dbdir.c_str()));

	// Set the current directory in config so that subsequent
	// getConfParams() will get local values
	m_config->setKeyDir(*it);

	// Set up skipped patterns for this subtree. This probably should be
	// done in the directory change code in processone() instead.
	m_walker.clearSkippedNames();
	string skipped; 
	if (m_config->getConfParam("skippedNames", skipped)) {
	    list<string> skpl;
	    stringToStrings(skipped, skpl);
	    m_walker.setSkippedNames(skpl);
	}

	// Walk the directory tree
	if (m_walker.walk(*it, *this) != FsTreeWalker::FtwOk) {
	    LOGERR(("DbIndexer::index: error while indexing %s\n", 
		    it->c_str()));
	    return false;
	}
    }

    // Get rid of all database entries that don't exist in the
    // filesystem anymore.
    m_db.purge();

    // Create stemming databases. We also remove those which are not
    // configured.
    string slangs;
    if (m_config->getConfParam("indexstemminglanguages", slangs)) {
	list<string> langs;
	stringToStrings(slangs, langs);

	// Get the list of existing stem dbs from the database (some may have 
	// been manually created, we just keep those from the config
	list<string> dblangs = m_db.getStemLangs();
	list<string>::const_iterator it;
	for (it = dblangs.begin(); it != dblangs.end(); it++) {
	    if (find(langs.begin(), langs.end(), *it) == langs.end())
		m_db.deleteStemDb(*it);
	}
	for (it = langs.begin(); it != langs.end(); it++) {
	    m_db.createStemDb(*it);
	}
    }

    // The close would be done in our destructor, but we want status here
    if (!m_db.close()) {
	LOGERR(("DbIndexer::index: error closing database in %s\n", 
		m_dbdir.c_str()));
	return false;
    }
    return true;
}

bool DbIndexer::init(bool resetbefore)
{
    if (!maketmpdir(m_tmpdir)) {
	LOGERR(("DbIndexer: cannot create temporary directory\n"));
	return false;
    }
    if (!m_db.open(m_dbdir, resetbefore ? Rcl::Db::DbTrunc : Rcl::Db::DbUpd)) {
	LOGERR(("DbIndexer: error opening database in %s\n", m_dbdir.c_str()));
	return false;
    }
    return true;
}

bool DbIndexer::createStemDb(const string &lang)
{
    if (!init())
	return false;
    return m_db.createStemDb(lang);
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
	m_config->setKeyDir(path_getfather(*it));
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
    if (!m_db.close()) {
	LOGERR(("DbIndexer::indexfiles: error closing database in %s\n", 
		m_dbdir.c_str()));
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
/// mostly contains pretty raw utf8 data.
FsTreeWalker::Status 
DbIndexer::processone(const std::string &fn, const struct stat *stp, 
		      FsTreeWalker::CbFlag flg)
{
    if (m_updfunc)
	m_updfunc->update(fn);
    // If we're changing directories, possibly adjust parameters (set
    // the current directory in configuration object)
    if (flg == FsTreeWalker::FtwDirEnter || 
	flg == FsTreeWalker::FtwDirReturn) {
	m_config->setKeyDir(fn);
	return FsTreeWalker::FtwOk;
    }

    // Check db up to date ? Doing this before file type
    // identification means that, if usesystemfilecommand is switched
    // from on to off it may happen that some files which are now
    // without mime type will not be purged from the db, resulting
    // into possible 'cannot intern file' messages at query time...
    if (!m_db.needUpdate(fn, stp)) {
	LOGDEB(("indexfile: up to date: %s\n", fn.c_str()));
	return FsTreeWalker::FtwOk;
    }

    FileInterner interner(fn, m_config, m_tmpdir);
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

	// File name transcoded to utf8 for indexation. 
	string charset = m_config->getDefCharset(true);
	// If this fails, the file name won't be indexed, no big deal
	// Note that we used to do the full path here, but I ended up believing
	// that it made more sense to use only the file name
	transcode(path_getsimple(fn), doc.utf8fn, charset, "UTF-8");
	// Do database-specific work to update document data
	if (!m_db.add(fn, doc, stp)) 
	    return FsTreeWalker::FtwError;
    }

    return FsTreeWalker::FtwOk;
}

////////////////////////////////////////////////////////////////////////////
// ConIndexer methods: ConfIndexer is the top-level object, that can index
// multiple directories to multiple databases.

ConfIndexer::~ConfIndexer()
{
     deleteZ(m_dbindexer);
}

bool ConfIndexer::index(bool resetbefore)
{
    // Retrieve the list of directories to be indexed.
    string topdirs;
    if (!m_config->getConfParam("topdirs", topdirs)) {
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
	m_config->setKeyDir(doctopdir);
	if (!m_config->getConfParam("dbdir", dbdir)) {
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
    m_config->setKeyDir("");

    // The dbmap now has dbdir as key and directory lists as values.
    // Index each directory group in turn
    for (dbit = dbmap.begin(); dbit != dbmap.end(); dbit++) {
	//cout << dbit->first << " -> ";
	//list<string>::const_iterator dit;
	//for (dit = dbit->second.begin(); dit != dbit->second.end(); dit++) {
	//    cout << *dit << " ";
	//}
	//cout << endl;
	m_dbindexer = new DbIndexer(m_config, dbit->first, m_updfunc);
	if (!m_dbindexer->indexDb(resetbefore, &dbit->second)) {
	    deleteZ(m_dbindexer);
	    return false;
	}
	deleteZ(m_dbindexer);
    }
    return true;
}
