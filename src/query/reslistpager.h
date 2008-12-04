#ifndef _reslistpager_h_included_
#define _reslistpager_h_included_
/* @(#$Id: reslistpager.h,v 1.3 2008-12-04 11:49:59 dockes Exp $  (C) 2007 J.F.Dockes */

#include <vector>
using std::vector;

#include "refcntr.h"
#include "docseq.h"

/**
 * Produces html text for a paged result list. 
 */
class ResListPager {
public:
    ResListPager() : m_pagesize(10), m_hasNext(false) {}
    void setDocSource(RefCntr<DocSequence> src)
    {
	m_winfirst = -1;
	m_docSource = src;
	m_hasNext = false;
	m_respage.clear();
    }
    ResListPager(RefCntr<DocSequence> src, int pagesize)
	: m_winfirst(-1), m_docSource(src), m_pagesize(pagesize), 
	m_hasNext(false) 
    {}
    virtual ~ResListPager() {}
    int pageNumber() 
    {
	if (m_winfirst < 0 || m_pagesize <= 0)
	    return -1;
	return m_winfirst / m_pagesize;
    }
    virtual int pageSize() const {return m_pagesize;}
    void pageNext();
    bool hasNext() {return m_hasNext;}
    bool hasPrev() {return m_winfirst > 0;}
    bool atBot() {return m_winfirst <= 0;}
    void resultPageBack() {
	if (m_winfirst <= 0) return;
	m_winfirst -= 2 * m_pagesize;
	resultPageNext();
    }
    void resultPageFirst() {
	m_winfirst = -1;
	resultPageNext();
    }
    void resultPageNext();
    void displayPage();
    bool pageEmpty() {return m_respage.size() == 0;}

    string queryDescription() {return m_docSource.isNull() ? "" :
	    m_docSource->getDescription();}

    // Things that need to be reimplemented in the subclass:
    virtual bool append(const string& data);
    virtual string tr(const string& in);
    virtual string detailsLink();
    virtual const string &parFormat();
    virtual string nextUrl();
    virtual string prevUrl();
    virtual string pageTop() {return string();}
private:
    // First docnum (from docseq) in current page
    int                  m_winfirst;
    RefCntr<DocSequence> m_docSource;
    const int            m_pagesize;

    bool                 m_hasNext;
    vector<ResListEntry> m_respage;
};

#endif /* _reslistpager_h_included_ */
