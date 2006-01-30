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
/* @(#$Id: history.h,v 1.3 2006-01-30 11:15:28 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>
#include <utility>

#include "conftree.h"

/** Holder for data returned when querying history */
class RclDHistoryEntry {
 public:
    RclDHistoryEntry() : unixtime(0) {}
    long unixtime;
    string fn;
    string ipath;
};

/** 
 * The documents history class. This is based on a ConfTree for no
 * imperative reason
 */
class RclDHistory {
 public:
    RclDHistory(const std::string &fn, unsigned int maxsize=1000);
    bool ok() {return m_data.getStatus() == ConfSimple::STATUS_RW;}

    bool enterDocument(const std::string fn, const std::string ipath);
    std::list<RclDHistoryEntry> getDocHistory();

 private:
    bool decodeValue(const string &value, RclDHistoryEntry *e);
    unsigned int m_mlen;
    ConfSimple m_data;
};


#endif /* _HISTORY_H_INCLUDED_ */
