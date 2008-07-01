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
    Xapian::Query    query; 

    /** In case there is a postq filter: sequence of db indices that match */
    vector<int> m_dbindices; 

    // Filtering results on location. There are 2 possible approaches
    // for this:
    //   - Set a "MatchDecider" to be used by Xapian during the query
    //   - Filter the results out of Xapian (this also uses a
    //     Xapian::MatchDecider object, but applied to the results by Recoll.
    // 
    // The result filtering approach was the first implemented. 
    //
    // The efficiency of both methods depend on the searches, so the code
    // for both has been kept.  A nice point for the Xapian approach is that
    // the result count estimate are correct (they are wrong with
    // the postfilter approach). It is also faster in some worst case scenarios
    // so this now the default (but the post-filtering is faster in many common
    // cases).
    // 
    // Which is used is decided in SetQuery(), by setting either of
    // the two following members. This in turn is controlled by a
    // preprocessor directive.
#define XAPIAN_FILTERING 1
    Xapian::MatchDecider *decider;   // Xapian does the filtering
    Xapian::MatchDecider *postfilter; // Result filtering done by Recoll

    Xapian::Enquire      *enquire; // Open query descriptor.
    Xapian::MSet          mset;    // Partial result set
    // Term frequencies for current query. See makeAbstract, setQuery
    map<string, double>  termfreqs; 

    Native(Query *q)
	: m_q(q), decider(0), postfilter(0), enquire(0)
    { }
    ~Native() {
	clear();
    }
    void clear() {
	m_dbindices.clear();
	delete decider; decider = 0;
	delete postfilter; postfilter = 0;
	delete enquire; enquire = 0;
	termfreqs.clear();
    }
};

}
#endif /* _rclquery_p_h_included_ */
