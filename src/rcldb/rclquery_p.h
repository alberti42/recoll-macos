#ifndef _rclquery_p_h_included_
#define _rclquery_p_h_included_
/* @(#$Id: rclquery_p.h,v 1.2 2008-07-01 08:31:08 dockes Exp $  (C) 2007 J.F.Dockes */

#include <map>
#include <vector>

using std::map;
using std::vector;

#include <xapian.h>
#include "rclquery.h"

namespace Rcl {

class Query::Native {
public:
    /** The query I belong to */
    Query                *m_q;


    /** query descriptor: terms and subqueries joined by operators
     * (or/and etc...)
     */
    Xapian::Query    xquery; 

    Xapian::Enquire      *xenquire; // Open query descriptor.
    Xapian::MSet          xmset;    // Partial result set
    // Term frequencies for current query. See makeAbstract, setQuery
    map<string, double>  termfreqs; 

    Native(Query *q)
	: m_q(q), xenquire(0)
    { }
    ~Native() {
	clear();
    }
    void clear() {
	delete xenquire; xenquire = 0;
	termfreqs.clear();
    }
};

}
#endif /* _rclquery_p_h_included_ */
