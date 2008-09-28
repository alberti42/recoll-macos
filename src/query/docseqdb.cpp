#ifndef lint
static char rcsid[] = "@(#$Id: docseqdb.cpp,v 1.6 2008-09-28 07:40:56 dockes Exp $ (C) 2005 J.F.Dockes";
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

#include "docseqdb.h"
#include "rcldb.h"

DocSequenceDb::DocSequenceDb(RefCntr<Rcl::Query> q, const string &t, 
			     RefCntr<Rcl::SearchData> sdata) 
    : DocSequence(t), m_q(q), m_sdata(sdata), m_rescnt(-1) 
{
}

DocSequenceDb::~DocSequenceDb() 
{
}

bool DocSequenceDb::getTerms(vector<string>& terms, 
			     vector<vector<string> >& groups, 
			     vector<int>& gslks)
{
    return m_sdata->getTerms(terms, groups, gslks);
}

string DocSequenceDb::getDescription() 
{
    return m_sdata->getDescription();
}

bool DocSequenceDb::getDoc(int num, Rcl::Doc &doc, int *percent, string *sh)
{
    if (sh) sh->erase();
    return m_q->getDoc(num, doc, percent);
}

int DocSequenceDb::getResCnt()
{
    if (m_rescnt < 0) {
	m_rescnt= m_q->getResCnt();
    }
    return m_rescnt;
}

string DocSequenceDb::getAbstract(Rcl::Doc &doc)
{
    if (!m_q->whatDb())
	return doc.meta[Rcl::Doc::keyabs];
    string abstract;
    m_q->whatDb()->makeDocAbstract(doc, m_q.getptr(), abstract);
    return abstract.empty() ? doc.meta[Rcl::Doc::keyabs] : abstract;
}

list<string> DocSequenceDb::expand(Rcl::Doc &doc)
{
    return m_q->expand(doc);
}

