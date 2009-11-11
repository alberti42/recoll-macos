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
#ifndef _fsindexer_h_included_
#define _fsindexer_h_included_
/* @(#$Id: $  (C) 2009 J.F.Dockes */

#include "fstreewalk.h"
#include "rcldb.h"

class DbIxStatusUpdater;

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
    FsIndexer(RclConfig *cnf, Rcl::Db *db, DbIxStatusUpdater *updfunc = 0) 
	: m_config(cnf), m_db(db), m_updater(updfunc)
    {
        m_havelocalfields = m_config->hasNameAnywhere("localfields");
    }
	
    virtual ~FsIndexer();

    /** 
     * Top level file system tree index method for updating a given database.
     *
     * We create the temporary directory, open the database,
     * then call a file system walk for each top-level directory.
     */
    bool index(bool resetbefore);

    /** Index a list of files. No db cleaning or stemdb updating */
    bool indexFiles(const std::list<string> &files);

    /** Purge a list of files. */
    bool purgeFiles(const std::list<string> &files);

    /**  Tree walker callback method */
    FsTreeWalker::Status 
    processone(const string &fn, const struct stat *, FsTreeWalker::CbFlag);

 private:
    FsTreeWalker m_walker;
    RclConfig   *m_config;
    Rcl::Db     *m_db;
    string       m_tmpdir;
    string       m_reason;
    DbIxStatusUpdater *m_updater;

    // The configuration can set attribute fields to be inherited by
    // all files in a file system area. Ie: set "apptag = thunderbird"
    // inside ~/.thunderbird. The boolean is set at init to avoid
    // further wasteful processing if no local fields are set.
    bool         m_havelocalfields;
    map<string, string> m_localfields;

    bool init();
    void localfieldsfromconf();
    void setlocalfields(Rcl::Doc& doc);
    string getDbDir() {return m_config->getDbDir();}
};

#endif /* _fsindexer_h_included_ */
