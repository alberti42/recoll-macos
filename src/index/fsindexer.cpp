/* Copyright (C) 2009 J.F.Dockes
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
#include "autoconfig.h"

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

#include "cstr.h"
#include "pathut.h"
#include "conftree.h"
#include "rclconfig.h"
#include "fstreewalk.h"
#include "rcldb.h"
#include "readfile.h"
#include "indexer.h"
#include "fsindexer.h"
#include "transcode.h"
#include "debuglog.h"
#include "internfile.h"
#include "smallut.h"
#include "wipedir.h"
#include "fileudi.h"
#include "cancelcheck.h"
#include "rclinit.h"
#include "execmd.h"

// When using extended attributes, we have to use the ctime. 
// This is quite an expensive price to pay...
#ifdef RCL_USE_XATTR
#define RCL_STTIME st_ctime
#else
#define RCL_STTIME st_mtime
#endif // RCL_USE_XATTR

using namespace std;

#ifdef IDX_THREADS
class DbUpdTask {
public:
    DbUpdTask(const string& u, const string& p, const Rcl::Doc& d)
	: udi(u), parent_udi(p), doc(d)
    {}
    string udi;
    string parent_udi;
    Rcl::Doc doc;
};
extern void *FsIndexerDbUpdWorker(void*);

class InternfileTask {
public:
    InternfileTask(const std::string &f, const struct stat *i_stp,
		   map<string,string> lfields, vector<MDReaper> reapers)
	: fn(f), statbuf(*i_stp), localfields(lfields), mdreapers(reapers)
    {}
    string fn;
    struct stat statbuf;
    map<string,string> localfields;
    vector<MDReapers> mdreapers;
};
extern void *FsIndexerInternfileWorker(void*);
#endif // IDX_THREADS

// Thread safe variation of the "missing helpers" storage. Only the
// addMissing method needs protection, the rest are called from the
// main thread either before or after the exciting part
class FSIFIMissingStore : public FIMissingStore {
#ifdef IDX_THREADS
    PTMutexInit m_mutex;
#endif
public:
    virtual void addMissing(const string& prog, const string& mt)
    {
#ifdef IDX_THREADS
	PTMutexLocker locker(m_mutex);
#endif
	FIMissingStore::addMissing(prog, mt);
    }
};

FsIndexer::FsIndexer(RclConfig *cnf, Rcl::Db *db, DbIxStatusUpdater *updfunc) 
    : m_config(cnf), m_db(db), m_updater(updfunc), 
      m_missing(new FSIFIMissingStore)
#ifdef IDX_THREADS
    , m_iwqueue("Internfile", cnf->getThrConf(RclConfig::ThrIntern).first), 
      m_dwqueue("Split", cnf->getThrConf(RclConfig::ThrSplit).first)
#endif // IDX_THREADS
{
    LOGDEB1(("FsIndexer::FsIndexer\n"));
    m_havelocalfields = m_config->hasNameAnywhere("localfields");
    m_havemdreapers = m_config->hasNameAnywhere("metadatacmds");

#ifdef IDX_THREADS
    m_stableconfig = new RclConfig(*m_config);
    m_loglevel = DebugLog::getdbl()->getlevel();
    m_haveInternQ = m_haveSplitQ = false;
    int internqlen = cnf->getThrConf(RclConfig::ThrIntern).first;
    int internthreads = cnf->getThrConf(RclConfig::ThrIntern).second;
    if (internqlen >= 0) {
	if (!m_iwqueue.start(internthreads, FsIndexerInternfileWorker, this)) {
	    LOGERR(("FsIndexer::FsIndexer: intern worker start failed\n"));
	    return;
	}
	m_haveInternQ = true;
    } 
    int splitqlen = cnf->getThrConf(RclConfig::ThrSplit).first;
    int splitthreads = cnf->getThrConf(RclConfig::ThrSplit).second;
    if (splitqlen >= 0) {
	if (!m_dwqueue.start(splitthreads, FsIndexerDbUpdWorker, this)) {
	    LOGERR(("FsIndexer::FsIndexer: split worker start failed\n"));
	    return;
	}
	m_haveSplitQ = true;
    }
    LOGDEB(("FsIndexer: threads: haveIQ %d iql %d iqts %d "
	    "haveSQ %d sql %d sqts %d\n", m_haveInternQ, internqlen, 
	    internthreads, m_haveSplitQ, splitqlen, splitthreads));
#endif // IDX_THREADS
}

FsIndexer::~FsIndexer() 
{
    LOGDEB1(("FsIndexer::~FsIndexer()\n"));

#ifdef IDX_THREADS
    void *status;
    if (m_haveInternQ) {
	status = m_iwqueue.setTerminateAndWait();
	LOGDEB0(("FsIndexer: internfile wrkr status: %ld (1->ok)\n", 
		 long(status)));
    }
    if (m_haveSplitQ) {
	status = m_dwqueue.setTerminateAndWait();
	LOGDEB0(("FsIndexer: dbupd worker status: %ld (1->ok)\n", 
		 long(status)));
    }
    delete m_stableconfig;
#endif // IDX_THREADS

    delete m_missing;
}

bool FsIndexer::init()
{
    if (m_tdl.empty()) {
        m_tdl = m_config->getTopdirs();
        if (m_tdl.empty()) {
            LOGERR(("FsIndexers: no topdirs list defined\n"));
            return false;
        }
    }
    return true;
}

// Recursively index each directory in the topdirs:
bool FsIndexer::index()
{
    Chrono chron;
    if (!init())
	return false;

    if (m_updater) {
#ifdef IDX_THREADS
	PTMutexLocker locker(m_updater->m_mutex);
#endif
	m_updater->status.reset();
	m_updater->status.dbtotdocs = m_db->docCnt();
    }

    m_walker.setSkippedPaths(m_config->getSkippedPaths());
    for (vector<string>::const_iterator it = m_tdl.begin();
	 it != m_tdl.end(); it++) {
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

	// Walk the directory tree
	if (m_walker.walk(*it, *this) != FsTreeWalker::FtwOk) {
	    LOGERR(("FsIndexer::index: error while indexing %s: %s\n", 
		    it->c_str(), m_walker.getReason().c_str()));
	    return false;
	}
    }

#ifdef IDX_THREADS
    if (m_haveInternQ) 
	m_iwqueue.waitIdle();
    if (m_haveSplitQ)
	m_dwqueue.waitIdle();
    m_db->waitUpdIdle();
#endif // IDX_THREADS

    if (m_missing) {
	string missing;
	m_missing->getMissingDescription(missing);
	if (!missing.empty()) {
	    LOGINFO(("FsIndexer::index missing helper program(s):\n%s\n", 
		     missing.c_str()));
	}
	m_config->storeMissingHelperDesc(missing);
    }
    LOGERR(("fsindexer index time:  %d mS\n", chron.ms()));
    return true;
}

static bool matchesSkipped(const vector<string>& tdl,
                           FsTreeWalker& walker,
                           const string& path)
{
    // First check what (if any) topdir this is in:
    string td;
    for (vector<string>::const_iterator it = tdl.begin(); 
	 it != tdl.end(); it++) {
        if (path.find(*it) == 0) {
            td = *it;
            break;
        }
    }
    if (td.empty()) {
        LOGDEB(("FsIndexer::indexFiles: skipping [%s] (ntd)\n", path.c_str()));
        return true;
    }

    // Check path against skippedPaths. 
    if (walker.inSkippedPaths(path)) {
        LOGDEB(("FsIndexer::indexFiles: skipping [%s] (skpp)\n", path.c_str()));
        return true;
    }

    // Then check all path components up to the topdir against skippedNames
    string mpath = path;
    while (mpath.length() >= td.length() && mpath.length() > 1) {
        string fn = path_getsimple(mpath);
        if (walker.inSkippedNames(fn)) {
            LOGDEB(("FsIndexer::indexFiles: skipping [%s] (skpn)\n", 
                    path.c_str()));
            return true;
        }

        string::size_type len = mpath.length();
        mpath = path_getfather(mpath);
        // getfather normally returns a path ending with /, getsimple 
        // would then return ''
        if (!mpath.empty() && mpath[mpath.size()-1] == '/')
            mpath.erase(mpath.size()-1);
        // should not be necessary, but lets be prudent. If the
        // path did not shorten, something is seriously amiss
        // (could be an assert actually)
        if (mpath.length() >= len)
            return true;
    }
    return false;
}

/** 
 * Index individual files, out of a full tree run. No database purging
 */
bool FsIndexer::indexFiles(list<string>& files, ConfIndexer::IxFlag flag)
{
    LOGDEB(("FsIndexer::indexFiles\n"));
    int ret = false;

    if (!init())
        return false;

    int abslen;
    if (m_config->getConfParam("idxabsmlen", &abslen))
	m_db->setAbstractParams(abslen, -1, -1);
      
    // We use an FsTreeWalker just for handling the skipped path/name lists
    FsTreeWalker walker;
    walker.setSkippedPaths(m_config->getSkippedPaths());

    for (list<string>::iterator it = files.begin(); it != files.end(); ) {
        LOGDEB2(("FsIndexer::indexFiles: [%s]\n", it->c_str()));

        m_config->setKeyDir(path_getfather(*it));
	if (m_havelocalfields)
	    localfieldsfromconf();
	if (m_havemdreapers)
	    mdreapersfromconf();

	bool follow = false;
	m_config->getConfParam("followLinks", &follow);

        walker.setSkippedNames(m_config->getSkippedNames());
	// Check path against indexed areas and skipped names/paths
        if (!(flag&ConfIndexer::IxFIgnoreSkip) && 
	    matchesSkipped(m_tdl, walker, *it)) {
            it++; 
	    continue;
        }

	struct stat stb;
	int ststat = follow ? stat(it->c_str(), &stb) : 
	    lstat(it->c_str(), &stb);
	if (ststat != 0) {
	    LOGERR(("FsIndexer::indexFiles: lstat(%s): %s", it->c_str(),
		    strerror(errno)));
            it++; 
	    continue;
	}

	if (processone(*it, &stb, FsTreeWalker::FtwRegular) != 
	    FsTreeWalker::FtwOk) {
	    LOGERR(("FsIndexer::indexFiles: processone failed\n"));
	    goto out;
	}
        it = files.erase(it);
    }

    ret = true;
out:
#ifdef IDX_THREADS
    if (m_haveInternQ) 
	m_iwqueue.waitIdle();
    if (m_haveSplitQ)
	m_dwqueue.waitIdle();
    m_db->waitUpdIdle();
#endif // IDX_THREADS
    LOGDEB(("FsIndexer::indexFiles: done\n"));
    return ret;
}


/** Purge docs for given files out of the database */
bool FsIndexer::purgeFiles(list<string>& files)
{
    LOGDEB(("FsIndexer::purgeFiles\n"));
    bool ret = false;
    if (!init())
	return false;

    for (list<string>::iterator it = files.begin(); it != files.end(); ) {
	string udi;
	make_udi(*it, cstr_null, udi);
        // rcldb::purgefile returns true if the udi was either not
        // found or deleted, false only in case of actual error
        bool existed;
	if (!m_db->purgeFile(udi, &existed)) {
	    LOGERR(("FsIndexer::purgeFiles: Database error\n"));
	    goto out;
	}
        // If we actually deleted something, take it off the list
        if (existed) {
            it = files.erase(it);
        } else {
            it++;
        }
    }

    ret = true;
out:
#ifdef IDX_THREADS
    if (m_haveInternQ) 
	m_iwqueue.waitIdle();
    if (m_haveSplitQ)
	m_dwqueue.waitIdle();
    m_db->waitUpdIdle();
#endif // IDX_THREADS
    LOGDEB(("FsIndexer::purgeFiles: done\n"));
    return ret;
}

// Local fields can be set for fs subtrees in the configuration file 
void FsIndexer::localfieldsfromconf()
{
    LOGDEB1(("FsIndexer::localfieldsfromconf\n"));

    string sfields;
    m_config->getConfParam("localfields", sfields);
    if (!sfields.compare(m_slocalfields)) 
	return;

    m_slocalfields = sfields;
    m_localfields.clear();
    if (sfields.empty())
	return;

    string value;
    ConfSimple attrs;
    m_config->valueSplitAttributes(sfields, value, attrs);
    vector<string> nmlst = attrs.getNames(cstr_null);
    for (vector<string>::const_iterator it = nmlst.begin();
         it != nmlst.end(); it++) {
	attrs.get(*it, m_localfields[*it]);
    }
}


// 
void FsIndexer::setlocalfields(const map<string, string>& fields, Rcl::Doc& doc)
{
    for (map<string, string>::const_iterator it = fields.begin();
	 it != fields.end(); it++) {
        // Should local fields override those coming from the document
        // ? I think not, but not too sure. We could also chose to
        // concatenate the values ?
        if (doc.meta.find(it->second) == doc.meta.end()) {
            doc.meta[it->first] = it->second;
	}
    }
}

// Metadata gathering commands
void FsIndexer::mdreapersfromconf()
{
    LOGDEB1(("FsIndexer::mdreapersfromconf\n"));

    string sreapers;
    m_config->getConfParam("metadatacmds", sreapers);
    if (!sreapers.compare(m_smdreapers)) 
	return;

    m_smdreapers = sreapers;
    m_mdreapers.clear();
    if (sreapers.empty())
	return;

    string value;
    ConfSimple attrs;
    m_config->valueSplitAttributes(sreapers, value, attrs);
    vector<string> nmlst = attrs.getNames(cstr_null);
    for (vector<string>::const_iterator it = nmlst.begin();
         it != nmlst.end(); it++) {
	MDReaper reaper;
	reaper.fieldname = m_config->fieldCanon(*it);
	string s;
	attrs.get(*it, s);
	stringToStrings(s, reaper.cmdv);
	m_mdreapers.push_back(reaper);
    }
}

void FsIndexer::reapmetadata(const vector<MDReaper>& reapers, const string& fn,
			     Rcl::Doc& doc)
{
    map<char,string> smap = create_map<char, string>('f', fn);
    for (vector<MDReaper>::const_iterator rp = reapers.begin();
	 rp != reapers.end(); rp++) {
	vector<string> cmd;
	for (vector<string>::const_iterator it = rp->cmdv.begin();
	     it != rp->cmdv.end(); it++) {
	    string s;
	    pcSubst(*it, s, smap);
	    cmd.push_back(s);
	}
	string output;
	if (ExecCmd::backtick(cmd, output)) {
	    doc.meta[rp->fieldname] += string(" ") + output;
	}
    }
}

void FsIndexer::makesig(const struct stat *stp, string& out)
{
    char cbuf[100]; 
    sprintf(cbuf, OFFTPC "%ld", stp->st_size, (long)stp->RCL_STTIME);
    out = cbuf;
}

#ifdef IDX_THREADS
// Called updworker as seen from here, but the first step (and only in
// most meaningful configurations) is doing the word-splitting, which
// is why the task is referred as "Split" in the grand scheme of
// things. An other stage usually deals with the actual index update.
void *FsIndexerDbUpdWorker(void * fsp)
{
    recoll_threadinit();
    FsIndexer *fip = (FsIndexer*)fsp;
    WorkQueue<DbUpdTask*> *tqp = &fip->m_dwqueue;
    DebugLog::getdbl()->setloglevel(fip->m_loglevel);

    DbUpdTask *tsk;
    for (;;) {
	size_t qsz;
	if (!tqp->take(&tsk, &qsz)) {
	    tqp->workerExit();
	    return (void*)1;
	}
	LOGDEB0(("FsIndexerDbUpdWorker: task ql %d\n", int(qsz)));
	if (!fip->m_db->addOrUpdate(tsk->udi, tsk->parent_udi, tsk->doc)) {
	    LOGERR(("FsIndexerDbUpdWorker: addOrUpdate failed\n"));
	    tqp->workerExit();
	    return (void*)0;
	}
	delete tsk;
    }
}

void *FsIndexerInternfileWorker(void * fsp)
{
    recoll_threadinit();
    FsIndexer *fip = (FsIndexer*)fsp;
    WorkQueue<InternfileTask*> *tqp = &fip->m_iwqueue;
    DebugLog::getdbl()->setloglevel(fip->m_loglevel);
    TempDir tmpdir;
    RclConfig myconf(*(fip->m_stableconfig));

    InternfileTask *tsk;
    for (;;) {
	if (!tqp->take(&tsk)) {
	    tqp->workerExit();
	    return (void*)1;
	}
	LOGDEB0(("FsIndexerInternfileWorker: task fn %s\n", tsk->fn.c_str()));
	if (fip->processonefile(&myconf, tmpdir, tsk->fn, &tsk->statbuf,
		tsk->localfields) !=
	    FsTreeWalker::FtwOk) {
	    LOGERR(("FsIndexerInternfileWorker: processone failed\n"));
	    tqp->workerExit();
	    return (void*)0;
	}
	LOGDEB1(("FsIndexerInternfileWorker: done fn %s\n", tsk->fn.c_str()));
	delete tsk;
    }
}
#endif // IDX_THREADS

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
    if (m_updater) {
#ifdef IDX_THREADS
	PTMutexLocker locker(m_updater->m_mutex);
#endif
	if (!m_updater->update()) {
	    return FsTreeWalker::FtwStop;
	}
    }

    // If we're changing directories, possibly adjust parameters (set
    // the current directory in configuration object)
    if (flg == FsTreeWalker::FtwDirEnter || 
	flg == FsTreeWalker::FtwDirReturn) {
	m_config->setKeyDir(fn);

	// Set up skipped patterns for this subtree. 
	m_walker.setSkippedNames(m_config->getSkippedNames());

        // Adjust local fields from config for this subtree
	if (m_havelocalfields)
	    localfieldsfromconf();
	if (m_havemdreapers)
	    mdreapersfromconf();

	if (flg == FsTreeWalker::FtwDirReturn)
	    return FsTreeWalker::FtwOk;
    }

#ifdef IDX_THREADS
    if (m_haveInternQ) {
	InternfileTask *tp = new InternfileTask(fn, stp, m_localfields, 
	    m_mdreapers);
	if (m_iwqueue.put(tp)) {
	    return FsTreeWalker::FtwOk;
	} else {
	    return FsTreeWalker::FtwError;
	}
    }
#endif

    return processonefile(m_config, m_tmpdir, fn, stp, m_localfields, 
			  m_mdreapers);
}


FsTreeWalker::Status 
FsIndexer::processonefile(RclConfig *config, TempDir& tmpdir,
			  const std::string &fn, const struct stat *stp,
			  const map<string, string>& localfields,
			  const vector<MDReaper>& mdreapers)
{
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
    string sig;
    makesig(stp, sig);
    string udi;
    make_udi(fn, cstr_null, udi);
    bool needupdate = m_db->needUpdate(udi, sig);

    if (!needupdate) {
	LOGDEB0(("processone: up to date: %s\n", fn.c_str()));
	if (m_updater) {
#ifdef IDX_THREADS
	    PTMutexLocker locker(m_updater->m_mutex);
#endif
	    // Status bar update, abort request etc.
	    m_updater->status.fn = fn;
	    ++(m_updater->status.filesdone);
	    if (!m_updater->update()) {
		return FsTreeWalker::FtwStop;
	    }
	}
	return FsTreeWalker::FtwOk;
    }

    LOGDEB0(("processone: processing: [%s] %s\n", 
             displayableBytes(stp->st_size).c_str(), fn.c_str()));

    FileInterner interner(fn, stp, config, tmpdir, FileInterner::FIF_none);
    if (!interner.ok()) {
        // no indexing whatsoever in this case. This typically means that
        // indexallfilenames is not set
        return FsTreeWalker::FtwOk;
    }
    interner.setMissingStore(m_missing);

    // File name transcoded to utf8 for indexing. 
    // If this fails, the file name won't be indexed, no big deal
    // Note that we used to do the full path here, but I ended up believing
    // that it made more sense to use only the file name
    // The charset is used is the one from the locale.
    string charset = config->getDefCharset(true);
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
    make_udi(fn, cstr_null, parent_udi);
    Rcl::Doc doc;
    char ascdate[30];
    sprintf(ascdate, "%ld", long(stp->st_mtime));

    FileInterner::Status fis = FileInterner::FIAgain;
    bool hadNullIpath = false;
    while (fis == FileInterner::FIAgain) {
	doc.erase();
        try {
            fis = interner.internfile(doc);
        } catch (CancelExcept) {
            LOGERR(("fsIndexer::processone: interrupted\n"));
            return FsTreeWalker::FtwStop;
        }

        // Index at least the file name even if there was an error.
        // We'll change the signature to ensure that the indexing will
        // be retried every time.

	// Internal access path for multi-document files. If empty, this is
	// for the main file.
	if (doc.ipath.empty()) {
	    hadNullIpath = true;
	    if (m_havemdreapers)
		reapmetadata(mdreapers, fn, doc);
	} 

	// Set file name, mod time and url if not done by filter
	if (doc.fmtime.empty())
	    doc.fmtime = ascdate;
        if (doc.url.empty())
            doc.url = cstr_fileu + fn;
	const string *fnp = 0;
	if (!doc.peekmeta(Rcl::Doc::keyfn, &fnp) || fnp->empty())
	    doc.meta[Rcl::Doc::keyfn] = utf8fn;

	char cbuf[100]; 
	sprintf(cbuf, OFFTPC, stp->st_size);
	doc.pcbytes = cbuf;
	// Document signature for up to date checks: concatenate
	// m/ctime and size. Looking for changes only, no need to
	// parseback so no need for reversible formatting. Also set,
	// but never used, for subdocs.
	makesig(stp, doc.sig);

	// If there was an error, ensure indexing will be
	// retried. This is for the once missing, later installed
	// filter case. It can make indexing much slower (if there are
	// myriads of such files, the ext script is executed for them
	// and fails every time)
	if (fis == FileInterner::FIError) {
	    doc.sig += cstr_plus;
	}

        // Possibly add fields from local config
        if (m_havelocalfields) 
            setlocalfields(localfields, doc);
	// Add document to database. If there is an ipath, add it as a children
	// of the file document.
	string udi;
	make_udi(fn, doc.ipath, udi);

#ifdef IDX_THREADS
	if (m_haveSplitQ) {
	    DbUpdTask *tp = new DbUpdTask(udi, doc.ipath.empty() ? cstr_null : parent_udi, doc);
	    if (!m_dwqueue.put(tp)) {
		LOGERR(("processonefile: wqueue.put failed\n"));
		return FsTreeWalker::FtwError;
	    } 
	} else {
#endif
	    if (!m_db->addOrUpdate(udi, doc.ipath.empty() ? 
				   cstr_null : parent_udi, doc)) {
		return FsTreeWalker::FtwError;
	    }
#ifdef IDX_THREADS
	}
#endif

	// Tell what we are doing and check for interrupt request
	if (m_updater) {
#ifdef IDX_THREADS
	    PTMutexLocker locker(m_updater->m_mutex);
#endif
	    ++(m_updater->status.docsdone);
            if (m_updater->status.dbtotdocs < m_updater->status.docsdone)
                m_updater->status.dbtotdocs = m_updater->status.docsdone;
            m_updater->status.fn = fn;
            if (!doc.ipath.empty())
                m_updater->status.fn += "|" + doc.ipath;
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
	fileDoc.meta[Rcl::Doc::keyfn] = utf8fn;
	fileDoc.mimetype = interner.getMimetype();
	fileDoc.url = cstr_fileu + fn;
        if (m_havelocalfields) 
            setlocalfields(localfields, fileDoc);
	if (m_havemdreapers)
	    reapmetadata(mdreapers, fn, fileDoc);
	char cbuf[100]; 
	sprintf(cbuf, OFFTPC, stp->st_size);
	fileDoc.pcbytes = cbuf;
	// Document signature for up to date checks.
	makesig(stp, fileDoc.sig);

#ifdef IDX_THREADS
	if (m_haveSplitQ) {
	    DbUpdTask *tp = new DbUpdTask(parent_udi, cstr_null, fileDoc);
	    if (!m_dwqueue.put(tp))
		return FsTreeWalker::FtwError;
	    else
		return FsTreeWalker::FtwOk;
	}
#endif
	if (!m_db->addOrUpdate(parent_udi, cstr_null, fileDoc)) 
	    return FsTreeWalker::FtwError;
    }

    return FsTreeWalker::FtwOk;
}
