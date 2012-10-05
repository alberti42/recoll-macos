/* Copyright (C) 2006 J.F.Dockes
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

// Handle translation from rcl's SearchData structures to Xapian Queries

#include "autoconfig.h"

#include <stdio.h>
#include <fnmatch.h>

#include <string>
#include <vector>
#include <algorithm>
using namespace std;

#include "xapian.h"

#include "cstr.h"
#include "rcldb.h"
#include "rcldb_p.h"
#include "searchdata.h"
#include "debuglog.h"
#include "smallut.h"
#include "textsplit.h"
#include "unacpp.h"
#include "utf8iter.h"
#include "stoplist.h"
#include "rclconfig.h"
#include "termproc.h"
#include "synfamily.h"
#include "stemdb.h"
#include "expansiondbs.h"

namespace Rcl {

typedef  vector<SearchDataClause *>::iterator qlist_it_t;
typedef  vector<SearchDataClause *>::const_iterator qlist_cit_t;

static const int original_term_wqf_booster = 10;

/* The dates-to-query routine is is lifted quasi-verbatim but
 *  modified from xapian-omega:date.cc. Copyright info:
 *
 * Copyright 1999,2000,2001 BrightStation PLC
 * Copyright 2001 James Aylett
 * Copyright 2001,2002 Ananova Ltd
 * Copyright 2002 Intercede 1749 Ltd
 * Copyright 2002,2003,2006 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#ifdef RCL_INDEX_STRIPCHARS
#define bufprefix(BUF, L) {(BUF)[0] = L;}
#define bpoffs() 1
#else
static inline void bufprefix(char *buf, char c)
{
    if (o_index_stripchars) {
	buf[0] = c;
    } else {
	buf[0] = ':'; 
	buf[1] = c; 
	buf[2] = ':';
    }
}
static inline int bpoffs() 
{
    return o_index_stripchars ? 1 : 3;
}
#endif

static Xapian::Query
date_range_filter(int y1, int m1, int d1, int y2, int m2, int d2)
{
    // Xapian uses a smallbuf and snprintf. Can't be bothered, we're
    // only doing %d's !
    char buf[200];
    bufprefix(buf, 'D');
    sprintf(buf+bpoffs(), "%04d%02d", y1, m1);
    vector<Xapian::Query> v;

    int d_last = monthdays(m1, y1);
    int d_end = d_last;
    if (y1 == y2 && m1 == m2 && d2 < d_last) {
	d_end = d2;
    }
    // Deal with any initial partial month
    if (d1 > 1 || d_end < d_last) {
    	for ( ; d1 <= d_end ; d1++) {
	    sprintf(buf + 6 + bpoffs(), "%02d", d1);
	    v.push_back(Xapian::Query(buf));
	}
    } else {
	bufprefix(buf, 'M');
	v.push_back(Xapian::Query(buf));
    }
    
    if (y1 == y2 && m1 == m2) {
	return Xapian::Query(Xapian::Query::OP_OR, v.begin(), v.end());
    }

    int m_last = (y1 < y2) ? 12 : m2 - 1;
    while (++m1 <= m_last) {
	sprintf(buf + 4 + bpoffs(), "%02d", m1);
	bufprefix(buf, 'M');
	v.push_back(Xapian::Query(buf));
    }
	
    if (y1 < y2) {
	while (++y1 < y2) {
	    sprintf(buf + bpoffs(), "%04d", y1);
	    bufprefix(buf, 'Y');
	    v.push_back(Xapian::Query(buf));
	}
	sprintf(buf + bpoffs(), "%04d", y2);
	bufprefix(buf, 'M');
	for (m1 = 1; m1 < m2; m1++) {
	    sprintf(buf + 4 + bpoffs(), "%02d", m1);
	    v.push_back(Xapian::Query(buf));
	}
    }
	
    sprintf(buf + 2 + bpoffs(), "%02d", m2);

    // Deal with any final partial month
    if (d2 < monthdays(m2, y2)) {
	bufprefix(buf, 'D');
    	for (d1 = 1 ; d1 <= d2; d1++) {
	    sprintf(buf + 6 + bpoffs(), "%02d", d1);
	    v.push_back(Xapian::Query(buf));
	}
    } else {
	bufprefix(buf, 'M');
	v.push_back(Xapian::Query(buf));
    }

    return Xapian::Query(Xapian::Query::OP_OR, v.begin(), v.end());
}

// Expand categories and mime type wild card exps
// Actually, using getAllMimeTypes() here is a bit problematic because
// there maybe other types in the index, not indexed by content, but
// which could be searched by file name. It would probably be
// preferable to do a termMatch() on field "mtype", which would
// retrieve all values from the index.
bool SearchData::expandFileTypes(RclConfig *cfg, vector<string>& tps)
{
    if (!cfg) {
	LOGFATAL(("Db::expandFileTypes: null configuration!!\n"));
	return false;
    }
    vector<string> exptps;
    vector<string> alltypes = cfg->getAllMimeTypes();

    for (vector<string>::iterator it = tps.begin(); it != tps.end(); it++) {
	if (cfg->isMimeCategory(*it)) {
	    vector<string>tps;
	    cfg->getMimeCatTypes(*it, tps);
	    exptps.insert(exptps.end(), tps.begin(), tps.end());
	} else {
	    for (vector<string>::const_iterator ait = alltypes.begin();
		 ait != alltypes.end(); ait++) {
		if (fnmatch(it->c_str(), ait->c_str(), FNM_CASEFOLD) 
		    != FNM_NOMATCH) {
		    exptps.push_back(*ait);
		}
	    }
	}
    }
    tps = exptps;
    return true;
}

bool SearchData::clausesToQuery(Rcl::Db &db, SClType tp, 
				vector<SearchDataClause*>& query, 
				string& reason, void *d, 
				int maxexp, int maxcl)
{
    Xapian::Query xq;
    for (qlist_it_t it = query.begin(); it != query.end(); it++) {
	Xapian::Query nq;
	if (!(*it)->toNativeQuery(db, &nq, maxexp, maxcl)) {
	    LOGERR(("SearchData::clausesToQuery: toNativeQuery failed: %s\n",
		    (*it)->getReason().c_str()));
	    reason += (*it)->getReason() + " ";
	    return false;
	}	    
        if (nq.empty()) {
            LOGDEB(("SearchData::clausesToQuery: skipping empty clause\n"));
            continue;
        }
	// If this structure is an AND list, must use AND_NOT for excl clauses.
	// Else this is an OR list, and there can't be excl clauses (checked by
	// addClause())
	Xapian::Query::op op;
	if (tp == SCLT_AND) {
            if ((*it)->m_tp == SCLT_EXCL) {
                op =  Xapian::Query::OP_AND_NOT;
            } else {
                op =  Xapian::Query::OP_AND;
            }
	} else {
	    op = Xapian::Query::OP_OR;
	}
        if (xq.empty()) {
            if (op == Xapian::Query::OP_AND_NOT)
                xq = Xapian::Query(op, Xapian::Query::MatchAll, nq);
            else 
                xq = nq;
        } else {
            xq = Xapian::Query(op, xq, nq);
        }
	if (int(xq.get_length()) >= maxcl) {
	    LOGERR(("Maximum Xapian query size exceeded."
		    " Maybe increase maxXapianClauses."));
	    m_reason += "Maximum Xapian query size exceeded."
		" Maybe increase maxXapianClauses.";
	    return false;
	}
    }
    if (xq.empty())
	xq = Xapian::Query::MatchAll;

   *((Xapian::Query *)d) = xq;
    return true;
}

bool SearchData::toNativeQuery(Rcl::Db &db, void *d, int maxexp, int maxcl)
{
    LOGDEB(("SearchData::toNativeQuery: stemlang [%s]\n", m_stemlang.c_str()));
    m_reason.erase();

    // Walk the clause list translating each in turn and building the 
    // Xapian query tree
    Xapian::Query xq;
    if (!clausesToQuery(db, m_tp, m_query, m_reason, &xq, maxexp, maxcl)) {
	LOGERR(("SearchData::toNativeQuery: clausesToQuery failed. reason: %s\n", 
		m_reason.c_str()));
	return false;
    }

    if (m_haveDates) {
        // If one of the extremities is unset, compute db extremas
        if (m_dates.y1 == 0 || m_dates.y2 == 0) {
            int minyear = 1970, maxyear = 2100;
            if (!db.maxYearSpan(&minyear, &maxyear)) {
                LOGERR(("Can't retrieve index min/max dates\n"));
                //whatever, go on.
            }

            if (m_dates.y1 == 0) {
                m_dates.y1 = minyear;
                m_dates.m1 = 1;
                m_dates.d1 = 1;
            }
            if (m_dates.y2 == 0) {
                m_dates.y2 = maxyear;
                m_dates.m2 = 12;
                m_dates.d2 = 31;
            }
        }
        LOGDEB(("Db::toNativeQuery: date interval: %d-%d-%d/%d-%d-%d\n",
                m_dates.y1, m_dates.m1, m_dates.d1,
                m_dates.y2, m_dates.m2, m_dates.d2));
        Xapian::Query dq = date_range_filter(m_dates.y1, m_dates.m1, m_dates.d1,
                m_dates.y2, m_dates.m2, m_dates.d2);
        if (dq.empty()) {
            LOGINFO(("Db::toNativeQuery: date filter is empty\n"));
        }
        // If no probabilistic query is provided then promote the daterange
        // filter to be THE query instead of filtering an empty query.
        if (xq.empty()) {
            LOGINFO(("Db::toNativeQuery: proba query is empty\n"));
            xq = dq;
        } else {
            xq = Xapian::Query(Xapian::Query::OP_FILTER, xq, dq);
        }
    }


    if (m_minSize != size_t(-1) || m_maxSize != size_t(-1)) {
        Xapian::Query sq;
	char min[50], max[50];
	sprintf(min, "%lld", (long long)m_minSize);
	sprintf(max, "%lld", (long long)m_maxSize);
	if (m_minSize == size_t(-1)) {
	    string value(max);
	    leftzeropad(value, 12);
	    sq = Xapian::Query(Xapian::Query::OP_VALUE_LE, VALUE_SIZE, value);
	} else if (m_maxSize == size_t(-1)) {
	    string value(min);
	    leftzeropad(value, 12);
	    sq = Xapian::Query(Xapian::Query::OP_VALUE_GE, VALUE_SIZE, value);
	} else {
	    string minvalue(min);
	    leftzeropad(minvalue, 12);
	    string maxvalue(max);
	    leftzeropad(maxvalue, 12);
	    sq = Xapian::Query(Xapian::Query::OP_VALUE_RANGE, VALUE_SIZE, 
			       minvalue, maxvalue);
	}
	    
        // If no probabilistic query is provided then promote the
        // filter to be THE query instead of filtering an empty query.
        if (xq.empty()) {
            LOGINFO(("Db::toNativeQuery: proba query is empty\n"));
            xq = sq;
        } else {
            xq = Xapian::Query(Xapian::Query::OP_FILTER, xq, sq);
        }
    }

    // Add the file type filtering clause if any
    if (!m_filetypes.empty()) {
	expandFileTypes(db.getConf(), m_filetypes);
	    
	Xapian::Query tq;
	for (vector<string>::iterator it = m_filetypes.begin(); 
	     it != m_filetypes.end(); it++) {
	    string term = "T" + *it;
	    LOGDEB0(("Adding file type term: [%s]\n", term.c_str()));
	    tq = tq.empty() ? Xapian::Query(term) : 
		Xapian::Query(Xapian::Query::OP_OR, tq, Xapian::Query(term));
	}
	xq = xq.empty() ? tq : Xapian::Query(Xapian::Query::OP_FILTER, xq, tq);
    }

    // Add the neg file type filtering clause if any
    if (!m_nfiletypes.empty()) {
	expandFileTypes(db.getConf(), m_nfiletypes);
	    
	Xapian::Query tq;
	for (vector<string>::iterator it = m_nfiletypes.begin(); 
	     it != m_nfiletypes.end(); it++) {
	    string term = "T" + *it;
	    LOGDEB0(("Adding negative file type term: [%s]\n", term.c_str()));
	    tq = tq.empty() ? Xapian::Query(term) : 
		Xapian::Query(Xapian::Query::OP_OR, tq, Xapian::Query(term));
	}
	xq = xq.empty() ? tq : Xapian::Query(Xapian::Query::OP_AND_NOT, xq, tq);
    }

    // Add the directory filtering clauses. Each is a phrase of terms
    // prefixed with the pathelt prefix XP
    for (vector<DirSpec>::const_iterator dit = m_dirspecs.begin();
	 dit != m_dirspecs.end(); dit++) {
	vector<string> vpath;
	stringToTokens(dit->dir, vpath, "/");
	vector<string> pvpath;
	if (dit->dir[0] == '/')
	    pvpath.push_back(wrap_prefix(pathelt_prefix));
	for (vector<string>::const_iterator pit = vpath.begin(); 
	     pit != vpath.end(); pit++){
	    pvpath.push_back(wrap_prefix(pathelt_prefix) + *pit);
	}
	Xapian::Query::op tdop;
	if (dit->weight == 1.0) {
	    tdop = dit->exclude ? 
		Xapian::Query::OP_AND_NOT : Xapian::Query::OP_FILTER;
	} else {
	    tdop = dit->exclude ? 
		Xapian::Query::OP_AND_NOT : Xapian::Query::OP_AND_MAYBE;
	}
	Xapian::Query tdq = Xapian::Query(Xapian::Query::OP_PHRASE, 
					  pvpath.begin(), pvpath.end());
	if (dit->weight != 1.0)
	    tdq = Xapian::Query(Xapian::Query::OP_SCALE_WEIGHT, 
				tdq, dit->weight);

	xq = Xapian::Query(tdop, xq, tdq);
    }

    *((Xapian::Query *)d) = xq;
    return true;
}

// This is called by the GUI simple search if the option is set: add
// (OR) phrase to a query (if it is simple enough) so that results
// where the search terms are close and in order will come up on top.
// We remove very common terms from the query to avoid performance issues.
bool SearchData::maybeAddAutoPhrase(Rcl::Db& db, double freqThreshold)
{
    LOGDEB0(("SearchData::maybeAddAutoPhrase()\n"));
    if (!m_query.size()) {
	LOGDEB2(("SearchData::maybeAddAutoPhrase: empty query\n"));
	return false;
    }

    string field;
    vector<string> words;
    // Walk the clause list. If we find any non simple clause or different
    // field names, bail out.
    for (qlist_it_t it = m_query.begin(); it != m_query.end(); it++) {
	SClType tp = (*it)->m_tp;
	if (tp != SCLT_AND && tp != SCLT_OR) {
	    LOGDEB2(("SearchData::maybeAddAutoPhrase: rejected clause\n"));
	    return false;
	}
	SearchDataClauseSimple *clp = 
	    dynamic_cast<SearchDataClauseSimple*>(*it);
	if (clp == 0) {
	    LOGDEB2(("SearchData::maybeAddAutoPhrase: dyncast failed\n"));
	    return false;
	}
	if (it == m_query.begin()) {
	    field = clp->getfield();
	} else {
	    if (clp->getfield().compare(field)) {
		LOGDEB2(("SearchData::maybeAddAutoPhrase: diff. fields\n"));
		return false;
	    }
	}

	// If there are wildcards or quotes in there, bail out
	if (clp->gettext().find_first_of("\"*[?") != string::npos) { 
	    LOGDEB2(("SearchData::maybeAddAutoPhrase: wildcards\n"));
	    return false;
	}
        // Do a simple word-split here, don't bother with the full-blown
	// textsplit. The autophrase thing is just "best effort", it's
	// normal that it won't work in strange cases.
	vector<string> wl;
	stringToStrings(clp->gettext(), wl);
	words.insert(words.end(), wl.begin(), wl.end());
    }


    // Trim the word list by eliminating very frequent terms
    // (increasing the slack as we do it):
    int slack = 0;
    int doccnt = db.docCnt();
    if (!doccnt)
	doccnt = 1;
    string swords;
    for (vector<string>::iterator it = words.begin(); 
	 it != words.end(); it++) {
	double freq = double(db.termDocCnt(*it)) / doccnt;
	if (freq < freqThreshold) {
	    if (!swords.empty())
		swords.append(1, ' ');
	    swords += *it;
	} else {
	    LOGDEB0(("Autophrase: [%s] too frequent (%.2f %%)\n", 
		    it->c_str(), 100 * freq));
	    slack++;
	}
    }
    
    // We can't make a phrase with a single word :)
    int nwords = TextSplit::countWords(swords);
    if (nwords <= 1) {
	LOGDEB2(("SearchData::maybeAddAutoPhrase: ended with 1 word\n"));
	return false;
    }

    // Increase the slack: we want to be a little more laxist than for
    // an actual user-entered phrase
    slack += 1 + nwords / 3;
    
    SearchDataClauseDist *nclp = 
	new SearchDataClauseDist(SCLT_PHRASE, swords, slack, field);

    // If the toplevel conjunction is an OR, just OR the phrase, else 
    // deepen the tree.
    if (m_tp == SCLT_OR) {
	addClause(nclp);
    } else {
	// My type is AND. Change it to OR and insert two queries, one
	// being the original query as a subquery, the other the
	// phrase.
	SearchData *sd = new SearchData(m_tp, m_stemlang);
	sd->m_query = m_query;
	sd->m_stemlang = m_stemlang;
	m_tp = SCLT_OR;
	m_query.clear();
	SearchDataClauseSub *oq = 
	    new SearchDataClauseSub(SCLT_OR, RefCntr<SearchData>(sd));
	addClause(oq);
	addClause(nclp);
    }
    return true;
}

// Add clause to current list. OR lists cant have EXCL clauses.
bool SearchData::addClause(SearchDataClause* cl)
{
    if (m_tp == SCLT_OR && (cl->m_tp == SCLT_EXCL)) {
	LOGERR(("SearchData::addClause: cant add EXCL to OR list\n"));
	m_reason = "No Negative (AND_NOT) clauses allowed in OR queries";
	return false;
    }
    cl->setParent(this);
    m_haveWildCards = m_haveWildCards || cl->m_haveWildCards;
    m_query.push_back(cl);
    return true;
}

// Make me all new
void SearchData::erase() {
    LOGDEB0(("SearchData::erase\n"));
    m_tp = SCLT_AND;
    for (qlist_it_t it = m_query.begin(); it != m_query.end(); it++)
	delete *it;
    m_query.clear();
    m_filetypes.clear();
    m_dirspecs.clear();
    m_description.erase();
    m_reason.erase();
    m_haveDates = false;
    m_minSize = size_t(-1);
    m_maxSize = size_t(-1);
}

// Am I a file name only search ? This is to turn off term highlighting
bool SearchData::fileNameOnly() 
{
    for (qlist_it_t it = m_query.begin(); it != m_query.end(); it++)
	if (!(*it)->isFileName())
	    return false;
    return true;
}

// Extract all term data
void SearchData::getTerms(HighlightData &hld) const
{
    for (qlist_cit_t it = m_query.begin(); it != m_query.end(); it++)
	(*it)->getTerms(hld);
    return;
}

// Splitter callback for breaking a user string into simple terms and
// phrases. This is for parts of the user entry which would appear as
// a single word because there is no white space inside, but are
// actually multiple terms to rcldb (ie term1,term2)
class TextSplitQ : public TextSplitP {
 public:
    TextSplitQ(Flags flags, const StopList &_stops, TermProc *prc)
	: TextSplitP(prc, flags), 
	  curnostemexp(false), stops(_stops), alltermcount(0), lastpos(0)
    {}

    bool takeword(const std::string &term, int pos, int bs, int be) 
    {
	// Check if the first letter is a majuscule in which
	// case we do not want to do stem expansion. Need to do this
	// before unac of course...
	curnostemexp = unaciscapital(term);

	return TextSplitP::takeword(term, pos, bs, be);
    }

    bool           curnostemexp;
    vector<string> terms;
    vector<bool>   nostemexps;
    const StopList &stops;
    // Count of terms including stopwords: this is for adjusting
    // phrase/near slack
    int alltermcount; 
    int lastpos;
};

class TermProcQ : public TermProc {
public:
    TermProcQ() : TermProc(0), m_ts(0) {}
    void setTSQ(TextSplitQ *ts) {m_ts = ts;}
    
    bool takeword(const std::string &term, int pos, int bs, int be) 
    {
	m_ts->alltermcount++;
	if (m_ts->lastpos < pos)
	    m_ts->lastpos = pos;
	bool noexpand = be ? m_ts->curnostemexp : true;
	LOGDEB(("TermProcQ::takeword: pushing [%s] pos %d noexp %d\n", 
		term.c_str(), pos, noexpand));
	if (m_terms[pos].size() < term.size()) {
	    m_terms[pos] = term;
	    m_nste[pos] = noexpand;
	}
	return true;
    }
    bool flush()
    {
	for (map<int, string>::const_iterator it = m_terms.begin();
	     it != m_terms.end(); it++) {
	    m_ts->terms.push_back(it->second);
	    m_ts->nostemexps.push_back(m_nste[it->first]);
	}
	return true;
    }
private:
    TextSplitQ *m_ts;
    map<int, string> m_terms;
    map<int, bool> m_nste;
};

// A class used to translate a user compound string (*not* a query
// language string) as may be entered in any_terms/all_terms search
// entry fields, ex: [term1 "a phrase" term3] into a xapian query
// tree.
// The object keeps track of the query terms and term groups while
// translating.
class StringToXapianQ {
public:
    StringToXapianQ(Db& db, HighlightData& hld, const string& field, 
		    const string &stmlng, bool boostUser, int maxexp, int maxcl)
	: m_db(db), m_field(field), m_stemlang(stmlng),
	  m_doBoostUserTerms(boostUser), m_hld(hld), m_autodiacsens(false),
	  m_autocasesens(true), m_maxexp(maxexp), m_maxcl(maxcl), m_curcl(0)
    { 
	m_db.getConf()->getConfParam("autodiacsens", &m_autodiacsens);
	m_db.getConf()->getConfParam("autocasesens", &m_autocasesens);
    }

    bool processUserString(const string &iq,
			   int mods, 
			   string &ermsg,
			   vector<Xapian::Query> &pqueries, 
			   int slack = 0, bool useNear = false);
private:
    bool expandTerm(string& ermsg, int mods, 
		    const string& term, vector<string>& exp, 
                    string& sterm, const string& prefix);
    // After splitting entry on whitespace: process non-phrase element
    void processSimpleSpan(string& ermsg, const string& span, 
			   int mods,
			   vector<Xapian::Query> &pqueries);
    // Process phrase/near element
    void processPhraseOrNear(string& ermsg, TextSplitQ *splitData, 
			     int mods,
			     vector<Xapian::Query> &pqueries,
			     bool useNear, int slack);

    Db&           m_db;
    const string& m_field;
    const string& m_stemlang;
    const bool    m_doBoostUserTerms;
    HighlightData& m_hld;
    bool m_autodiacsens;
    bool m_autocasesens;
    int  m_maxexp;
    int  m_maxcl;
    int  m_curcl;
};

#if 1
static void listVector(const string& what, const vector<string>&l)
{
    string a;
    for (vector<string>::const_iterator it = l.begin(); it != l.end(); it++) {
        a = a + *it + " ";
    }
    LOGDEB(("%s: %s\n", what.c_str(), a.c_str()));
}
#endif

/** Expand term into term list, using appropriate mode: stem, wildcards, 
 *  diacritics... 
 *
 * @param mods stem expansion, case and diacritics sensitivity control.
 * @param term input single word
 * @param exp output expansion list
 * @param sterm output original input term if there were no wildcards
 * @param prefix field prefix in index. We could recompute it, but the caller
 *  has it already. Used in the simple case where there is nothing to expand, 
 *  and we just return the prefixed term (else Db::termMatch deals with it).
 */
bool StringToXapianQ::expandTerm(string& ermsg, int mods, 
				 const string& term, 
                                 vector<string>& oexp, string &sterm,
				 const string& prefix)
{
    LOGDEB0(("expandTerm: mods 0x%x fld [%s] trm [%s] lang [%s]\n",
	     mods, m_field.c_str(), term.c_str(), m_stemlang.c_str()));
    sterm.clear();
    oexp.clear();
    if (term.empty())
	return true;

    bool haswild = term.find_first_of(cstr_minwilds) != string::npos;

    // If there are no wildcards, add term to the list of user-entered terms
    if (!haswild)
	m_hld.uterms.insert(term);

    bool nostemexp = (mods & SearchDataClause::SDCM_NOSTEMMING) != 0;

    // No stem expansion if there are wildcards or if prevented by caller
    if (haswild || m_stemlang.empty()) {
	LOGDEB2(("expandTerm: found wildcards or stemlang empty: no exp\n"));
	nostemexp = true;
    }

    bool noexpansion = nostemexp && !haswild;

#ifndef RCL_INDEX_STRIPCHARS
    bool diac_sensitive = (mods & SearchDataClause::SDCM_DIACSENS) != 0;
    bool case_sensitive = (mods & SearchDataClause::SDCM_CASESENS) != 0;

    if (o_index_stripchars) {
	diac_sensitive = case_sensitive = false;
    } else {
	// If we are working with a raw index, apply the rules for case and 
	// diacritics sensitivity.

	// If any character has a diacritic, we become
	// diacritic-sensitive. Note that the way that the test is
	// performed (conversion+comparison) will automatically ignore
	// accented characters which are actually a separate letter
	if (m_autodiacsens && unachasaccents(term)) {
	    LOGDEB0(("expandTerm: term has accents -> diac-sensitive\n"));
	    diac_sensitive = true;
	}

	// If any character apart the first is uppercase, we become
	// case-sensitive.  The first character is reserved for
	// turning off stemming. You need to use a query language
	// modifier to search for Floor in a case-sensitive way.
	Utf8Iter it(term);
	it++;
	if (m_autocasesens && unachasuppercase(term.substr(it.getBpos()))) {
	    LOGDEB0(("expandTerm: term has uppercase -> case-sensitive\n"));
	    case_sensitive = true;
	}

	// If we are sensitive to case or diacritics turn stemming off
	if (diac_sensitive || case_sensitive) {
	    LOGDEB0(("expandTerm: diac or case sens set -> stemexpand off\n"));
	    nostemexp = true;
	}

	if (!case_sensitive || !diac_sensitive)
	    noexpansion = false;
    }
#endif

    if (noexpansion) {
	sterm = term;
	oexp.push_back(prefix + term);
	m_hld.terms[term] = m_hld.uterms.size() - 1;
	LOGDEB(("ExpandTerm: final: %s\n", stringsToString(oexp).c_str()));
	return true;
    } 

    // Make objects before the goto jungle to avoid compiler complaints
    SynTermTransUnac unacfoldtrans(UNACOP_UNACFOLD);
    XapComputableSynFamMember synac(m_db.m_ndb->xrdb, synFamDiCa, "all", 
				    &unacfoldtrans);
    // This will hold the result of case and diacritics expansion as input
    // to stem expansion.
    vector<string> lexp;
    
    TermMatchResult res;
    if (haswild) {
	// Note that if there are wildcards, we do a direct from-index
	// expansion, which means that we are casediac-sensitive. There
	// would be nothing to prevent us to expand from the casediac
	// synonyms first. To be done later
	m_db.termMatch(Rcl::Db::ET_WILD, m_stemlang,term,res,m_maxexp,m_field);
	goto termmatchtoresult;
    }

    sterm = term;

#ifdef RCL_INDEX_STRIPCHARS

    m_db.termMatch(Rcl::Db::ET_STEM, m_stemlang, term, res, m_maxexp, m_field);

#else

    if (o_index_stripchars) {
	// If the index is raw, we can only come here if nostemexp is unset
	// and we just need stem expansion.
	m_db.termMatch(Rcl::Db::ET_STEM, m_stemlang,term,res,m_maxexp,m_field);
	goto termmatchtoresult;
    } 

    // No stem expansion when diacritic or case sensitivity is set, it
    // makes no sense (it would mess with the diacritics anyway if
    // they are not in the stem part).  In these 3 cases, perform
    // appropriate expansion from the charstripping db, and do a bogus
    // wildcard expansion (there is no wild card) to generate the
    // result:

    if (diac_sensitive && case_sensitive) {
	// No expansion whatsoever. 
	lexp.push_back(term);
	goto exptotermatch;
    } else if (diac_sensitive) {
	// Expand for accents and case, filtering for same accents,
	SynTermTransUnac foldtrans(UNACOP_FOLD);
	synac.synExpand(term, lexp, &foldtrans);
	goto exptotermatch;
    } else if (case_sensitive) {
	// Expand for accents and case, filtering for same case
	SynTermTransUnac unactrans(UNACOP_UNAC);
	synac.synExpand(term, lexp, &unactrans);
	goto exptotermatch;
    } else {
	// We are neither accent- nor case- sensitive and may need stem
	// expansion or not. Expand for accents and case
	synac.synExpand(term, lexp);
	if (nostemexp)
	    goto exptotermatch;
    }

    // Need stem expansion. Lowercase the result of accent and case
    // expansion for input to stemdb.
    for (unsigned int i = 0; i < lexp.size(); i++) {
	string lower;
	unacmaybefold(lexp[i], lower, "UTF-8", UNACOP_FOLD);
	lexp[i] = lower;
    }
    sort(lexp.begin(), lexp.end());
    {
	vector<string>::iterator uit = unique(lexp.begin(), lexp.end());
	lexp.resize(uit - lexp.begin());
	StemDb db(m_db.m_ndb->xrdb);
	vector<string> exp1;
	for (vector<string>::const_iterator it = lexp.begin(); 
	     it != lexp.end(); it++) {
	    db.stemExpand(m_stemlang, *it, exp1);
	}
	LOGDEB(("ExpTerm: stem exp-> %s\n", stringsToString(exp1).c_str()));

	// Expand the resulting list for case (all stemdb content
	// is lowercase)
	lexp.clear();
	for (vector<string>::const_iterator it = exp1.begin(); 
	     it != exp1.end(); it++) {
	    synac.synExpand(*it, lexp);
	}
	sort(lexp.begin(), lexp.end());
	uit = unique(lexp.begin(), lexp.end());
	lexp.resize(uit - lexp.begin());
    }

    // Bogus wildcard expand to generate the result (possibly add prefixes)
exptotermatch:
    LOGDEB(("ExpandTerm:TM: lexp: %s\n", stringsToString(lexp).c_str()));
    for (vector<string>::const_iterator it = lexp.begin();
	 it != lexp.end(); it++) {
	m_db.termMatch(Rcl::Db::ET_WILD, m_stemlang, *it, res,m_maxexp,m_field);
    }
#endif

    // Term match entries to vector of terms
termmatchtoresult:
    if (int(res.entries.size()) >= m_maxexp) {
	ermsg = "Maximum term expansion size exceeded."
	    " Maybe increase maxTermExpand.";
	return false;
    }
    for (vector<TermMatchEntry>::const_iterator it = res.entries.begin(); 
	 it != res.entries.end(); it++) {
	oexp.push_back(it->term);
    }
    // If the term does not exist at all in the db, the return from
    // term match is going to be empty, which is not what we want (we
    // would then compute an empty Xapian query)
    if (oexp.empty())
	oexp.push_back(prefix + term);

    // Remember the uterm-to-expansion links
    for (vector<string>::const_iterator it = oexp.begin(); 
	 it != oexp.end(); it++) {
	m_hld.terms[strip_prefix(*it)] = term;
    }
    LOGDEB(("ExpandTerm: final: %s\n", stringsToString(oexp).c_str()));
    return true;
}

// Do distribution of string vectors: a,b c,d -> a,c a,d b,c b,d
void multiply_groups(vector<vector<string> >::const_iterator vvit,
		     vector<vector<string> >::const_iterator vvend, 
		     vector<string>& comb,
		     vector<vector<string> >&allcombs)
{
    // Remember my string vector and compute next, for recursive calls.
    vector<vector<string> >::const_iterator myvit = vvit++;

    // Walk the string vector I'm called upon and, for each string,
    // add it to current result, an call myself recursively on the
    // next string vector. The last call (last element of the vector of
    // vectors), adds the elementary result to the output

    // Walk my string vector
    for (vector<string>::const_iterator strit = (*myvit).begin();
	 strit != (*myvit).end(); strit++) {

	// Add my current value to the string vector we're building
	comb.push_back(*strit);

	if (vvit == vvend) {
	    // Last call: store current result
	    allcombs.push_back(comb);
	} else {
	    // Call recursively on next string vector
	    multiply_groups(vvit, vvend, comb, allcombs);
	}
	// Pop the value I just added (make room for the next element in my
	// vector)
	comb.pop_back();
    }
}

void StringToXapianQ::processSimpleSpan(string& ermsg, const string& span, 
					int mods,
					vector<Xapian::Query> &pqueries)
{
    LOGDEB0(("StringToXapianQ::processSimpleSpan: [%s] mods 0x%x\n",
	    span.c_str(), (unsigned int)mods));
    vector<string> exp;  
    string sterm; // dumb version of user term

    string prefix;
    const FieldTraits *ftp;
    if (!m_field.empty() && m_db.fieldToTraits(m_field, &ftp)) {
	prefix = wrap_prefix(ftp->pfx);
    }

    if (!expandTerm(ermsg, mods, span, exp, sterm, prefix))
	return;
    
    // Set up the highlight data. No prefix should go in there
    for (vector<string>::const_iterator it = exp.begin(); 
	 it != exp.end(); it++) {
	m_hld.groups.push_back(vector<string>(1, it->substr(prefix.size())));
	m_hld.slacks.push_back(0);
	m_hld.grpsugidx.push_back(m_hld.ugroups.size() - 1);
    }

    // Push either term or OR of stem-expanded set
    Xapian::Query xq(Xapian::Query::OP_OR, exp.begin(), exp.end());
    m_curcl += exp.size();

    // If sterm (simplified original user term) is not null, give it a
    // relevance boost. We do this even if no expansion occurred (else
    // the non-expanded terms in a term list would end-up with even
    // less wqf). This does not happen if there are wildcards anywhere
    // in the search.
    if (m_doBoostUserTerms && !sterm.empty()) {
        xq = Xapian::Query(Xapian::Query::OP_OR, xq, 
			   Xapian::Query(prefix+sterm, 
					 original_term_wqf_booster));
    }
    pqueries.push_back(xq);
}

// User entry element had several terms: transform into a PHRASE or
// NEAR xapian query, the elements of which can themselves be OR
// queries if the terms get expanded by stemming or wildcards (we
// don't do stemming for PHRASE though)
void StringToXapianQ::processPhraseOrNear(string& ermsg, TextSplitQ *splitData, 
					  int mods,
					  vector<Xapian::Query> &pqueries,
					  bool useNear, int slack)
{
    Xapian::Query::op op = useNear ? Xapian::Query::OP_NEAR : 
	Xapian::Query::OP_PHRASE;
    vector<Xapian::Query> orqueries;
#ifdef XAPIAN_NEAR_EXPAND_SINGLE_BUF
    bool hadmultiple = false;
#endif
    vector<vector<string> >groups;

    string prefix;
    const FieldTraits *ftp;
    if (!m_field.empty() && m_db.fieldToTraits(m_field, &ftp)) {
	prefix = wrap_prefix(ftp->pfx);
    }

    if (mods & Rcl::SearchDataClause::SDCM_ANCHORSTART) {
	orqueries.push_back(Xapian::Query(prefix + start_of_field_term));
	slack++;
    }

    // Go through the list and perform stem/wildcard expansion for each element
    vector<bool>::iterator nxit = splitData->nostemexps.begin();
    for (vector<string>::iterator it = splitData->terms.begin();
	 it != splitData->terms.end(); it++, nxit++) {
	LOGDEB0(("ProcessPhrase: processing [%s]\n", it->c_str()));
	// Adjust when we do stem expansion. Not if disabled by
	// caller, not inside phrases, and some versions of xapian
	// will accept only one OR clause inside NEAR.
	bool nostemexp = *nxit || (op == Xapian::Query::OP_PHRASE) 
#ifdef XAPIAN_NEAR_EXPAND_SINGLE_BUF
	    || hadmultiple
#endif // single OR inside NEAR
	    ;
	int lmods = mods;
	if (nostemexp)
	    lmods |= SearchDataClause::SDCM_NOSTEMMING;
	string sterm;
	vector<string> exp;
	if (!expandTerm(ermsg, lmods, *it, exp, sterm, prefix))
	    return;
	LOGDEB0(("ProcessPhraseOrNear: exp size %d\n", exp.size()));
	listVector("", exp);
	// groups is used for highlighting, we don't want prefixes in there.
	vector<string> noprefs;
	for (vector<string>::const_iterator it = exp.begin(); 
	     it != exp.end(); it++) {
	    noprefs.push_back(it->substr(prefix.size()));
	}
	groups.push_back(noprefs);
	orqueries.push_back(Xapian::Query(Xapian::Query::OP_OR, 
					  exp.begin(), exp.end()));
	m_curcl += exp.size();
	if (m_curcl >= m_maxcl)
	    return;
#ifdef XAPIAN_NEAR_EXPAND_SINGLE_BUF
	if (exp.size() > 1) 
	    hadmultiple = true;
#endif
    }

    if (mods & Rcl::SearchDataClause::SDCM_ANCHOREND) {
	orqueries.push_back(Xapian::Query(prefix + end_of_field_term));
	slack++;
    }

    // Generate an appropriate PHRASE/NEAR query with adjusted slack
    // For phrases, give a relevance boost like we do for original terms
    LOGDEB2(("PHRASE/NEAR:  alltermcount %d lastpos %d\n", 
             splitData->alltermcount, splitData->lastpos));
    Xapian::Query xq(op, orqueries.begin(), orqueries.end(),
		     splitData->lastpos + 1 + slack);
    if (op == Xapian::Query::OP_PHRASE)
	xq = Xapian::Query(Xapian::Query::OP_SCALE_WEIGHT, xq, 
			   original_term_wqf_booster);
    pqueries.push_back(xq);

    // Add all combinations of NEAR/PHRASE groups to the highlighting data. 
    vector<vector<string> > allcombs;
    vector<string> comb;
    multiply_groups(groups.begin(), groups.end(), comb, allcombs);
    
    // Insert the search groups and slacks in the highlight data, with
    // a reference to the user entry that generated them:
    m_hld.groups.insert(m_hld.groups.end(), allcombs.begin(), allcombs.end());
    m_hld.slacks.insert(m_hld.slacks.end(), allcombs.size(), slack);
    m_hld.grpsugidx.insert(m_hld.grpsugidx.end(), allcombs.size(), 
			   m_hld.ugroups.size() - 1);
}

// Trim string beginning with ^ or ending with $ and convert to flags
static int stringToMods(string& s)
{
    int mods = 0;
    // Check for an anchored search
    trimstring(s);
    if (s.length() > 0 && s[0] == '^') {
	mods |= Rcl::SearchDataClause::SDCM_ANCHORSTART;
	s.erase(0, 1);
    }
    if (s.length() > 0 && s[s.length()-1] == '$') {
	mods |= Rcl::SearchDataClause::SDCM_ANCHOREND;
	s.erase(s.length()-1);
    }
    return mods;
}

/** 
 * Turn user entry string (NOT query language) into a list of xapian queries.
 * We just separate words and phrases, and do wildcard and stem expansion,
 *
 * This is used to process data entered into an OR/AND/NEAR/PHRASE field of
 * the GUI (in the case of NEAR/PHRASE, clausedist adds dquotes to the user
 * entry).
 *
 * This appears awful, and it would seem that the split into
 * terms/phrases should be performed in the upper layer so that we
 * only receive pure term or near/phrase pure elements here, but in
 * fact there are things that would appear like terms to naive code,
 * and which will actually may be turned into phrases (ie: tom:jerry),
 * in a manner which intimately depends on the index implementation,
 * so that it makes sense to process this here.
 *
 * The final list contains one query for each term or phrase
 *   - Elements corresponding to a stem-expanded part are an OP_OR
 *     composition of the stem-expanded terms (or a single term query).
 *   - Elements corresponding to phrase/near are an OP_PHRASE/NEAR
 *     composition of the phrase terms (no stem expansion in this case)
 * @return the subquery count (either or'd stem-expanded terms or phrase word
 *   count)
 */
bool StringToXapianQ::processUserString(const string &iq,
					int mods, 
					string &ermsg,
					vector<Xapian::Query> &pqueries,
					int slack, 
					bool useNear
					)
{
    LOGDEB(("StringToXapianQ:pUS:: qstr [%s] fld [%s] mods 0x%x "
	    "slack %d near %d\n", 
	    iq.c_str(), m_field.c_str(), mods, slack, useNear));
    ermsg.erase();
    m_curcl = 0;
    const StopList stops = m_db.getStopList();

    // Simple whitespace-split input into user-level words and
    // double-quoted phrases: word1 word2 "this is a phrase". 
    //
    // The text splitter may further still decide that the resulting
    // "words" are really phrases, this depends on separators:
    // [paul@dom.net] would still be a word (span), but [about:me]
    // will probably be handled as a phrase.
    vector<string> phrases;
    TextSplit::stringToStrings(iq, phrases);

    // Process each element: textsplit into terms, handle stem/wildcard 
    // expansion and transform into an appropriate Xapian::Query
    try {
	for (vector<string>::iterator it = phrases.begin(); 
	     it != phrases.end(); it++) {
	    LOGDEB0(("strToXapianQ: phrase/word: [%s]\n", it->c_str()));
	    // Anchoring modifiers
	    int amods = stringToMods(*it);
	    int terminc = amods != 0 ? 1 : 0;
	    mods |= amods;
	    // If there are multiple spans in this element, including
	    // at least one composite, we have to increase the slack
	    // else a phrase query including a span would fail. 
	    // Ex: "term0@term1 term2" is onlyspans-split as:
	    //   0 term0@term1             0   12
	    //   2 term2                  13   18
	    // The position of term2 is 2, not 1, so a phrase search
	    // would fail.
	    // We used to do  word split, searching for 
	    // "term0 term1 term2" instead, which may have worse 
	    // performance, but will succeed.
	    // We now adjust the phrase/near slack by comparing the term count
	    // and the last position

	    // The term processing pipeline:
	    TermProcQ tpq;
	    TermProc *nxt = &tpq;
            TermProcStop tpstop(nxt, stops); nxt = &tpstop;
            //TermProcCommongrams tpcommon(nxt, stops); nxt = &tpcommon;
            //tpcommon.onlygrams(true);
	    TermProcPrep tpprep(nxt);
#ifndef RCL_INDEX_STRIPCHARS
	    if (o_index_stripchars)
#endif
		nxt = &tpprep;

	    TextSplitQ splitter(TextSplit::Flags(TextSplit::TXTS_ONLYSPANS | 
						 TextSplit::TXTS_KEEPWILD), 
				stops, nxt);
	    tpq.setTSQ(&splitter);
	    splitter.text_to_words(*it);

	    slack += splitter.lastpos - splitter.terms.size() + 1;

	    LOGDEB0(("strToXapianQ: termcount: %d\n", splitter.terms.size()));
	    switch (splitter.terms.size() + terminc) {
	    case 0: 
		continue;// ??
	    case 1: {
		int lmods = mods;
		if (splitter.nostemexps.front())
		    lmods |= SearchDataClause::SDCM_NOSTEMMING;
		m_hld.ugroups.push_back(vector<string>(1, *it));
		processSimpleSpan(ermsg,splitter.terms.front(),lmods, pqueries);
	    }
		break;
	    default:
		m_hld.ugroups.push_back(vector<string>(1, *it));
		processPhraseOrNear(ermsg, &splitter, mods, pqueries,
				    useNear, slack);
	    }
	    if (m_curcl >= m_maxcl) {
		ermsg = "Maximum Xapian query size exceeded."
		    " Maybe increase maxXapianClauses.";
		break;
	    }
	}
    } catch (const Xapian::Error &e) {
	ermsg = e.get_msg();
    } catch (const string &s) {
	ermsg = s;
    } catch (const char *s) {
	ermsg = s;
    } catch (...) {
	ermsg = "Caught unknown exception";
    }
    if (!ermsg.empty()) {
	LOGERR(("stringToXapianQueries: %s\n", ermsg.c_str()));
	return false;
    }
    return true;
}

// Translate a simple OR, AND, or EXCL search clause. 
bool SearchDataClauseSimple::toNativeQuery(Rcl::Db &db, void *p, 
					   int maxexp, int maxcl)
{
    LOGDEB2(("SearchDataClauseSimple::toNativeQuery: stemlang [%s]\n",
	     getStemLang().c_str()));

    Xapian::Query *qp = (Xapian::Query *)p;
    *qp = Xapian::Query();

    Xapian::Query::op op;
    switch (m_tp) {
    case SCLT_AND: op = Xapian::Query::OP_AND; break;
	// EXCL will be set with AND_NOT in the list. So it's an OR list here
    case SCLT_OR: 
    case SCLT_EXCL: op = Xapian::Query::OP_OR; break;
    default:
	LOGERR(("SearchDataClauseSimple: bad m_tp %d\n", m_tp));
	return false;
    }
    vector<Xapian::Query> pqueries;

    // We normally boost the original term in the stem expansion list. Don't
    // do it if there are wildcards anywhere, this would skew the results.
    bool doBoostUserTerm = 
	(m_parentSearch && !m_parentSearch->haveWildCards()) || 
	(m_parentSearch == 0 && !m_haveWildCards);

    StringToXapianQ tr(db, m_hldata, m_field, getStemLang(), doBoostUserTerm,
		       maxexp, maxcl);
    if (!tr.processUserString(m_text, getModifiers(), m_reason, pqueries))
	return false;
    if (pqueries.empty()) {
	LOGERR(("SearchDataClauseSimple: resolved to null query\n"));
	return true;
    }

    *qp = Xapian::Query(op, pqueries.begin(), pqueries.end());
    if  (m_weight != 1.0) {
	*qp = Xapian::Query(Xapian::Query::OP_SCALE_WEIGHT, *qp, m_weight);
    }
    return true;
}

// Translate a FILENAME search clause. This always comes
// from a "filename" search from the gui or recollq. A query language
// "filename:"-prefixed field will not go through here, but through
// the generic field-processing code.
//
// We do not split the entry any more (used to do some crazy thing
// about expanding multiple fragments in the past. We just take the
// value blanks and all and expand this against the indexed unsplit
// file names
bool SearchDataClauseFilename::toNativeQuery(Rcl::Db &db, void *p, 
					     int maxexp, int)
{
    Xapian::Query *qp = (Xapian::Query *)p;
    *qp = Xapian::Query();

    vector<string> names;
    db.filenameWildExp(m_text, names, maxexp);
    *qp = Xapian::Query(Xapian::Query::OP_OR, names.begin(), names.end());

    if (m_weight != 1.0) {
	*qp = Xapian::Query(Xapian::Query::OP_SCALE_WEIGHT, *qp, m_weight);
    }
    return true;
}

// Translate NEAR or PHRASE clause. 
bool SearchDataClauseDist::toNativeQuery(Rcl::Db &db, void *p, 
					 int maxexp, int maxcl)
{
    LOGDEB(("SearchDataClauseDist::toNativeQuery\n"));

    Xapian::Query *qp = (Xapian::Query *)p;
    *qp = Xapian::Query();

    vector<Xapian::Query> pqueries;
    Xapian::Query nq;

    // We normally boost the original term in the stem expansion list. Don't
    // do it if there are wildcards anywhere, this would skew the results.
    bool doBoostUserTerm = 
	(m_parentSearch && !m_parentSearch->haveWildCards()) || 
	(m_parentSearch == 0 && !m_haveWildCards);

    // We produce a single phrase out of the user entry then use
    // stringToXapianQueries() to lowercase and simplify the phrase
    // terms etc. This will result into a single (complex)
    // Xapian::Query.
    if (m_text.find('\"') != string::npos) {
	m_text = neutchars(m_text, "\"");
    }
    string s = cstr_dquote + m_text + cstr_dquote;
    bool useNear = (m_tp == SCLT_NEAR);
    StringToXapianQ tr(db, m_hldata, m_field, getStemLang(), doBoostUserTerm,
		       maxexp, maxcl);
    if (!tr.processUserString(s, getModifiers(), m_reason, pqueries, 
			      m_slack, useNear))
	return false;
    if (pqueries.empty()) {
	LOGERR(("SearchDataClauseDist: resolved to null query\n"));
	return true;
    }

    *qp = *pqueries.begin();
    if (m_weight != 1.0) {
	*qp = Xapian::Query(Xapian::Query::OP_SCALE_WEIGHT, *qp, m_weight);
    }
    return true;
}

} // Namespace Rcl
