#ifndef _STOPLIST_H_INCLUDED_
#define _STOPLIST_H_INCLUDED_
/* @(#$Id: stoplist.h,v 1.1 2007-06-02 08:30:42 dockes Exp $  (C) 2006 J.F.Dockes */

#include <set>
#include <string>

#include "textsplit.h"

#ifndef NO_NAMESPACES
using std::set;
using std::string;
namespace Rcl 
{
#endif

class StopList : public TextSplitCB {
public:
    StopList() : m_hasStops(false) {}
    StopList(const string &filename) {setFile(filename);}
    virtual ~StopList() {}

    bool setFile(const string &filename);
    bool isStop(const string &term) const;
    bool hasStops() const {return m_hasStops;}
    virtual bool takeword(const string& term, int pos, int bts, int bte); 

private:
    bool m_hasStops;
    set<string> m_stops;
};

#ifndef NO_NAMESPACES
}
#endif

#endif /* _STOPLIST_H_INCLUDED_ */
