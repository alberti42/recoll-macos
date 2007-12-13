#ifndef lint
static char rcsid[] = "@(#$Id: docseqhist.cpp,v 1.2 2007-12-13 06:58:21 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include <math.h>
#include <time.h>
#include <cmath>

#include "docseqhist.h"
#include "rcldb.h"

bool DocSequenceHistory::getDoc(int num, Rcl::Doc &doc, int *percent, 
				string *sh) 
{
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
	if (m_prevtime < 0 || abs (float(m_prevtime) - float(m_it->unixtime)) > 86400) {
	    m_prevtime = m_it->unixtime;
	    time_t t = (time_t)(m_it->unixtime);
	    *sh = string(ctime(&t));
	    // Get rid of the final \n in ctime
	    sh->erase(sh->length()-1);
	} else
	    sh->erase();
    }
    return m_db->getDoc(m_it->fn, m_it->ipath, doc, percent);
}

int DocSequenceHistory::getResCnt()
{	
    if (m_hlist.empty())
	m_hlist = m_hist->getDocHistory();
    return m_hlist.size();
}
