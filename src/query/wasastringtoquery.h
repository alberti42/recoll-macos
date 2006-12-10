#ifndef _WASASTRINGTOQUERY_H_INCLUDED_
#define _WASASTRINGTOQUERY_H_INCLUDED_
/* @(#$Id: wasastringtoquery.h,v 1.3 2006-12-10 17:03:08 dockes Exp $  (C) 2006 J.F.Dockes */
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
#include <vector>

using std::string;
using std::vector;

/** 
 * A simple class to represent a parsed wasabiSimple query
 * string. Can hold a string value or an array of subqueries.
 * The value can hold one or several words. In the latter case, it should 
 * be interpreted as a phrase (comes from a user-entered "quoted string").
 */
class WasaQuery {
public:
    enum Op {OP_NULL, OP_LEAF, OP_EXCL, OP_OR, OP_AND};
    typedef vector<WasaQuery*> subqlist_t;

    WasaQuery() 
	: m_op(OP_NULL), m_typeKind(WQTK_NONE)
    {}
    ~WasaQuery();

    // Get string describing the query tree from this point
    void describe(string &desc) const;

    WasaQuery::Op      m_op;
    string             m_fieldspec;
    /* Valid for op == OP_LEAF */
    string             m_value;
    /* Valid for conjunctions */
    vector<WasaQuery*> m_subs;
    
    /* Restrict results to some file type, defined by either mime, app group, 
     * or extension */
    enum TypeKind {WQTK_NONE, WQTK_MIME, WQTK_GROUP, WQTK_EXT};
    TypeKind           m_typeKind;
    vector<string>     m_types;

    /* Sort on relevance, date, name or group */
    enum SortKind {WQSK_REL, WQSK_DATE, WQSK_ALPHA, WQSK_GROUP};
    vector<SortKind>   m_sortSpec;
};

/**
 * Wasabi query string parser class. Could be a simple function
 * really, but there might be some parser initialization work done in
 * the constructor.
 */
class StringToWasaQuery {
public:
    StringToWasaQuery();
    ~StringToWasaQuery();
    WasaQuery *stringToQuery(const string& str, string& reason);
    class Internal;
private:
    Internal *internal;
};

#endif /* _WASASTRINGTOQUERY_H_INCLUDED_ */
