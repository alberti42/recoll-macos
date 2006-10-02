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
#ifndef _HISTORY_H_INCLUDED_
#define _HISTORY_H_INCLUDED_
/* @(#$Id: history.h,v 1.6 2006-10-02 12:33:13 dockes Exp $  (C) 2004 J.F.Dockes */

/**
 * Dynamic configuration storage
 *
 * The term "history" is a misnomer as this code is now used to save
 * all dynamic parameters except those that are stored in the global
 * recollrc QT preferences file. Examples:
 *  - History of documents selected for preview
 *  - Active and inactive external databases (these should depend on the 
 *    configuration directory and cant be stored in recollrc).
 *  - ...
 *
 * The storage is performed in a ConSimple file, with subkeys and
 * encodings which depend on the data stored.
 *
 */

#include <string>
#include <list>
#include <utility>

#include "conftree.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif

class HistoryEntry {
 public:
    virtual ~HistoryEntry() {}
    virtual bool decode(const string &value) = 0;
    virtual bool encode(string& value) = 0;
    virtual bool equal(const HistoryEntry &other) = 0;
};


/** Document history entry */
class RclDHistoryEntry : public HistoryEntry {
 public:
    RclDHistoryEntry() : unixtime(0) {}
    RclDHistoryEntry(long t, const string& f, const string& i) 
	: unixtime(t), fn(f), ipath(i) {}
    virtual ~RclDHistoryEntry() {}
    virtual bool decode(const string &value);
    virtual bool encode(string& value);
    virtual bool equal(const HistoryEntry& other);
    long unixtime;
    string fn;
    string ipath;
};


/** String storage generic object */
class RclSListEntry : public HistoryEntry {
 public:
    RclSListEntry() {}
    RclSListEntry(const string& v) : value(v) {}
    virtual ~RclSListEntry() {}
    virtual bool decode(const string &enc);
    virtual bool encode(string& enc);
    virtual bool equal(const HistoryEntry& other);

    string value;
};

/** 
 * The history class. This uses a ConfSimple for storage, and should be 
 * renamed something like dynconf, as it is used to stored quite a few 
 * things beyond doc history: all dynamic configuration parameters that are 
 * not suitable for QT settings because they are specific to a RECOLL_CONFDIR
 */
class RclHistory {
 public:
    RclHistory(const string &fn, unsigned int maxsize=100)
	: m_mlen(maxsize), m_data(fn.c_str()) {}
    bool ok() {return m_data.getStatus() == ConfSimple::STATUS_RW;}

    // Specific methods for history entries
    bool enterDoc(const string fn, const string ipath);
    list<RclDHistoryEntry> getDocHistory();

    // Generic methods used for string lists, designated by the subkey value
    bool enterString(const string sk, const string value);
    list<string> getStringList(const string sk);
    bool eraseAll(const string& sk);
    bool truncate(const string& sk, unsigned int n);

 private:
    unsigned int m_mlen;
    ConfSimple m_data;
    bool insertNew(const string& sk, HistoryEntry &n, HistoryEntry &s);
    template<typename Tp> list<Tp> getHistory(const string& sk);
};

template<typename Tp> list<Tp> RclHistory::getHistory(const string &sk)
{
    list<Tp> mlist;
    Tp entry;
    list<string> names = m_data.getNames(sk);
    for (list<string>::const_iterator it = names.begin(); 
	 it != names.end(); it++) {
	string value;
	if (m_data.get(*it, value, sk)) {
	    if (!entry.decode(value))
		continue;
	    mlist.push_front(entry);
	}
    }
    return mlist;
}

#endif /* _HISTORY_H_INCLUDED_ */
