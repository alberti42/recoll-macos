/* Copyright (C) 2005 J.F.Dockes
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
#include "docseqhist.h"

#include <stdio.h>
#include <math.h>
#include <time.h>

#include <cmath>
using std::vector;

#include "rcldb.h"
#include "fileudi.h"
#include "base64.h"
#include "log.h"
#include "smallut.h"

// Encode document history entry: 
// U + Unix time + base64 of udi
// The U distinguishes udi-based entries from older fn+ipath ones
bool RclDHistoryEntry::encode(string& value)
{
    string budi;
    base64_encode(udi, budi);
    value = string("U ") + lltodecstr(unixtime) + " " + budi;
    return true;
}

// Decode. We support historical entries which were like "time b64fn [b64ipath]"
// Current entry format is "U time b64udi"
bool RclDHistoryEntry::decode(const string &value)
{
    vector<string> vall;
    stringToStrings(value, vall);

    vector<string>::const_iterator it = vall.begin();
    udi.erase();
    string fn, ipath;
    switch (vall.size()) {
    case 2:
        // Old fn+ipath, null ipath case 
        unixtime = atoll((*it++).c_str());
        base64_decode(*it++, fn);
        break;
    case 3:
        if (!it->compare("U")) {
            // New udi-based entry
            it++;
            unixtime = atoll((*it++).c_str());
            base64_decode(*it++, udi);
        } else {
            // Old fn + ipath. We happen to know how to build an udi
            unixtime = atoll((*it++).c_str());
            base64_decode(*it++, fn);
            base64_decode(*it, ipath);
        }
        break;
    default: 
        return false;
    }

    if (!fn.empty()) {
        // Old style entry found, make an udi, using the fs udi maker
        make_udi(fn, ipath, udi);
    }
    LOGDEB1("RclDHistoryEntry::decode: udi ["  << udi << "]\n");
    return true;
}

bool RclDHistoryEntry::equal(const DynConfEntry& other)
{
    const RclDHistoryEntry& e = dynamic_cast<const RclDHistoryEntry&>(other);
    return e.udi == udi;
}

bool historyEnterDoc(RclDynConf *dncf, const string& udi)
{
    LOGDEB1("historyEnterDoc: [" << udi << "] into " << dncf->getFilename() <<
            "\n");
    RclDHistoryEntry ne(time(0), udi);
    RclDHistoryEntry scratch;
    return dncf->insertNew(docHistSubKey, ne, scratch, 200);
}

vector<RclDHistoryEntry> getDocHistory(RclDynConf* dncf)
{
    return dncf->getEntries<std::vector, RclDHistoryEntry>(docHistSubKey);
}

bool DocSequenceHistory::getDoc(int num, Rcl::Doc &doc, string *sh) 
{
    // Retrieve history list
    if (!m_hist)
	return false;
    if (m_history.empty())
	m_history = getDocHistory(m_hist);

    if (num < 0 || num >= (int)m_history.size())
	return false;
    // We get the history oldest first, but our users expect newest first
    RclDHistoryEntry& hentry = m_history[m_history.size() - 1 - num];
    if (sh) {
	if (m_prevtime < 0 || 
            abs (float(m_prevtime) - float(hentry.unixtime)) > 86400) {
	    m_prevtime = hentry.unixtime;
	    time_t t = (time_t)(hentry.unixtime);
	    *sh = string(ctime(&t));
	    // Get rid of the final \n in ctime
	    sh->erase(sh->length()-1);
	} else {
	    sh->erase();
        }
    }

    // For now history does not store an index id. Use empty doc as ref.
    Rcl::Doc idxdoc;
    bool ret = m_db->getDoc(hentry.udi, idxdoc, doc);
    if (!ret || doc.pc == -1) {
	doc.url = "UNKNOWN";
        doc.ipath = "";
    }

    // Ensure the snippets link won't be shown as it does not make
    // sense (no query terms...)
    doc.haspages = 0;

    return ret;
}

Rcl::Db *DocSequenceHistory::getDb()
{
    return m_db;
}

int DocSequenceHistory::getResCnt()
{	
    if (m_history.empty())
	m_history = getDocHistory(m_hist);
    return int(m_history.size());
}

