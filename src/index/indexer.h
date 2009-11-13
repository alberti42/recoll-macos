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
#ifndef _INDEXER_H_INCLUDED_
#define _INDEXER_H_INCLUDED_
/* @(#$Id: indexer.h,v 1.27 2008-12-17 08:01:40 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>
#include <map>

#ifndef NO_NAMESPACES
using std::string;
using std::list;
using std::map;
#endif

#include "rclconfig.h"
#include "rcldb.h"

class FsIndexer;
class BeagleQueueIndexer;

class DbIxStatus {
 public:
    enum Phase {DBIXS_FILES, DBIXS_PURGE, DBIXS_STEMDB, DBIXS_CLOSING};
    Phase phase;
    string fn;   // Last file processed
    int docsdone;  // Documents processed
    int dbtotdocs;  // Doc count in index at start
    void reset() {phase = DBIXS_FILES;fn.erase();docsdone=dbtotdocs=0;}
    DbIxStatus() {reset();}
};

/** Callback to say what we're doing. If the update func returns false, we
 * stop as soon as possible without corrupting state */
class DbIxStatusUpdater {
 public:
    DbIxStatus status;
    virtual ~DbIxStatusUpdater(){}
    virtual bool update() = 0;
};

/**
 * The top level indexing object. Processes the configuration, then invokes
 * file system walking or other to populate/update the database(s).
*/
class ConfIndexer {
 public:
    enum runStatus {IndexerOk, IndexerError};
    ConfIndexer(RclConfig *cnf, DbIxStatusUpdater *updfunc = 0);
    virtual ~ConfIndexer();

    // Indexer types. Maybe we'll have something more dynamic one day
    enum ixType {IxTNone, IxTFs=1, IxTBeagleQueue=2, 
                 IxTAll = IxTFs | IxTBeagleQueue};
    /** Run indexers */
    bool index(bool resetbefore, ixType typestorun);

    const string &getReason() {return m_reason;}

    /** Stemming reset to config: create needed, delete unconfigured */
    bool createStemmingDatabases();

    /** Create stem database for given language */
    bool createStemDb(const string &lang);

    /** Create misspelling expansion dictionary if aspell i/f is available */
    bool createAspellDict();

    /** List possible stemmer names */
    static list<string> getStemmerNames();

    /** Index a list of files. No db cleaning or stemdb updating */
    bool indexFiles(const std::list<string> &files);

    /** Purge a list of files. */
    bool purgeFiles(const std::list<string> &files);

 private:
    RclConfig *m_config;
    Rcl::Db    m_db;
    FsIndexer *m_fsindexer; 
    bool                m_dobeagle;
    BeagleQueueIndexer *m_beagler; 
    DbIxStatusUpdater  *m_updater;
    string m_reason;
    list<string> m_tdl;

    bool initTopDirs();
};

#endif /* _INDEXER_H_INCLUDED_ */
