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
    FsIndexer(RclConfig *cnf, DbIxStatusUpdater *updfunc = 0) 
	: m_config(cnf), m_db(cnf), m_updater(updfunc)
    {
        m_havelocalfields = m_config->hasNameAnywhere("localfields");
    }
	
    virtual ~FsIndexer();

    /** Top level file system tree index method for updating a
	given database.

	The list is supposed to have all the filename space for the
	db, and we shall purge entries for non-existing files at the
	end. We create the temporary directory, open the database,
	then call a file system walk for each top-level directory.
	When walking is done, we create the stem databases and close
	the main db.
    */
    bool indexTrees(bool resetbefore, std::list<string> *topdirs);

    /** Index a list of files. No db cleaning or stemdb updating */
    bool indexFiles(const std::list<string> &files);

    /** Purge a list of files. */
    bool purgeFiles(const std::list<string> &files);

    /** Stemming reset to config: create needed, delete unconfigured */
    bool createStemmingDatabases();

    /** Create stem database for given language */
    bool createStemDb(const string &lang);

    /** Create misspelling expansion dictionary if aspell i/f is available */
    bool createAspellDict();

    /**  Tree walker callback method */
    FsTreeWalker::Status 
    processone(const string &, const struct stat *, FsTreeWalker::CbFlag);

    /** Return my db dir */
    string getDbDir() {return m_config->getDbDir();}

    /** List possible stemmer names */
    static list<string> getStemmerNames();

 private:
    FsTreeWalker m_walker;
    RclConfig   *m_config;
    Rcl::Db      m_db;
    string       m_tmpdir;
    DbIxStatusUpdater *m_updater;

    // The configuration can set attribute fields to be inherited by
    // all files in a file system area. Ie: set "apptag = thunderbird"
    // inside ~/.thunderbird. The boolean is set at init to avoid
    // further wasteful processing if no local fields are set.
    bool         m_havelocalfields;
    map<string, string> m_localfields;

    bool init(bool rst = false, bool rdonly = false);
    void localfieldsfromconf();
    void setlocalfields(Rcl::Doc& doc);
};

#endif /* _fsindexer_h_included_ */
