#ifndef lint
static char rcsid[] = "@(#$Id: wasastringtoquery.cpp,v 1.3 2006-12-10 17:03:08 dockes Exp $ (C) 2006 J.F.Dockes";
#endif
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
#ifndef TEST_STRINGTOQUERY
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "wasastringtoquery.h"

WasaQuery::~WasaQuery()
{
    for (vector<WasaQuery*>::iterator it = m_subs.begin();
	 it != m_subs.end(); it++) {
	delete *it;
    }
    m_subs.clear();
}

void WasaQuery::describe(string &desc) const
{
    if (!m_types.empty()) {
	desc += "type_restrict(";
	for (vector<string>::const_iterator it = m_types.begin();
	     it != m_types.end(); it++) {
	    desc += *it + ", ";
	}
	desc.erase(desc.size() - 2);
	desc += ")";
    }
    if (m_sortSpec.size() > 1 || 
	(m_sortSpec.size() == 1 && m_sortSpec[0] != WQSK_REL)) {
	desc += "sort_by(";
	for (vector<SortKind>::const_iterator it = m_sortSpec.begin();
	     it != m_sortSpec.end(); it++) {
	    switch (*it) {
	    case WQSK_DATE: desc += string("date") + ", ";break;
	    case WQSK_ALPHA: desc += string("name") + ", ";break;
	    case WQSK_GROUP: desc += string("group") + ", ";break;
	    case WQSK_REL: default: desc += string("relevance") + ", ";break;
	    }
	}
	desc.erase(desc.size() - 2);
	desc += ")";
    }
    desc += "(";
    switch (m_op) {
    case OP_NULL: 
	desc += "NULL"; 
	break;
    case OP_LEAF: 
	desc += m_fieldspec.empty() ?
	    m_value : m_fieldspec + ":" + m_value;
	break;
    case OP_EXCL: 
	desc += string("NOT (" ) + m_value + ") ";
	break;
    case OP_OR: 
    case OP_AND:
	for (vector<WasaQuery *>::const_iterator it = m_subs.begin();
	     it != m_subs.end(); it++) {
	    (*it)->describe(desc);
	    vector<WasaQuery *>::const_iterator it1 = it;
	    it1++;
	    if (it1 != m_subs.end())
		desc += m_op == OP_OR ? "OR ": "AND ";
	}
	break;
    }
    desc += ") "; 
}

// The string query parser code:

/* Shamelessly lifted from Beagle:			
 * This is our regular Expression Pattern:
 * we expect something like this:
 * -key:"Value String"
 * key:Value
 * or
 * Value
 ([+-]?)		# Required or Prohibited (optional)
 (\w+:)?		# Key  (optional)
 (		# Query Text
 (\"([^\"]*)\"?)#  quoted
 |		#  or
 ([^\s\"]+)	#  unquoted
 )
 ";
*/

/* The master regular expression used to parse a query string
 * Sub-expressions in parenthesis are numbered from 1. Each opening
 * parenthesis increases the index, but we're not interested in all
 */
static const char * parserExpr = 
    "([oO][rR])"                     //1 OR is a special word
    "|"
    "("                              //2
      "([+-])?"                      //3 Force or exclude indicator
      "("                            //4
        "([[:alpha:]][[:alnum:]]+)"  //5 Field spec: "fieldname:"
      ":)?"
      "("                            //6
        "(\""                        //7
          "([^\"]+)"                 //8 "A quoted term"
        "\")"                        
        "|"
        "([^[:space:]]+)"            //9 ANormalTerm
      ")"
    ")"
;

// For debugging the parser. But see also NMATCH
static const char *matchNames[] = {
     /*0*/   "",
     /*1*/   "OR",
     /*2*/   "",
     /*3*/   "+-",
     /*4*/   "",
     /*5*/   "FIELD",
     /*6*/   "",
     /*7*/   "",
     /*8*/   "QUOTEDTERM",
     /*9*/   "TERM",
};
#define NMATCH (sizeof(matchNames) / sizeof(char *))

// Symbolic names for the interesting submatch indices
enum SbMatchIdx {SMI_OR=1, SMI_PM=3, SMI_FIELD=5, SMI_QUOTED=8, SMI_TERM=9};

static const int maxmatchlen = 1024;
static const int errbuflen = 300;

class StringToWasaQuery::Internal {
public:
    Internal() 
	: m_rxneedsfree(false)
    {}
    ~Internal()
    {
	if (m_rxneedsfree)
	    regfree(&m_rx);
    }
    bool checkSubMatch(int i, char *match, string& reason)
    {
	if (i < 0 || i >= int(NMATCH) || m_pmatch[i].rm_so == -1)
	    return false;
	if (m_pmatch[i].rm_eo - m_pmatch[i].rm_so <= 0) {
	    // weird and fatal
	    reason = "Internal regular expression handling error";
	    return false;
	}
	memcpy(match, m_cp + m_pmatch[i].rm_so, 
	       m_pmatch[i].rm_eo - m_pmatch[i].rm_so);
	match[m_pmatch[i].rm_eo - m_pmatch[i].rm_so] = 0;
	return true;
    }

    WasaQuery *stringToQuery(const string& str, string& reason);

    friend class StringToWasaQuery;
private:
    const char *m_cp;
    regex_t     m_rx;
    bool        m_rxneedsfree;
    regmatch_t  m_pmatch[NMATCH];
};

StringToWasaQuery::StringToWasaQuery() 
    : internal(new Internal)
{
}

StringToWasaQuery::~StringToWasaQuery()
{
    delete internal;
}

WasaQuery *
StringToWasaQuery::stringToQuery(const string& str, string& reason)
{
    return internal ? internal->stringToQuery(str, reason) : 0;
}

WasaQuery *
StringToWasaQuery::Internal::stringToQuery(const string& str, string& reason)
{
    if (m_rxneedsfree)
	regfree(&m_rx);

    char errbuf[errbuflen+1];
    int errcode;
    if ((errcode = regcomp(&m_rx, parserExpr, REG_EXTENDED)) != 0) {
	regerror(errcode, &m_rx, errbuf, errbuflen);
	reason = errbuf;
	return 0;
    }
    m_rxneedsfree = true;

    const char *cpe;
    m_cp = str.c_str();
    cpe = str.c_str() + str.length();

    WasaQuery *query = new WasaQuery;
    query->m_op = WasaQuery::OP_AND;
    WasaQuery *orClause = 0;
    bool prev_or = false;

    // Loop on repeated regexp matches on the main string.
    for (int loop = 0;;loop++) {
	if ((errcode = regexec(&m_rx, m_cp, NMATCH, m_pmatch, 0))) {
	    regerror(errcode, &m_rx, errbuf, errbuflen);
	    reason = errbuf;
	    return 0;
	}
	if (m_pmatch[0].rm_eo <= 0) {
	    // weird and fatal
	    reason = "Internal regular expression handling error";
	    return 0;
	}
#if 0
	if (loop) printf("Next part:\n");
	for (i = 0; i < NMATCH; i++) {
	    if (m_pmatch[i].rm_so == -1) 	continue;
	    char match[maxmatchlen+1];
	    memcpy(match, m_cp + m_pmatch[i].rm_so,
		   m_pmatch[i].rm_eo - m_pmatch[i].rm_so);
	    match[m_pmatch[i].rm_eo - m_pmatch[i].rm_so] = 0;
	    if (matchNames[i][0])
		printf("%10s: [%s] (%d->%d)\n", matchNames[i], match, 
		       (int)m_pmatch[i].rm_so, (int)m_pmatch[i].rm_eo);
	}
#endif
	char match[maxmatchlen+1];
	if (checkSubMatch(SMI_OR, match, reason)) {
	    if (prev_or) {
		// Bad syntax
		reason = "Bad syntax: consecutive OR";
		return 0;
	    }

	    if (orClause == 0) {
		// Fist OR seen: start OR subclause.
		if ((orClause = new WasaQuery()) == 0) {
		    reason = "Out of memory";
		    return 0;
		}
		orClause->m_op = WasaQuery::OP_OR;
	    }

	    // We need to transfer the previous query from the main vector
	    // to the OR subquery
	    if (!query->m_subs.empty()) {
		orClause->m_subs.push_back(query->m_subs.back());
		query->m_subs.pop_back();
	    }
	    prev_or = true;

	} else {

	    WasaQuery *nclause = new WasaQuery;
	    if (nclause == 0) {
		reason = "Out of memory";
		return 0;
	    }

	    // Check for quoted or unquoted value
	    if (checkSubMatch(SMI_QUOTED, match, reason)) {
		nclause->m_value = match;
	    } else if (checkSubMatch(SMI_TERM, match, reason)) {
		nclause->m_value = match;
	    }
	    if (nclause->m_value.empty()) {
		// Isolated +- or fieldname: without a value. Ignore until
		// told otherwise.
		delete nclause;
		goto nextfield;
	    }

	    // Field indicator ?
	    if (checkSubMatch(SMI_FIELD, match, reason)) {
		// Check for special fields
		if (match == string("mime")) {
		    if (query->m_typeKind == WasaQuery::WQTK_NONE)
			query->m_typeKind = WasaQuery::WQTK_MIME;
		    if (query->m_typeKind == WasaQuery::WQTK_MIME)
			query->m_types.push_back(nclause->m_value);
		    delete nclause;
		    goto nextfield;
		} else if (match == string("group")) {
		    if (query->m_typeKind == WasaQuery::WQTK_NONE)
			query->m_typeKind = WasaQuery::WQTK_GROUP;
		    if (query->m_typeKind == WasaQuery::WQTK_GROUP)
			query->m_types.push_back(nclause->m_value);
		    delete nclause;
		    goto nextfield;
		} else if (match == string("filetype") || 
			   match == string("ext")) {
		    if (query->m_typeKind == WasaQuery::WQTK_NONE)
			query->m_typeKind = WasaQuery::WQTK_EXT;
		    if (query->m_typeKind == WasaQuery::WQTK_EXT)
			query->m_types.push_back(nclause->m_value);
		    delete nclause;
		    goto nextfield;
		} else if (match == string("sort")) {
		    if (nclause->m_value == "score") {
			query->m_sortSpec.push_back(WasaQuery::WQSK_REL);
		    } else if (nclause->m_value == "date") {
			query->m_sortSpec.push_back(WasaQuery::WQSK_DATE);
		    } else if (nclause->m_value == "alpha") {
			query->m_sortSpec.push_back(WasaQuery::WQSK_ALPHA);
		    } else if (nclause->m_value == "group") {
			query->m_sortSpec.push_back(WasaQuery::WQSK_GROUP);
		    }
		    delete nclause;
		    goto nextfield;
		} else {
		    nclause->m_fieldspec = match;
		}
	    }

	    // +- indicator ?
	    if (checkSubMatch(SMI_PM, match, reason) && match[0] == '-') {
		nclause->m_op = WasaQuery::OP_EXCL;
	    } else {
		nclause->m_op = WasaQuery::OP_LEAF;
	    }


	    if (prev_or) {
		// We're in an OR subquery, add new subquery
		orClause->m_subs.push_back(nclause);
	    } else {
		if (orClause) {
		    // Getting out of OR. Add the OR subquery to the main one
		    query->m_subs.push_back(orClause);
		    orClause = 0;
		}
		// Add new subquery to main one.
		query->m_subs.push_back(nclause);
	    }
	    prev_or = false;
	}

    nextfield:
	// Advance current string position. We checked earlier that
	// the increment is strictly positive, so we won't loop
	// forever
	m_cp += m_pmatch[0].rm_eo;
	if (m_cp >= cpe)
	    break;
    }

    regfree(&m_rx);
    m_rxneedsfree = false;
    return query;
}

#else // TEST

#include <stdio.h>
#include "wasastringtoquery.h"

static char *thisprog;

int main(int argc, char **argv)
{
    thisprog = argv[0];
    argc--; argv++;

    if (argc != 1) {
	fprintf(stderr, "need one arg\n");
	exit(1);
    }
    const string str = *argv++;argc--;
    string reason;
    StringToWasaQuery qparser;
    WasaQuery *q = qparser.stringToQuery(str, reason);
    if (q == 0) {
	fprintf(stderr, "stringToQuery failed: %s\n", reason.c_str());
	exit(1);
    }
    string desc;
    q->describe(desc);
    printf("%s\n", desc.c_str());
    exit(0);
}

#endif // TEST_STRINGTOQUERY
