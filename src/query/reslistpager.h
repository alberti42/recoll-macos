#ifndef _reslistpager_h_included_
#define _reslistpager_h_included_
/* @(#$Id: reslistpager.h,v 1.4 2008-12-16 14:20:10 dockes Exp $  (C) 2007 J.F.Dockes */

#include <vector>
using std::vector;

#include "refcntr.h"
#include "docseq.h"

class PlainToRich;

/**
 * Manage a paged HTML result list. 
 */
class ResListPager {
public:
    ResListPager(int pagesize=10) : m_pagesize(pagesize) {initall();}
    virtual ~ResListPager() {}

    void setHighLighter(PlainToRich *ptr) {m_hiliter = ptr;}
    void setDocSource(RefCntr<DocSequence> src)
    {
	m_winfirst = -1;
	m_hasNext = false;
	m_respage.clear();
	m_docSource = src;
    }

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
    virtual bool append(const string& data, int, const Rcl::Doc&)
    {
	return append(data);
    }
    virtual string trans(const string& in);
    virtual string detailsLink();
    virtual const string &parFormat();
    virtual string nextUrl();
    virtual string prevUrl();
    virtual string pageTop() {return string();}
    virtual string iconPath(const string& mtype);
    virtual void suggest(const vector<string>, vector<string>&sugg) {
        sugg.clear();
    }
private:
    void initall() 
    {
	m_winfirst = -1;
	m_hasNext = false;
	m_respage.clear();
	m_hiliter = 0;
    }

    // First docnum (from docseq) in current page
    int                  m_winfirst;
    RefCntr<DocSequence> m_docSource;
    const int            m_pagesize;

    bool                 m_hasNext;
    vector<ResListEntry> m_respage;
    PlainToRich         *m_hiliter;
};

#endif /* _reslistpager_h_included_ */
