#ifndef _INDEXER_H_INCLUDED_
#define _INDEXER_H_INCLUDED_
/* @(#$Id: indexer.h,v 1.4 2005-01-31 14:31:09 dockes Exp $  (C) 2004 J.F.Dockes */

#include "rclconfig.h"
class DbIndexer;
class ConfIndexer {
    RclConfig *config;
    DbIndexer *indexer;
 public:
    enum runStatus {IndexerOk, IndexerError};
    ConfIndexer(RclConfig *cnf) : config(cnf), indexer(0) {}
    virtual ~ConfIndexer();
    bool index();
};

#endif /* _INDEXER_H_INCLUDED_ */
