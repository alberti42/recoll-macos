#ifndef lint
static char rcsid[] = "@(#$Id: docseq.cpp,v 1.5 2005-12-07 15:41:50 dockes Exp $ (C) 2005 J.F.Dockes";
#endif
#include <math.h>
#include <time.h>

#include "docseq.h"

bool DocSequenceDb::getDoc(int num, Rcl::Doc &doc, int *percent, string *sh)
{
    if (sh) sh->erase();
    return m_db ? m_db->getDoc(num, doc, percent) : false;
}

int DocSequenceDb::getResCnt()
{
    if (!m_db)
	return -1;
    // Need to fetch one document before we can get the result count 
    if (m_rescnt < 0) {
	Rcl::Doc doc;
	int percent;
	m_db->getDoc(0, doc, &percent);
	m_rescnt= m_db->getResCnt();
    }
    return m_rescnt;
}


bool DocSequenceHistory::getDoc(int num, Rcl::Doc &doc, int *percent, 
				string *sh) {
    // Retrieve history list
    if (!m_hist)
	return false;
    if (m_hlist.empty())
	m_hlist = m_hist->getDocHistory();

    if (num < 0 || num >= (int)m_hlist.size())
	return false;
    int skip;
    if (m_prevnum >= 0 && num >= m_prevnum) {
	skip = num - m_prevnum;
    } else {
	skip = num;
	m_it = m_hlist.begin();
	m_prevtime = -1;
    }
    m_prevnum = num;
    while (skip--) 
	m_it++;
    if (percent)
	*percent = 100;
    if (sh) {
	if (m_prevtime < 0 || abs(m_prevtime - m_it->unixtime) > 86400) {
	    m_prevtime = m_it->unixtime;
	    time_t t = (time_t)(m_it->unixtime);
	    *sh = string(ctime(&t));
	    // Get rid of the final \n in ctime
	    sh->erase(sh->length()-1);
	} else
	    sh->erase();
    }
    return m_db->getDoc(m_it->fn, m_it->ipath, doc);
}

int DocSequenceHistory::getResCnt()
{	
    if (m_hlist.empty())
	m_hlist = m_hist->getDocHistory();
    return m_hlist.size();
}

