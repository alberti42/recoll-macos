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
#ifndef _circache_h_included_
#define _circache_h_included_
/* @(#$Id: $  (C) 2009 J.F.Dockes */
/**
 * A data cache implemented as a circularly managed file
 *
 * A single file is used to stored objects. The file grows to a
 * specified maximum size, then is rewritten from the start,
 * overwriting older entries.
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
    virtual ~CirCache();

    virtual string getReason();

    virtual bool create(off_t maxsize);

    enum OpMode {CC_OPREAD, CC_OPWRITE};
    virtual bool open(OpMode mode);

    virtual bool get(const string& udi, string& dic, string& data, 
                     int instance = -1);

    virtual bool put(const string& udi, const string& dic, const string& data);

    /* Debug */
    virtual bool dump();

protected:
    CirCacheInternal *m_d;
    string m_dir;
};

#endif /* _circache_h_included_ */
