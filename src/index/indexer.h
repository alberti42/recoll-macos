#ifndef _INDEXER_H_INCLUDED_
#define _INDEXER_H_INCLUDED_
/* @(#$Id: indexer.h,v 1.3 2005-01-25 14:37:21 dockes Exp $  (C) 2004 J.F.Dockes */

#include "rclconfig.h"

#if 0
class FsIndexer {
    const ConfTree &conf;
 public:
    enum runStatus {IndexerOk, IndexerError};
    Indexer(const ConfTree &cnf): conf(cnf) {}
    virtual ~Indexer() {}
    runStatus run() = 0;
};
#endif

#endif /* _INDEXER_H_INCLUDED_ */
