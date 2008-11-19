#ifndef _reslistpager_h_included_
#define _reslistpager_h_included_
/* @(#$Id: reslistpager.h,v 1.1 2008-11-19 12:19:40 dockes Exp $  (C) 2007 J.F.Dockes */

#include <vector>
using std::vector;

#include "refcntr.h"
#include "docseq.h"

/**
 * Produces html text for a paged result list. 
 */
class ResListPager {
public:
    ResListPager(RefCntr<DocSequence> src, int pagesize, 
		 const string& parformat)
	: m_docSource(src), m_pagesize(pagesize), m_parformat(parformat),
	  m_hasNext(false) 
    {}
    virtual ~ResListPager() {}
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
    virtual bool append(const string& data);
    virtual string tr(const string& in);
private:
    // First docnum (from docseq) in current page
    int                  m_winfirst;
    RefCntr<DocSequence> m_docSource;
    int                  m_pagesize;
    string               m_parformat;

    bool                 m_hasNext;
    vector<ResListEntry> m_respage;
};

#endif /* _reslistpager_h_included_ */
