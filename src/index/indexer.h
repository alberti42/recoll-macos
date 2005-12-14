#ifndef _INDEXER_H_INCLUDED_
#define _INDEXER_H_INCLUDED_
/* @(#$Id: indexer.h,v 1.8 2005-12-14 11:00:48 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

#include "rclconfig.h"
#include "fstreewalk.h"
#include "rcldb.h"

/* Forward decl for lower level indexing object */
class DbIndexer;

/**
   The top level indexing object. Processes the configuration, then invokes
   file system walking to populate/update the database(s).
 
   Multiple top-level directories can be listed in the
   configuration. Each can be indexed to a different
   database. Directories are first grouped by database, then an
   internal class (DbIndexer) is used to process each group.
*/
class ConfIndexer {
 public:
    enum runStatus {IndexerOk, IndexerError};
    ConfIndexer(RclConfig *cnf) : config(cnf), dbindexer(0) {}
	virtual ~ConfIndexer();
	/** Worker function: doe the actual indexing */
	bool index(bool resetbefore = false);
 private:
	RclConfig *config;
	DbIndexer *dbindexer; // Object to process directories for a given db
};

/** Index things into one database
 
Tree indexing: we inherits FsTreeWalkerCB so that, the processone()
method is called by the file-system tree walk code for each file and
directory. We keep all state needed while indexing, and finally call
the methods to purge the db of stale entries and create the stemming
databases.

Single file(s) indexing: no database purging or stem db updating.
*/
class DbIndexer : public FsTreeWalkerCB {
 public:
    /** Constructor does nothing but store parameters */
    DbIndexer(RclConfig *cnf, const std::string &dbd) 
	: config(cnf), dbdir(dbd) { 
    }
	
    virtual ~DbIndexer();

    /** Top level file system tree index method for updating a
	given database.

	The list is supposed to have all the filename space for the
	db, and we shall purge entries for non-existing files at the
	end. We create the temporary directory, open the database,
	then call a file system walk for each top-level directory.
	When walking is done, we create the stem databases and close
	the main db.
    */
    bool indexDb(bool resetbefore, std::list<std::string> *topdirs);

    /** Index a list of files. No db cleaning or stemdb updating */
    bool indexFiles(const std::list<std::string> &files);

    /**  Tree walker callback method */
    FsTreeWalker::Status 
	processone(const std::string &, const struct stat *, 
		   FsTreeWalker::CbFlag);

 private:
    FsTreeWalker walker;
    RclConfig *config;
    std::string dbdir;
    Rcl::Db db;
    std::string tmpdir;
    bool init(bool rst = false);
};

#endif /* _INDEXER_H_INCLUDED_ */
