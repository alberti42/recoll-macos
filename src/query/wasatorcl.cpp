#ifndef lint
static char rcsid[] = "@(#$Id: wasatorcl.cpp,v 1.10 2007-10-05 14:00:04 dockes Exp $ (C) 2006 J.F.Dockes";
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
#include <string>
#include <list>
using std::string;
using std::list;

#include "wasastringtoquery.h"
#include "rcldb.h"
#include "searchdata.h"
#include "wasatorcl.h"
#include "debuglog.h"
#include "smallut.h"
#include "rclconfig.h"

Rcl::SearchData *wasaStringToRcl(const string &qs, string &reason)
{
    StringToWasaQuery parser;
    WasaQuery *wq = parser.stringToQuery(qs, reason);
    if (wq == 0) 
	return 0;
    Rcl::SearchData *rq = wasaQueryToRcl(wq);
    if (rq == 0) {
	reason = "Failed translating wasa query structure to recoll";
	return 0;
    }
    return rq;
}

Rcl::SearchData *wasaQueryToRcl(WasaQuery *wasa)
{
    if (wasa == 0)
	return 0;

    Rcl::SearchData *sdata = new Rcl::SearchData(Rcl::SCLT_AND);

    WasaQuery::subqlist_t::iterator it;
    Rcl::SearchDataClause *nclause;

    for (it = wasa->m_subs.begin(); it != wasa->m_subs.end(); it++) {
	switch ((*it)->m_op) {
	case WasaQuery::OP_NULL:
	case WasaQuery::OP_AND:
	default:
	    // ??
	    continue;
	case WasaQuery::OP_LEAF:

	    // Special cases for mime and category. Not pretty.
	    if (!stringicmp("mime", (*it)->m_fieldspec)) {
		sdata->addFiletype((*it)->m_value);
		break;
	    } 
	    if (!stringicmp("rclcat", (*it)->m_fieldspec)) {
		RclConfig *conf = RclConfig::getMainConfig();
		list<string> mtypes;
		if (conf && conf->getMimeCatTypes((*it)->m_value, mtypes)) {
		    for (list<string>::iterator mit = mtypes.begin();
			 mit != mtypes.end(); mit++) {
			sdata->addFiletype(*mit);
		    }
		}
		break;
	    } 

	    if ((*it)->m_value.find_first_of(" \t\n\r") != string::npos) {
		nclause = new Rcl::SearchDataClauseDist(Rcl::SCLT_PHRASE, 
							(*it)->m_value, 0, 
							(*it)->m_fieldspec);
	    } else {
		nclause = new Rcl::SearchDataClauseSimple(Rcl::SCLT_AND, 
							  (*it)->m_value, 
							  (*it)->m_fieldspec);
	    }
	    if (nclause == 0) {
		LOGERR(("wasaQueryToRcl: out of memory\n"));
		return 0;
	    }
	    if ((*it)->m_modifiers & WasaQuery::WQM_NOSTEM) {
		fprintf(stderr, "Setting NOSTEM\n");
		nclause->setModifiers(Rcl::SearchDataClause::SDCM_NOSTEMMING);
	    }
	    sdata->addClause(nclause);
	    break;
	case WasaQuery::OP_EXCL:
	    // Note: have to add dquotes which will be translated to
	    // phrase if there are several words in there. Not pretty
	    // but should work. If there is actually a single
	    // word, it will not be taken as a phrase, and
	    // stem-expansion will work normally
	    nclause = new Rcl::SearchDataClauseSimple(Rcl::SCLT_EXCL, 
						      string("\"") + 
						      (*it)->m_value + "\"",
						      (*it)->m_fieldspec);
	    
	    if (nclause == 0) {
		LOGERR(("wasaQueryToRcl: out of memory\n"));
		return 0;
	    }
	    if ((*it)->m_modifiers & WasaQuery::WQM_NOSTEM)
		nclause->setModifiers(Rcl::SearchDataClause::SDCM_NOSTEMMING);
	    sdata->addClause(nclause);
	    break;
	case WasaQuery::OP_OR:
	    // Concatenate all OR values as phrases. Hope there are no
	    // stray dquotes in there. Note that single terms won't be
	    // handled as real phrases inside searchdata.cpp (so the
	    // added dquotes won't interfer with stemming)
	    {
		string orvalue;
		WasaQuery::subqlist_t::iterator orit;
		for (orit = (*it)->m_subs.begin(); 
		     orit != (*it)->m_subs.end(); orit++) {
		    orvalue += string("\"") + (*orit)->m_value + "\" ";
		}
		nclause = new Rcl::SearchDataClauseSimple(Rcl::SCLT_OR, 
							  orvalue,
							  (*it)->m_fieldspec);
		if (nclause == 0) {
		    LOGERR(("wasaQueryToRcl: out of memory\n"));
		    return 0;
		}
		if ((*it)->m_modifiers & WasaQuery::WQM_NOSTEM)
		    nclause->setModifiers(Rcl::SearchDataClause::SDCM_NOSTEMMING);
		sdata->addClause(nclause);
	    }
	}
    }

    return sdata;
}
