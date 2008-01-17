#ifndef _WASASTRINGTOQUERY_H_INCLUDED_
#define _WASASTRINGTOQUERY_H_INCLUDED_
/* @(#$Id: wasastringtoquery.h,v 1.6 2008-01-17 11:14:13 dockes Exp $  (C) 2006 J.F.Dockes */
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
 * A simple class to represent a parsed wasabiSimple query element. 
 * Can hold a string value or an array of subqueries.
 *
 * The complete query is represented by a top WasaQuery holding a
 * chain of ANDed subclauses. Some of the subclauses may be themselves
 * OR'ed lists (it doesn't go deeper). Entries in the AND list may be
 * negated (AND NOT).
 *
 * For LEAF elements, the value can hold one or several words. In the
 * latter case, it should be interpreted as a phrase (comes from a
 * user-entered "quoted string").
 * 
 * Some fields only make sense either for compound or LEAF queries. This 
 * is commented for each. We should subclass really.
 */
class WasaQuery {
public:
    enum Op {OP_NULL, OP_LEAF, OP_EXCL, OP_OR, OP_AND};
    typedef vector<WasaQuery*> subqlist_t;

    WasaQuery() 
	: m_op(OP_NULL), m_modifiers(0)
    {}

    ~WasaQuery();

    /** Get string describing the query tree from this point */
    void describe(string &desc) const;

    /** Op to be performed on either value (may be LEAF or EXCL, or subqs */
    WasaQuery::Op      m_op;

    /** Field specification if any (ie: title, author ...) Only OPT_LEAF */
    string             m_fieldspec;

    /* String value. Valid for op == OP_LEAF or EXCL */
    string             m_value;

    /** Subqueries. Valid for conjunctions */
    vector<WasaQuery*> m_subs;
    
    /** Modifiers for term handling: case/diacritics handling,
	stemming control */
    enum Modifier {WQM_CASESENS = 1, WQM_DIACSENS = 2, WQM_NOSTEM = 4, 
		   WQM_BOOST = 8, WQM_PROX = 0x10, WQM_SLOPPY = 0x20, 
		   WQM_WORDS = 0x40, WQM_PHRASESLACK = 0x80, WQM_REGEX = 0x100,
		   WQM_FUZZY = 0x200};
    unsigned int       m_modifiers;
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
