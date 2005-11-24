#ifndef _HISTORY_H_INCLUDED_
#define _HISTORY_H_INCLUDED_
/* @(#$Id: history.h,v 1.1 2005-11-24 18:21:55 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>
#include <utility>

#include "conftree.h"

/** 
 * The query and documents history class. This is based on a ConfTree for no 
 * imperative reason
 */
class RclQHistory {
 public:
    RclQHistory(const std::string &fn, unsigned int maxsize=1000);
    bool ok() {return m_data.getStatus() == ConfSimple::STATUS_RW;}

    bool enterDocument(const std::string fn, const std::string ipath);
    std::list< std::pair<std::string, std::string> > getDocHistory();

 private:
    unsigned int m_mlen;
    ConfSimple m_data;
};


#endif /* _HISTORY_H_INCLUDED_ */
