#ifndef _INDEXER_H_INCLUDED_
#define _INDEXER_H_INCLUDED_
/* @(#$Id: indexer.h,v 1.1 2004-12-14 17:53:51 dockes Exp $  (C) 2004 J.F.Dockes */

#include "rclconfig.h"

/* Definition for document interner functions */
typedef Rcl::Doc* (*MimeHandlerFunc)(RclConfig *, const string &, 
				    const string &);


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
