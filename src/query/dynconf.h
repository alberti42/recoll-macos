/* Copyright (C) 2004 J.F.Dockes
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
#ifndef _DYNCONF_H_INCLUDED_
#define _DYNCONF_H_INCLUDED_

/**
 * Dynamic configuration storage
 *
 * This used to be called "history" because of the initial usage.
 * Used to store some parameters which would fit neither in recoll.conf,
 * basically because they change a lot, nor in the QT preferences file, mostly 
 * because they are specific to a configuration directory.
 * Examples:
 *  - History of documents selected for preview
 *  - Active and inactive external databases (depend on the 
 *    configuration directory)
 *  - ...
 *
 * The storage is performed in a ConfSimple file, with subkeys and
 * encodings which depend on the data stored. Under each section, the keys 
 * are sequential numeric, so this basically manages a set of lists.
 *
 */

#include <string>
#include <list>
#include <utility>

#include "conftree.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif

// Entry interface.
class DynConfEntry {
 public:
    virtual ~DynConfEntry() {}
    virtual bool decode(const string &value) = 0;
    virtual bool encode(string& value) = 0;
    virtual bool equal(const DynConfEntry &other) = 0;
};


/** String storage generic object */
class RclSListEntry : public DynConfEntry {
 public:
    RclSListEntry() {}
    RclSListEntry(const string& v) : value(v) {}
    virtual ~RclSListEntry() {}
    virtual bool decode(const string &enc);
    virtual bool encode(string& enc);
    virtual bool equal(const DynConfEntry& other);

    string value;
};

/** The dynamic configuration class */
class RclDynConf {
 public:
    RclDynConf(const string &fn)
	: m_data(fn.c_str()) {}
    bool ok() {return m_data.getStatus() == ConfSimple::STATUS_RW;}
    string getFilename() {return m_data.getFilename();}

    // Generic methods
    bool eraseAll(const string& sk);
    bool insertNew(const string& sk, DynConfEntry &n, DynConfEntry &s, 
                   int maxlen = -1);
    template<typename Tp> list<Tp> getList(const string& sk);

    // Specialized methods for simple string lists, designated by the
    // subkey value
    bool enterString(const string sk, const string value, int maxlen = -1);
    list<string> getStringList(const string sk);

 private:
    unsigned int m_mlen;
    ConfSimple   m_data;

};

template<typename Tp> list<Tp> RclDynConf::getList(const string &sk)
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

// Defined subkeys. Values in dynconf.cpp
// History
extern const string docHistSubKey;
// All external indexes
extern const string allEdbsSk;
// Active external indexes
extern const string actEdbsSk;

#endif /* _DYNCONF_H_INCLUDED_ */
