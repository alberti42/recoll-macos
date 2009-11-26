#ifndef _rclquery_h_included_
#define _rclquery_h_included_
/* @(#$Id: rclquery.h,v 1.6 2008-09-29 11:33:55 dockes Exp $  (C) 2008 J.F.Dockes */
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
 *
 */
class Query {
 public:
    /** The constructor only allocates memory */
    Query(Db *db);
    ~Query();

    /** Get explanation about last error */
    string getReason() const;

    /** Choose sort order. Must be called before setQuery */
    void setSortBy(const string& fld, bool ascending = true);
    const string& getSortBy() const {return m_sortField;}
    bool getSortAscending() const {return m_sortAscending;}

    /** Return or filter results with identical content checksum */
    void setCollapseDuplicates(bool on) {m_collapseDuplicates = on;}

    /** Accept data describing the search and query the index. This can
     * be called repeatedly on the same object which gets reinitialized each
     * time.
     */
    bool setQuery(RefCntr<SearchData> q);

    /** Get results count for current query */
    int getResCnt();

    /** Get document at rank i in current query results. */
    bool getDoc(int i, Doc &doc);

    /** Get possibly expanded list of query terms */
    bool getQueryTerms(list<string>& terms);

    /** Return a list of terms which matched for a specific result document */
    bool getMatchTerms(const Doc& doc, list<string>& terms);
    bool getMatchTerms(unsigned long xdocid, list<string>& terms);

    /** Expand query to look for documents like the one passed in */
    list<string> expand(const Doc &doc);

    /** Return the Db we're set for */
    Db *whatDb();

    /* make this public for access from embedded Db::Native */
    class Native;
    Native *m_nq;

private:
    string m_filterTopDir; // Current query filter on subtree top directory 
    string m_reason; // Error explanation
    Db    *m_db;
    void  *m_sorter;
    string m_sortField;
    bool   m_sortAscending;
    bool   m_collapseDuplicates;     
    /* Copyconst and assignement private and forbidden */
    Query(const Query &) {}
    Query & operator=(const Query &) {return *this;};
};

#ifndef NO_NAMESPACES
}
#endif // NO_NAMESPACES


#endif /* _rclquery_h_included_ */
