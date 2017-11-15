/* Copyright (C) 2004 J.F.Dockes
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
#ifndef _INDEXER_H_INCLUDED_
#define _INDEXER_H_INCLUDED_
#include "rclconfig.h"

#include <string>
#include <list>
#include <map>
#include <vector>
#include <mutex>

using std::string;
using std::list;
using std::map;
using std::vector;

#include "rcldb.h"
#include "rcldoc.h"

class FsIndexer;
class BeagleQueueIndexer;

class DbIxStatus {
 public:
    enum Phase {DBIXS_NONE,
		DBIXS_FILES, DBIXS_PURGE, DBIXS_STEMDB, DBIXS_CLOSING, 
		DBIXS_MONITOR,
		DBIXS_DONE};
    Phase phase;
    string fn;   // Last file processed
    int docsdone;  // Documents actually updated
    int filesdone; // Files tested (updated or not)
    int fileerrors; // Failed files (e.g.: missing input handler).
    int dbtotdocs;  // Doc count in index at start
    // Total files in index.This is actually difficult to compute from
    // the index so it's preserved from last indexing
    int totfiles;
    
    void reset() {
	phase = DBIXS_FILES;
	fn.erase();
	docsdone = filesdone = fileerrors = dbtotdocs = totfiles = 0;
    }
    DbIxStatus() {reset();}
};

/** Callback to say what we're doing. If the update func returns false, we
 * stop as soon as possible without corrupting state */
class DbIxStatusUpdater {
 public:
#ifdef IDX_THREADS
    std::mutex m_mutex;
#endif
    DbIxStatus status;
    virtual ~DbIxStatusUpdater(){}

    // Convenience: change phase/fn and update
    virtual bool update(DbIxStatus::Phase phase, const string& fn)
    {
#ifdef IDX_THREADS
	std::unique_lock<std::mutex>  lock(m_mutex);
#endif
        status.phase = phase;
        status.fn = fn;
        return update();
    }

    // To be implemented by user for sending info somewhere
    virtual bool update() = 0;
};

/**
 * The top level batch indexing object. Processes the configuration,
 * then invokes file system walking or other to populate/update the 
 * database(s). 
 */
class ConfIndexer {
 public:
    enum runStatus {IndexerOk, IndexerError};
    ConfIndexer(RclConfig *cnf, DbIxStatusUpdater *updfunc = 0);
    virtual ~ConfIndexer();

    // Indexer types. Maybe we'll have something more dynamic one day
    enum ixType {IxTNone, IxTFs=1, IxTBeagleQueue=2, 
                 IxTAll = IxTFs | IxTBeagleQueue};
    // Misc indexing flags
    enum IxFlag {IxFNone = 0, 
                 IxFIgnoreSkip = 1, // Ignore skipped lists
                 IxFNoWeb = 2, // Do not process the web queue.
                 // First pass: just do the top files so that the user can 
                 // try searching asap.
                 IxFQuickShallow = 4, 
                 // Do not retry files which previously failed ('+' sigs)
                 IxFNoRetryFailed = 8,
                 // Do perform purge pass even if we can't be sure we saw
                 // all files
                 IxFDoPurge = 16,
    };

    /** Run indexers */
    bool index(bool resetbefore, ixType typestorun, int f = IxFNone);

    const string &getReason() {return m_reason;}

    /** Stemming reset to config: create needed, delete unconfigured */
    bool createStemmingDatabases();

    /** Create stem database for given language */
    bool createStemDb(const string &lang);

    /** Create misspelling expansion dictionary if aspell i/f is available */
    bool createAspellDict();

    /** List possible stemmer names */
    static vector<string> getStemmerNames();

    /** Index a list of files. No db cleaning or stemdb updating */
    bool indexFiles(list<string> &files, int f = IxFNone);

    /** Update index for list of documents given as list of docs (out of query)
     */
    bool updateDocs(vector<Rcl::Doc> &docs, IxFlag f = IxFNone);

    /** Purge a list of files. */
    bool purgeFiles(list<string> &files, int f = IxFNone);

    /** Set in place reset mode */
    void setInPlaceReset() {m_db.setInPlaceReset();}
 private:
    RclConfig *m_config;
    Rcl::Db    m_db;
    FsIndexer *m_fsindexer; 
    bool                m_dobeagle;
    BeagleQueueIndexer *m_beagler; 
    DbIxStatusUpdater  *m_updater;
    string              m_reason;

    // The first time we index, we do things a bit differently to
    // avoid user frustration (make at least some results available
    // fast by using several passes, the first ones to index common
    // interesting locations).
    bool runFirstIndexing();
    bool firstFsIndexingSequence();
};

#endif /* _INDEXER_H_INCLUDED_ */
