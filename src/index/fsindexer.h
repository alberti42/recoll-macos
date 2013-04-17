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
#ifndef _fsindexer_h_included_
#define _fsindexer_h_included_

#include <list>

#include "indexer.h"
#include "fstreewalk.h"
#ifdef IDX_THREADS
#include "ptmutex.h"
#include "workqueue.h"
#endif // IDX_THREADS

class DbIxStatusUpdater;
class FIMissingStore;
struct stat;

class DbUpdTask;
class InternfileTask;

/** Index selected parts of the file system
 
Tree indexing: we inherits FsTreeWalkerCB so that, the processone()
method is called by the file-system tree walk code for each file and
directory. We keep all state needed while indexing, and finally call
the methods to purge the db of stale entries and create the stemming
databases.

Single file(s) indexing: there are also calls to index or purge lists of files.
No database purging or stem db updating in this case.
*/
class FsIndexer : public FsTreeWalkerCB {
 public:
    /** Constructor does nothing but store parameters 
     *
     * @param cnf Configuration data
     * @param updfunc Status updater callback
     */
    FsIndexer(RclConfig *cnf, Rcl::Db *db, DbIxStatusUpdater *updfunc = 0);
    virtual ~FsIndexer();

    /** 
     * Top level file system tree index method for updating a given database.
     *
     * We open the database,
     * then call a file system walk for each top-level directory.
     */
    bool index();

    /** Index a list of files. No db cleaning or stemdb updating */
    bool indexFiles(std::list<std::string> &files, ConfIndexer::IxFlag f = 
		    ConfIndexer::IxFNone);

    /** Purge a list of files. */
    bool purgeFiles(std::list<std::string> &files);

    /**  Tree walker callback method */
    FsTreeWalker::Status 
    processone(const string &fn, const struct stat *, FsTreeWalker::CbFlag);

    /** Make signature for file up to date checks */
    static void makesig(const struct stat *stp, string& out);

    /* Hold the description for an external metadata-gathering command */
    struct MDReaper {
	string fieldname;
	vector<string> cmdv;
    };

 private:
    FsTreeWalker m_walker;
    RclConfig   *m_config;
    Rcl::Db     *m_db;
    string       m_reason;
    DbIxStatusUpdater *m_updater;
    std::vector<std::string> m_tdl;
    FIMissingStore *m_missing;

    // The configuration can set attribute fields to be inherited by
    // all files in a file system area. Ie: set "rclaptg = thunderbird"
    // inside ~/.thunderbird. The boolean is set at init to avoid
    // further wasteful processing if no local fields are set.
    bool         m_havelocalfields;
    string       m_slocalfields;
    map<string, string>  m_localfields;

    // Same idea with the metadata-gathering external commands,
    // (e.g. used to reap tagging info: "tmsu tags %f")
    bool m_havemdreapers;
    string m_smdreapers;
    vector<MDReaper> m_mdreapers;

#ifdef IDX_THREADS
    friend void *FsIndexerDbUpdWorker(void*);
    friend void *FsIndexerInternfileWorker(void*);
    int m_loglevel;
    WorkQueue<InternfileTask*> m_iwqueue;
    WorkQueue<DbUpdTask*> m_dwqueue;
    bool m_haveInternQ;
    bool m_haveSplitQ;
    RclConfig   *m_stableconfig;
#endif // IDX_THREADS

    bool init();
    void localfieldsfromconf();
    void mdreapersfromconf();
    void setlocalfields(const map<string, string>& flds, Rcl::Doc& doc);
    void reapmetadata(const vector<MDReaper>& reapers, const string &fn,
		      Rcl::Doc& doc);
    string getDbDir() {return m_config->getDbDir();}
    FsTreeWalker::Status 
    processonefile(RclConfig *config, const string &fn, 
		   const struct stat *, const map<string,string>& localfields,
		   const vector<MDReaper>& mdreapers);
};

#endif /* _fsindexer_h_included_ */
