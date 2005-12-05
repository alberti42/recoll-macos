#ifndef lint
static char rcsid[] = "@(#$Id: sortseq.cpp,v 1.4 2005-12-05 12:02:01 dockes Exp $ (C) 2005 J.F.Dockes";
#endif
#include <algorithm>

#include "debuglog.h"
#include "sortseq.h"

using std::string;

class CompareDocs { 
    RclSortSpec ss;
public: 
    CompareDocs(const RclSortSpec &sortspec) : ss(sortspec) {}

    // It's not too clear in the std::sort doc what this should do. This 
    // behaves as operator< 
    int operator()(const Rcl::Doc *x, const Rcl::Doc *y) 
    { 
	LOGDEB1(("Comparing .. \n"));

	// Compare using each criterion in term. Further comparisons must only 
	// be made if previous order ones are equal.
	for (unsigned int i = 0; i < ss.crits.size(); i++) {
	    switch (ss.crits[i]) {
	    case RclSortSpec::RCLFLD_MTIME:
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
	    case RclSortSpec::RCLFLD_URL:
		LOGDEB1((" URL\n"));
		if (ss.dirs[i] ? x->url > y->url :  x->url < y->url)
		    return 1;
		else if (x->url != y->url)
		    return 0;
		break;
	    case RclSortSpec::RCLFLD_IPATH: 
		LOGDEB1((" IPATH\n"));
		if (ss.dirs[i] ? x->ipath > y->ipath : x->ipath < y->ipath)
		    return 1;
		else if (x->ipath != y->ipath)
		    return 0;
		break;
	    case RclSortSpec::RCLFLD_MIMETYPE:
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

DocSeqSorted::DocSeqSorted(DocSequence &iseq, int cnt, RclSortSpec &sortspec)
{
    LOGDEB(("DocSeqSorted:: count %d\n", cnt));
    
    m_docs.resize(cnt);
    int i;
    for (i = 0; i < cnt; i++) {
	int percent;
	if (!iseq.getDoc(i, m_docs[i], &percent)) {
	    LOGERR(("DocSeqSorted: getDoc failed for doc %d\n", i));
	    break;
	}
	m_docs[i].pc = percent;
    }
    m_count = i;
    LOGDEB(("DocSeqSorted:: m_count %d\n", m_count));
    m_docs.resize(m_count);
    m_docsp.resize(m_count);
    for (i = 0; i < m_count; i++)
	m_docsp[i] = &m_docs[i];

    m_title = iseq.title() + " (sorted)";
    CompareDocs cmp(sortspec);
    sort(m_docsp.begin(), m_docsp.end(), cmp);
}

bool DocSeqSorted::getDoc(int num, Rcl::Doc &doc, int *percent, string *)
{
    LOGDEB1(("DocSeqSorted: fetching %d\n", num));
    
    if (num >= m_count)
	return false;
    if (percent)
	*percent = (*m_docsp[num]).pc;
    doc = *m_docsp[num];
    return true;
}
