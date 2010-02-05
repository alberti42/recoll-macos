#ifndef _beaglequeuecache_h_included_
#define _beaglequeuecache_h_included_
/* @(#$Id: $  (C) 2009 J.F.Dockes */

#include <string>
using std::string;

class RclConfig;
namespace Rcl {
    class Db;
    class Doc;
}
class CirCache;

/**
 * Manage the CirCache for the Beagle Queue indexer. Separated from the main
 * indexer code because it's also used for querying (getting the data for a
 * preview 
 */
class BeagleQueueCache {
public:
    BeagleQueueCache(RclConfig *config);
    ~BeagleQueueCache();

    bool getFromCache(const string& udi, Rcl::Doc &doc, string& data,
                      string *hittype = 0);
    // We could write proxies for all the circache ops, but why bother?
    CirCache *cc() {return m_cache;}

private:
    CirCache *m_cache;
};

#endif /* _beaglequeuecache_h_included_ */
