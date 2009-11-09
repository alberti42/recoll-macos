#ifndef _circache_h_included_
#define _circache_h_included_
/* @(#$Id: $  (C) 2009 J.F.Dockes */
/**
 * A data cache implemented as a circularly managed file
 *
 * This is used to store cached remote pages for recoll. A single file is used
 * to store the compressed pages and the associated metadata. The file
 * grows to a specified maximum size, then is rewritten from the
 * start, overwriting older entries.
 *
 * Data objects inside the cache each have two parts: a data segment and an 
 * attribute (metadata) dictionary.
 * They are named using the same identifiers that are used inside the Recoll
 * index, but any unique identifier scheme would work.
 *
 * The names are stored in an auxiliary index for fast access. This index can
 * be rebuilt from the main file.
 */

#include <sys/types.h>

#include <string>

#ifndef NO_NAMESPACES
using std::string;
#endif

class CirCacheInternal;
class CirCache {
public:
    CirCache(const string& dir);
    ~CirCache();

    string getReason();

    bool create(off_t maxsize);

    enum OpMode {CC_OPREAD, CC_OPWRITE};
    bool open(OpMode mode);

    bool get(const string& udi, string dic, string data);

    bool put(const string& udi, const string& dic, const string& data);

private:
    CirCacheInternal *m_d;
    string m_dir;
};

#endif /* _circache_h_included_ */
