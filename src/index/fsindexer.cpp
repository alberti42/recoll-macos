#ifndef lint
static char rcsid[] = "@(#$Id: $ (C) 2009 J.F.Dockes";
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
#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <fnmatch.h>

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
#include "fsindexer.h"
#include "csguess.h"
#include "transcode.h"
#include "debuglog.h"
#include "internfile.h"
#include "smallut.h"
#include "wipedir.h"
#include "fileudi.h"

// When using extended attributes, we have to use the ctime. 
// This is quite an expensive price to pay...
#ifdef RCL_USE_XATTR
#define RCL_STTIME st_ctime
#else
#define RCL_STTIME st_mtime
#endif // RCL_USE_XATTR

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

#ifndef deleteZ
#define deleteZ(X) {delete X;X = 0;}
#endif

FsIndexer::~FsIndexer() {
    // Maybe clean up temporary directory
    if (!m_tmpdir.empty()) {
	wipedir(m_tmpdir);
	if (rmdir(m_tmpdir.c_str()) < 0) {
	    LOGERR(("FsIndexer::~FsIndexer: cannot clear temp dir %s\n",
		    m_tmpdir.c_str()));
	}
    }
}

bool FsIndexer::init()
{
    if (m_tmpdir.empty() || access(m_tmpdir.c_str(), 0) < 0) {
	if (!maketmpdir(m_tmpdir, m_reason)) {
	    LOGERR(("FsIndexer: cannot create temporary directory: %s\n",
		    m_reason.c_str()));
            m_tmpdir.erase();
	    return false;
	}
    }
    return true;
}

// Recursively index each directory in the topdirs:
bool FsIndexer::index(bool resetbefore)
{
    list<string> topdirs = m_config->getTopdirs();
    if (topdirs.empty()) {
        LOGERR(("FsIndexer::indexTrees: no valid topdirs in config\n"));
        return false;
    }

    if (!init())
	return false;

    if (m_updater) {
	m_updater->status.reset();
	m_updater->status.dbtotdocs = m_db->docCnt();
    }

    m_walker.setSkippedPaths(m_config->getSkippedPaths());

    for (list<string>::const_iterator it = topdirs.begin();
	 it != topdirs.end(); it++) {
	LOGDEB(("FsIndexer::index: Indexing %s into %s\n", it->c_str(), 
		getDbDir().c_str()));

	// Set the current directory in config so that subsequent
	// getConfParams() will get local values
	m_config->setKeyDir(*it);

	// Adjust the "follow symlinks" option
	bool follow;
	if (m_config->getConfParam("followLinks", &follow) && follow) {
	    m_walker.setOpts(FsTreeWalker::FtwFollow);
	} else {
	    m_walker.setOpts(FsTreeWalker::FtwOptNone);
	}	    

	int abslen;
	if (m_config->getConfParam("idxabsmlen", &abslen))
	    m_db->setAbstractParams(abslen, -1, -1);

	// Set up skipped patterns for this subtree. This probably should be
	// done in the directory change code in processone() instead.
	m_walker.setSkippedNames(m_config->getSkippedNames());

	// Walk the directory tree
	if (m_walker.walk(*it, *this) != FsTreeWalker::FtwOk) {
	    LOGERR(("FsIndexer::index: error while indexing %s: %s\n", 
		    it->c_str(), m_walker.getReason().c_str()));
	    return false;
	}
    }

    string missing;
    FileInterner::getMissingDescription(missing);
    if (!missing.empty()) {
	LOGINFO(("FsIndexer::index missing helper program(s):\n%s\n", 
		 missing.c_str()));
    }
    m_config->storeMissingHelperDesc(missing);
    return true;
}

/** 
 * Index individual files, out of a full tree run. No database purging
 */
bool FsIndexer::indexFiles(const list<string> &filenames)
{
    if (!init())
        return false;

    list<string>::const_iterator it;
    for (it = filenames.begin(); it != filenames.end(); it++) {
	string dir = path_getfather(*it);
	m_config->setKeyDir(dir);
	int abslen;
	if (m_config->getConfParam("idxabsmlen", &abslen))
	    m_db->setAbstractParams(abslen, -1, -1);
	struct stat stb;
	if (lstat(it->c_str(), &stb) != 0) {
	    LOGERR(("FsIndexer::indexFiles: lstat(%s): %s", it->c_str(),
		    strerror(errno)));
	    continue;
	}

	// If we get to indexing directory names one day, will need to test 
	// against dbdir here to avoid modification loops (with rclmon).
	if (!S_ISREG(stb.st_mode)) {
	    LOGDEB2(("FsIndexer::indexFiles: %s: not a regular file\n", 
		    it->c_str()));
	    continue;
	}

	static string lstdir;
	static list<string> skpl;
	if (lstdir.compare(dir)) {
	    LOGDEB(("Recomputing list of skipped names\n"));
	    skpl = m_config->getSkippedNames();
	    lstdir = dir;
	}
	if (!skpl.empty()) {
	    list<string>::const_iterator skit;
	    string fn = path_getsimple(*it);
	    for (skit = skpl.begin(); skit != skpl.end(); skit++) {
		if (fnmatch(skit->c_str(), fn.c_str(), 0) == 0) {
		    LOGDEB(("Skipping [%s] :matches skip list\n", fn.c_str()));
		    goto skipped;
		}
	    }
	}
	if (processone(*it, &stb, FsTreeWalker::FtwRegular) != 
	    FsTreeWalker::FtwOk) {
	    LOGERR(("FsIndexer::indexFiles: processone failed\n"));
	    return false;
	}
    skipped: 
	false; // Need a statement here to make compiler happy ??
    }

    return true;
}


/** Purge docs for given files out of the database */
bool FsIndexer::purgeFiles(const list<string> &filenames)
{
    if (!init())
	return false;

    list<string>::const_iterator it;
    for (it = filenames.begin(); it != filenames.end(); it++) {
	string udi;
	make_udi(*it, "", udi);
	if (!m_db->purgeFile(udi)) {
	    LOGERR(("FsIndexer::purgeFiles: Database error\n"));
	    return false;
	}
    }

    return true;
}

// Local fields can be set for fs subtrees in the configuration file 
void FsIndexer::localfieldsfromconf()
{
    LOGDEB(("FsIndexer::localfieldsfromconf\n"));
    m_localfields.clear();
    string sfields;
    if (!m_config->getConfParam("localfields", sfields))
        return;
    list<string> lfields;
    if (!stringToStrings(sfields, lfields)) {
        LOGERR(("FsIndexer::localfieldsfromconf: bad syntax for [%s]\n", 
                sfields.c_str()));
        return;
    }
    for (list<string>::const_iterator it = lfields.begin();
         it != lfields.end(); it++) {
        ConfSimple conf(*it, 1, true);
        list<string> nmlst = conf.getNames("");
        for (list<string>::const_iterator it1 = nmlst.begin();
             it1 != nmlst.end(); it1++) {
            conf.get(*it1, m_localfields[*it1]);
            LOGDEB2(("FsIndexer::localfieldsfromconf: [%s] => [%s]\n",
                    (*it1).c_str(), m_localfields[*it1].c_str()));
        }
    }
}

// 
void FsIndexer::setlocalfields(Rcl::Doc& doc)
{
    for (map<string, string>::const_iterator it = m_localfields.begin();
         it != m_localfields.end(); it++) {
        // Should local fields override those coming from the document
        // ? I think not, but not too sure
        if (doc.meta.find(it->second) == doc.meta.end()) {
            doc.meta[it->first] = it->second;
        }
    }
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
FsIndexer::processone(const std::string &fn, const struct stat *stp, 
		      FsTreeWalker::CbFlag flg)
{
    if (m_updater && !m_updater->update()) {
        return FsTreeWalker::FtwStop;
    }

    // If we're changing directories, possibly adjust parameters (set
    // the current directory in configuration object)
    if (flg == FsTreeWalker::FtwDirEnter || 
	flg == FsTreeWalker::FtwDirReturn) {
	m_config->setKeyDir(fn);

	int abslen;
	if (m_config->getConfParam("idxabsmlen", &abslen))
	    m_db->setAbstractParams(abslen, -1, -1);

        // Adjust local fields from config for this subtree
        if (m_havelocalfields)
            localfieldsfromconf();

	if (flg == FsTreeWalker::FtwDirReturn)
	    return FsTreeWalker::FtwOk;
    }

    ////////////////////
    // Check db up to date ? Doing this before file type
    // identification means that, if usesystemfilecommand is switched
    // from on to off it may happen that some files which are now
    // without mime type will not be purged from the db, resulting
    // in possible 'cannot intern file' messages at query time...

    // Document signature. This is based on m/ctime and size and used
    // for the uptodate check (the value computed here is checked
    // against the stored one). Changing the computation forces a full
    // reindex of course.
    char cbuf[100]; 
    sprintf(cbuf, "%ld%ld", (long)stp->st_size, (long)stp->RCL_STTIME);
    string sig = cbuf;
    string udi;
    make_udi(fn, "", udi);
    if (!m_db->needUpdate(udi, sig)) {
	LOGDEB(("processone: up to date: %s\n", fn.c_str()));
	if (m_updater) {
	    // Status bar update, abort request etc.
	    m_updater->status.fn = fn;
	    if (!m_updater->update()) {
		return FsTreeWalker::FtwStop;
	    }
	}
	return FsTreeWalker::FtwOk;
    }

    LOGDEB0(("processone: processing: [%s] %s\n", 
             displayableBytes(stp->st_size).c_str(), fn.c_str()));

    FileInterner interner(fn, stp, m_config, m_tmpdir, FileInterner::FIF_none);

    // File name transcoded to utf8 for indexation. 
    string charset = m_config->getDefCharset(true);
    // If this fails, the file name won't be indexed, no big deal
    // Note that we used to do the full path here, but I ended up believing
    // that it made more sense to use only the file name
    string utf8fn; int ercnt;
    if (!transcode(path_getsimple(fn), utf8fn, charset, "UTF-8", &ercnt)) {
	LOGERR(("processone: fn transcode failure from [%s] to UTF-8: %s\n",
		charset.c_str(), path_getsimple(fn).c_str()));
    } else if (ercnt) {
	LOGDEB(("processone: fn transcode %d errors from [%s] to UTF-8: %s\n",
		ercnt, charset.c_str(), path_getsimple(fn).c_str()));
    }
    LOGDEB2(("processone: fn transcoded from [%s] to [%s] (%s->%s)\n",
	     path_getsimple(fn).c_str(), utf8fn.c_str(), charset.c_str(), 
	     "UTF-8"));

    string parent_udi;
    make_udi(fn, "", parent_udi);
    Rcl::Doc doc;
    const string plus("+");
    char ascdate[20];
    sprintf(ascdate, "%ld", long(stp->st_mtime));

    FileInterner::Status fis = FileInterner::FIAgain;
    bool hadNullIpath = false;
    while (fis == FileInterner::FIAgain) {
	doc.erase();
	string ipath;
	fis = interner.internfile(doc, ipath);

        // Index at least the file name even if there was an error.
        // We'll change the signature to ensure that the indexing will
        // be retried every time.


	// Internal access path for multi-document files
	if (ipath.empty())
	    hadNullIpath = true;
	else
	    doc.ipath = ipath;

	// Set file name, mod time and url if not done by filter
	if (doc.fmtime.empty())
	    doc.fmtime = ascdate;
        if (doc.url.empty())
            doc.url = string("file://") + fn;
	if (doc.utf8fn.empty())
	    doc.utf8fn = utf8fn;

	char cbuf[100]; 
	sprintf(cbuf, "%ld", (long)stp->st_size);
	doc.fbytes = cbuf;
	// Document signature for up to date checks: concatenate
	// m/ctime and size. Looking for changes only, no need to
	// parseback so no need for reversible formatting. Also set,
	// but never used, for subdocs.
	sprintf(cbuf, "%ld%ld", (long)stp->st_size, (long)stp->RCL_STTIME);
	doc.sig = cbuf;
	// If there was an error, ensure indexing will be
	// retried. This is for the once missing, later installed
	// filter case. It can make indexing much slower (if there are
	// myriads of such files, the ext script is executed for them
	// and fails every time)
	if (fis == FileInterner::FIError) {
	    doc.sig += plus;
	}

        // Possibly add fields from local config
        if (m_havelocalfields) 
            setlocalfields(doc);
	// Add document to database. If there is an ipath, add it as a children
	// of the file document.
	string udi;
	make_udi(fn, ipath, udi);
	if (!m_db->addOrUpdate(udi, ipath.empty() ? "" : parent_udi, doc)) 
	    return FsTreeWalker::FtwError;

	// Tell what we are doing and check for interrupt request
	if (m_updater) {
	    ++(m_updater->status.docsdone);
            m_updater->status.fn = fn;
            if (!ipath.empty())
                m_updater->status.fn += "|" + ipath;
            if (!m_updater->update()) {
                return FsTreeWalker::FtwStop;
            }
	}
    }

    // If we had no instance with a null ipath, we create an empty
    // document to stand for the file itself, to be used mainly for up
    // to date checks. Typically this happens for an mbox file.
    if (hadNullIpath == false) {
	LOGDEB1(("Creating empty doc for file\n"));
	Rcl::Doc fileDoc;
	fileDoc.fmtime = ascdate;
	fileDoc.utf8fn = utf8fn;
	fileDoc.mimetype = interner.getMimetype();
	fileDoc.url = string("file://") + fn;

	char cbuf[100]; 
	sprintf(cbuf, "%ld", (long)stp->st_size);
	fileDoc.fbytes = cbuf;
	// Document signature for up to date checks.
	sprintf(cbuf, "%ld%ld", (long)stp->st_size, (long)stp->RCL_STTIME);
	fileDoc.sig = cbuf;
	if (!m_db->addOrUpdate(parent_udi, "", fileDoc)) 
	    return FsTreeWalker::FtwError;
    }

    return FsTreeWalker::FtwOk;
}
