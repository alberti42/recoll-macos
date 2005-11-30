#ifndef _INDEXER_H_INCLUDED_
#define _INDEXER_H_INCLUDED_
/* @(#$Id: indexer.h,v 1.7 2005-11-30 09:46:25 dockes Exp $  (C) 2004 J.F.Dockes */

#include "rclconfig.h"

/** 
 * An internal class to process all directories indexed into the same database.
 */
class DbIndexer;

/**
 * The file system indexing object. Processes the configuration, then invokes
 * file system walking to populate/update the database(s).
 *
 * Multiple top-level directories can be listed in the
 * configuration. Each can be indexed to a different
 * database. Directories are first grouped by database, then an
 * internal class (DbIndexer) is used to process each group.
 */
class ConfIndexer {
    RclConfig *config;
    DbIndexer *dbindexer; // Object to process directories for a given db
 public:
    enum runStatus {IndexerOk, IndexerError};
    ConfIndexer(RclConfig *cnf) : config(cnf), dbindexer(0) {}
    virtual ~ConfIndexer();
    /** Worker function: doe the actual indexing */
    bool index(bool resetbefore = false);
};

#endif /* _INDEXER_H_INCLUDED_ */
