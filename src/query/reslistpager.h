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
    ResListPager(int pagesize=10) :
        m_pagesize(pagesize),
        m_newpagesize(pagesize),
        m_winfirst(-1),
        m_hasNext(false),
        m_hiliter(0)
    {
    }
    virtual ~ResListPager() {}

    void setHighLighter(PlainToRich *ptr) 
    {
        m_hiliter = ptr;
    }
    void setDocSource(RefCntr<DocSequence> src)
    {
        m_pagesize = m_newpagesize;
	m_winfirst = -1;
	m_hasNext = false;
	m_docSource = src;
	m_respage.clear();
    }
    void setPageSize(int ps) 
    {
        m_newpagesize = ps;
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
    void resultPageFirst() {
	m_winfirst = -1;
        m_pagesize = m_newpagesize;
	resultPageNext();
    }
    void resultPageBack() {
	if (m_winfirst <= 0) return;
	m_winfirst -= 2 * m_pagesize;
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
    // Translation function. This is reimplemented in the qt reslist
    // object For this to work, the strings must be duplicated inside
    // reslist.cpp (see the QT_TR_NOOP in there). Very very unwieldy.
    // To repeat: any change to a string used with trans() inside
    // reslistpager.cpp must be reflected in the string table inside
    // reslist.cpp for translation to work.
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
    int                  m_pagesize;
    int                  m_newpagesize;
    // First docnum (from docseq) in current page
    int                  m_winfirst;
    bool                 m_hasNext;
    PlainToRich         *m_hiliter;
    RefCntr<DocSequence> m_docSource;
    vector<ResListEntry> m_respage;
};

#endif /* _reslistpager_h_included_ */
