#ifndef _rclquery_h_included_
#define _rclquery_h_included_
/* @(#$Id: rclquery.h,v 1.1 2008-06-13 18:22:46 dockes Exp $  (C) 2008 J.F.Dockes */
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
#include <string>
#include <list>
#include <vector>

#ifndef NO_NAMESPACES
using std::string;
using std::list;
using std::vector;
#endif

#include "refcntr.h"

#ifndef NO_NAMESPACES
namespace Rcl {
#endif

class SearchData;
class Db;
class Doc;

/**
 * An Rcl::Query is a question (SearchData) applied to a
 * database. Handles access to the results. Somewhat equivalent to a
 * cursor in an rdb.
 */
class Query {
 public:
    enum QueryOpts {QO_NONE=0, QO_STEM = 1};

    Query(Db *db);

    ~Query();

    /** Get explanation about last error */
    string getReason() const;

    /** Parse query string and initialize query */
    bool setQuery(RefCntr<SearchData> q, int opts = QO_NONE,
		  const string& stemlang = "english");
    bool getQueryTerms(list<string>& terms);
    bool getMatchTerms(const Doc& doc, list<string>& terms);

    /** Get document at rank i in current query. */
    bool getDoc(int i, Doc &doc, int *percent = 0);

    /** Expand query */
    list<string> expand(const Doc &doc);

    /** Get results count for current query */
    int getResCnt();

    Db *whatDb();

    /** make this public for access from embedded Db::Native */
    class Native;
    Native *m_nq;

private:
    string m_filterTopDir; // Current query filter on subtree top directory 
    string m_reason; // Error explanation
    Db    *m_db;
    unsigned int m_qOpts;
    /* Copyconst and assignemt private and forbidden */
    Query(const Query &) {}
    Query & operator=(const Query &) {return *this;};
};

#ifndef NO_NAMESPACES
}
#endif // NO_NAMESPACES


#endif /* _rclquery_h_included_ */
