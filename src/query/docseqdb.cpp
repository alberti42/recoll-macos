/* Copyright (C) 2005 J.F.Dockes
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
#include <math.h>
#include <time.h>

#include "docseqdb.h"
#include "rcldb.h"
#include "debuglog.h"
#include "internfile.h"
#include "wasatorcl.h"

DocSequenceDb::DocSequenceDb(RefCntr<Rcl::Query> q, const string &t, 
			     RefCntr<Rcl::SearchData> sdata) 
    : DocSequence(t), m_q(q), m_sdata(sdata), m_fsdata(sdata),
      m_rescnt(-1), 
      m_queryBuildAbstract(true),
      m_queryReplaceAbstract(false),
      m_isFiltered(false),
      m_isSorted(false),
      m_needSetQuery(false)
{
}

DocSequenceDb::~DocSequenceDb() 
{
}

bool DocSequenceDb::getTerms(vector<string>& terms, 
			     vector<vector<string> >& groups, 
			     vector<int>& gslks)
{
    return m_fsdata->getTerms(terms, groups, gslks);
}

void DocSequenceDb::getUTerms(vector<string>& terms)
{
    m_sdata->getUTerms(terms);
}

string DocSequenceDb::getDescription() 
{
    return m_fsdata->getDescription();
}

bool DocSequenceDb::getDoc(int num, Rcl::Doc &doc, string *sh)
{
    setQuery();
    if (sh) sh->erase();
    return m_q->getDoc(num, doc);
}

int DocSequenceDb::getResCnt()
{
    setQuery();
    if (m_rescnt < 0) {
	m_rescnt= m_q->getResCnt();
    }
    return m_rescnt;
}

bool DocSequenceDb::getAbstract(Rcl::Doc &doc, vector<string>& vabs)
{
    setQuery();
    if (m_q->whatDb() &&
	m_queryBuildAbstract && (doc.syntabs || m_queryReplaceAbstract)) {
	m_q->whatDb()->makeDocAbstract(doc, m_q.getptr(), vabs);
    } 
    if (vabs.empty())
	vabs.push_back(doc.meta[Rcl::Doc::keyabs]);

    return true;
}

bool DocSequenceDb::getEnclosing(Rcl::Doc& doc, Rcl::Doc& pdoc) 
{
    setQuery();
    string udi;
    if (!FileInterner::getEnclosing(doc.url, doc.ipath, pdoc.url, pdoc.ipath,
                                    udi))
        return false;
    return m_q->whatDb()->getDoc(udi, pdoc);
}

list<string> DocSequenceDb::expand(Rcl::Doc &doc)
{
    setQuery();
    vector<string> v = m_q->expand(doc);
    return list<string>(v.begin(), v.end());
}

string DocSequenceDb::title()
{
    string qual;
    if (m_isFiltered && !m_isSorted)
	qual = string(" (") + o_filt_trans + string(")");
    else if (!m_isFiltered && m_isSorted)
	qual = string(" (") + o_sort_trans + string(")");
    else if (m_isFiltered && m_isSorted)
	qual = string(" (") + o_sort_trans + string(",") + o_filt_trans + string(")");
    return DocSequence::title() + qual;
}

bool DocSequenceDb::setFiltSpec(const DocSeqFiltSpec &fs) 
{
    LOGDEB(("DocSequenceDb::setFiltSpec\n"));
    if (fs.isNotNull()) {
	// We build a search spec by adding a filtering layer to the base one.
	m_fsdata = RefCntr<Rcl::SearchData>(new Rcl::SearchData(Rcl::SCLT_AND));
	Rcl::SearchDataClauseSub *cl = 
	    new Rcl::SearchDataClauseSub(Rcl::SCLT_SUB, m_sdata);
	m_fsdata->addClause(cl);
    
	for (unsigned int i = 0; i < fs.crits.size(); i++) {
	    switch (fs.crits[i]) {
	    case DocSeqFiltSpec::DSFS_MIMETYPE:
		m_fsdata->addFiletype(fs.values[i]);
		break;
	    case DocSeqFiltSpec::DSFS_QLANG:
	    {
		if (m_q.isNull())
		    break;
		    
		string reason;
		Rcl::SearchData *sd = 
		    wasaStringToRcl(m_q->whatDb()->getConf(), 
				    fs.values[i], reason);
		if (sd)  {
		    Rcl::SearchDataClauseSub *cl1 = 
			new Rcl::SearchDataClauseSub(Rcl::SCLT_SUB, 
						     RefCntr<Rcl::SearchData>(sd));
		    m_fsdata->addClause(cl1);
		}
	    }
	    break;
	    default:
		break;
	    }
	}
	m_isFiltered = true;
    } else {
	m_fsdata = m_sdata;
	m_isFiltered = false;
    }
    m_needSetQuery = true;
    return true;
}

bool DocSequenceDb::setSortSpec(const DocSeqSortSpec &spec) 
{
    LOGDEB(("DocSequenceDb::setSortSpec: fld [%s] %s\n", 
	    spec.field.c_str(), spec.desc ? "desc" : "asc"));
    if (spec.isNotNull()) {
	m_q->setSortBy(spec.field, !spec.desc);
	m_isSorted = true;
    } else {
	m_q->setSortBy(string(), true);
	m_isSorted = false;
    }
    m_needSetQuery = true;
    return true;
}

bool DocSequenceDb::setQuery()
{
    if (!m_needSetQuery)
	return true;
    m_rescnt = -1;
    m_needSetQuery = !m_q->setQuery(m_fsdata);
    return !m_needSetQuery;
}
