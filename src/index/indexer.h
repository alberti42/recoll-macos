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
#include "fsindexer.h"

/* Forward decl for lower level indexing object */
class DbIndexer;

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
   The top level indexing object. Processes the configuration, then invokes
   file system walking to populate/update the database(s).

   Fiction:
      Multiple top-level directories can be listed in the
      configuration. Each can be indexed to a different
      database. Directories are first grouped by database, then an
      internal class (DbIndexer) is used to process each group.
   Fact: we've had one db per config forever. The multidb/config code has been 
   kept around for no good reason, this fiction only affects indexer.cpp
*/
class ConfIndexer {
 public:
    enum runStatus {IndexerOk, IndexerError};
    ConfIndexer(RclConfig *cnf, DbIxStatusUpdater *updfunc = 0)
	: m_config(cnf), m_fsindexer(0), m_updater(updfunc)
	{}
    virtual ~ConfIndexer();
    /** Worker function: doe the actual indexing */
    bool index(bool resetbefore = false);
    const string &getReason() {return m_reason;}
 private:
    RclConfig *m_config;
    FsIndexer *m_fsindexer; // Object to process directories for a given db
    DbIxStatusUpdater *m_updater;
    string m_reason;
};

#endif /* _INDEXER_H_INCLUDED_ */
