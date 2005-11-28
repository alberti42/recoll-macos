#ifndef _HISTORY_H_INCLUDED_
#define _HISTORY_H_INCLUDED_
/* @(#$Id: history.h,v 1.2 2005-11-28 15:31:01 dockes Exp $  (C) 2004 J.F.Dockes */

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
