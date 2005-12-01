#ifndef lint
static char rcsid[] = "@(#$Id: sortseq.cpp,v 1.1 2005-12-01 16:23:09 dockes Exp $ (C) 2005 J.F.Dockes";
#endif
#include <algorithm>

#include "debuglog.h"
#include "sortseq.h"

using std::string;

class CompareDocs { 
    RclSortSpec ss;
public: 
    CompareDocs(const RclSortSpec &sortspec) : ss(sortspec) {}

    int operator()(const Rcl::Doc &x, const Rcl::Doc &y) { 
	for (unsigned int i = 0; i < ss.crits.size(); i++) {
	    switch (ss.crits[i]) {
	    case RclSortSpec::RCLFLD_MTIME:
		{
		    long xmtime = x.dmtime.empty() ? atol(x.fmtime.c_str()) :
			atol(x.dmtime.c_str());
		    long ymtime = y.dmtime.empty() ? atol(y.fmtime.c_str()) :
			atol(y.dmtime.c_str());
		    if (ss.dirs[i])
			return xmtime > ymtime;
		    else
			return xmtime < ymtime;
		}
		continue;
	    case RclSortSpec::RCLFLD_URL:
		if (ss.dirs[i])
		    return x.url > y.url;
		else
		    return x.url < y.url;
		continue;
	    case RclSortSpec::RCLFLD_IPATH: 
		if (ss.dirs[i])
		    return x.ipath > y.ipath;
		else
		    return x.ipath < y.ipath;
		continue;
	    case RclSortSpec::RCLFLD_MIMETYPE:
		if (ss.dirs[i])
		    return x.mimetype > y.mimetype;
		else
		    return x.mimetype < y.mimetype;
		continue;
	    }
	}
	// Did all comparisons: must be equal
	return 0;
    } 
};

DocSeqSorted::DocSeqSorted(DocSequence &iseq, int cnt, RclSortSpec &sortspec)
{
    LOGDEB1(("DocSeqSorted:: input cnt %d\n", cnt));
    
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
    LOGDEB1(("DocSeqSorted:: m_count %d\n", m_count));
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
