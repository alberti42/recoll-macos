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
#ifndef _beaglequeue_h_included_
#define _beaglequeue_h_included_
/* @(#$Id: $  (C) 2009 J.F.Dockes */

/**
 * Code to process the Beagle indexing queue. Beagle MUST NOT be
 * running, else mayhem will ensue. Interesting to reuse the beagle
 * firefox visited page indexing plugin for example.
 */

#include "rclconfig.h"
#include "fstreewalk.h"
#include "rcldb.h"

class DbIxStatusUpdater;
class CirCache;

class BeagleQueueIndexer : public FsTreeWalkerCB {
public:
    BeagleQueueIndexer(RclConfig *cnf, Rcl::Db *db, 
                       DbIxStatusUpdater *updfunc = 0);
    ~BeagleQueueIndexer();
    
    bool index();

    FsTreeWalker::Status 
    processone(const string &, const struct stat *, FsTreeWalker::CbFlag);

private:
    RclConfig *m_config;
    Rcl::Db   *m_db;
    CirCache  *m_cache;
    string     m_queuedir;
    string     m_tmpdir;
    DbIxStatusUpdater *m_updater;

    bool indexFromCache(const string& udi);

};

#endif /* _beaglequeue_h_included_ */
