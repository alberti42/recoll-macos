#ifndef _INDEXER_H_INCLUDED_
#define _INDEXER_H_INCLUDED_
/* @(#$Id: indexer.h,v 1.5 2005-03-17 15:35:49 dockes Exp $  (C) 2004 J.F.Dockes */

#include "rclconfig.h"
class DbIndexer;

/**
 * The file system indexing object. Processes the configuration, then invokes
 * file system walking to populate/update the database(s).
 */
class ConfIndexer {
    RclConfig *config;
    DbIndexer *indexer; // Internal object used to store opaque private data
 public:
    enum runStatus {IndexerOk, IndexerError};
    ConfIndexer(RclConfig *cnf) : config(cnf), indexer(0) {}
    virtual ~ConfIndexer();
    bool index();
};

#endif /* _INDEXER_H_INCLUDED_ */
