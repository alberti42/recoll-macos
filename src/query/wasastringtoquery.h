#ifndef _WASASTRINGTOQUERY_H_INCLUDED_
#define _WASASTRINGTOQUERY_H_INCLUDED_
/* @(#$Id: wasastringtoquery.h,v 1.1 2006-11-30 18:12:16 dockes Exp $  (C) 2006 J.F.Dockes */
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

// A simple class to represent a parsed wasabi query string.
class WasaQuery {
public:
    enum Op {OP_NULL, OP_LEAF, OP_EXCL, OP_OR, OP_AND};
    typedef vector<WasaQuery*> subqlist_t;

    WasaQuery() :  m_op(OP_NULL) {}
    ~WasaQuery();

    // Get string describing this query
    void describe(string &desc) const;

    WasaQuery::Op            m_op;
    string                   m_fieldspec;
    vector<WasaQuery*>       m_subs;
    string                   m_value;
};


// Wasabi query string parser class.
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
