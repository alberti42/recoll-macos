#ifndef lint
static char rcsid[] = "@(#$Id: sortseq.cpp,v 1.2 2005-12-02 14:18:44 dockes Exp $ (C) 2005 J.F.Dockes";
#endif
#include <algorithm>

#include "debuglog.h"
#include "sortseq.h"

using std::string;

class CompareDocs { 
    RclSortSpec ss;
public: 
    CompareDocs(const RclSortSpec &sortspec) : ss(sortspec) {}

    int operator()(const Rcl::Doc &x, const Rcl::Doc &y) 
    { 
	LOGDEB(("Comparing .. \n"));
	for (unsigned int i = 0; i < ss.crits.size(); i++) {
	    switch (ss.crits[i]) {
	    case RclSortSpec::RCLFLD_MTIME:
		{
		    LOGDEB((" MTIME\n"));
		    long xmtime = x.dmtime.empty() ? atol(x.fmtime.c_str()) :
			atol(x.dmtime.c_str());
		    long ymtime = y.dmtime.empty() ? atol(y.fmtime.c_str()) :
			atol(y.dmtime.c_str());
		    
		    if (ss.dirs[i] ? xmtime > ymtime : xmtime < ymtime)
			return 1;
		}
		break;
	    case RclSortSpec::RCLFLD_URL:
		LOGDEB((" URL\n"));
		if (ss.dirs[i] ? x.url > y.url :  x.url < y.url)
		    return 1;
		break;
	    case RclSortSpec::RCLFLD_IPATH: 
		LOGDEB((" IPATH\n"));
		if (ss.dirs[i] ? x.ipath > y.ipath : x.ipath < y.ipath)
		    return 1;
		break;
	    case RclSortSpec::RCLFLD_MIMETYPE:
		LOGDEB((" MIMETYPE\n"));
		if (ss.dirs[i] ? x.mimetype > y.mimetype : 
		    x.mimetype < y.mimetype)
		    return 1;
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
    m_pcs.resize(cnt);
    int i;
    for (i = 0; i < cnt; i++) {
	if (!iseq.getDoc(i, m_docs[i], &m_pcs[i])) {
	    LOGERR(("DocSeqSorted: getDoc failed for doc %d\n", i));
	    break;
	}
    }
    m_count = i;
    LOGDEB(("DocSeqSorted:: m_count %d\n", m_count));
    m_docs.resize(m_count);
    m_pcs.resize(m_count);
    m_title = string("Sorted ") + iseq.title();
    CompareDocs cmp(sortspec);
    sort(m_docs.begin(), m_docs.end(), cmp);
}

bool DocSeqSorted::getDoc(int num, Rcl::Doc &doc, int *percent, string *)
{
    LOGDEB1(("DocSeqSorted: fetching %d\n", num));
    
    if (num >= m_count)
	return false;
    if (percent)
	*percent = m_pcs[num];
    doc = m_docs[num];
    return true;
}
