#ifndef lint
static char rcsid[] = "@(#$Id: sortseq.cpp,v 1.9 2006-12-05 15:18:48 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include <algorithm>

#include "debuglog.h"
#include "sortseq.h"

using std::string;

class CompareDocs { 
    DocSeqSortSpec ss;
public: 
    CompareDocs(const DocSeqSortSpec &sortspec) : ss(sortspec) {}

    // It's not too clear in the std::sort doc what this should do. This 
    // behaves as operator< 
    int operator()(const Rcl::Doc *x, const Rcl::Doc *y) 
    { 
	LOGDEB1(("Comparing .. \n"));

	// Compare using each criterion in term. Further comparisons must only 
	// be made if previous order ones are equal.
	for (unsigned int i = 0; i < ss.crits.size(); i++) {
	    switch (ss.crits[i]) {
	    case DocSeqSortSpec::RCLFLD_MTIME:
		{
		    long xmtime = x->dmtime.empty() ? atol(x->fmtime.c_str()) :
			atol(x->dmtime.c_str());
		    long ymtime = y->dmtime.empty() ? atol(y->fmtime.c_str()) :
			atol(y->dmtime.c_str());
		    
		    LOGDEB1((" MTIME %ld %ld\n", xmtime, ymtime));
		    if (ss.dirs[i] ? xmtime > ymtime : xmtime < ymtime)
			return 1;
		    else if (xmtime != ymtime)
			return 0;
		}
		break;
	    case DocSeqSortSpec::RCLFLD_URL:
		LOGDEB1((" URL\n"));
		if (ss.dirs[i] ? x->url > y->url :  x->url < y->url)
		    return 1;
		else if (x->url != y->url)
		    return 0;
		break;
	    case DocSeqSortSpec::RCLFLD_IPATH: 
		LOGDEB1((" IPATH\n"));
		if (ss.dirs[i] ? x->ipath > y->ipath : x->ipath < y->ipath)
		    return 1;
		else if (x->ipath != y->ipath)
		    return 0;
		break;
	    case DocSeqSortSpec::RCLFLD_MIMETYPE:
		LOGDEB1((" MIMETYPE\n"));
		if (ss.dirs[i] ? x->mimetype > y->mimetype : 
		    x->mimetype < y->mimetype)
		    return 1;
		else if (x->mimetype != y->mimetype)
		    return 0;
		break;
	    }
	}
	// Did all comparisons
	return 0;
    } 
};

DocSeqSorted::DocSeqSorted(RefCntr<DocSequence> iseq, DocSeqSortSpec &sortspec,
			   const std::string &t)
    : DocSequence(t)
{
    m_spec = sortspec;
    LOGDEB(("DocSeqSorted:: count %d\n", m_spec.sortwidth));
    m_docs.resize(m_spec.sortwidth);
    int i;
    for (i = 0; i < m_spec.sortwidth; i++) {
	int percent;
	if (!iseq->getDoc(i, m_docs[i], &percent)) {
	    LOGERR(("DocSeqSorted: getDoc failed for doc %d\n", i));
	    break;
	}
	m_docs[i].pc = percent;
    }
    m_spec.sortwidth = i;
    LOGDEB(("DocSeqSorted:: m_count %d\n", m_spec.sortwidth));
    m_docs.resize(m_spec.sortwidth);
    m_docsp.resize(m_spec.sortwidth);
    for (i = 0; i < m_spec.sortwidth; i++)
	m_docsp[i] = &m_docs[i];

    CompareDocs cmp(sortspec);
    sort(m_docsp.begin(), m_docsp.end(), cmp);
}

bool DocSeqSorted::getDoc(int num, Rcl::Doc &doc, int *percent, string *)
{
    LOGDEB1(("DocSeqSorted: fetching %d\n", num));
    
    if (num >= m_spec.sortwidth)
	return false;
    if (percent)
	*percent = (*m_docsp[num]).pc;
    doc = *m_docsp[num];
    return true;
}
