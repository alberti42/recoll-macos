#ifndef lint
static char rcsid[] = "@(#$Id: docseqdb.cpp,v 1.2 2007-01-19 15:22:50 dockes Exp $ (C) 2005 J.F.Dockes";
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

bool DocSequenceDb::getDoc(int num, Rcl::Doc &doc, int *percent, string *sh)
{
    if (sh) sh->erase();
    return m_db ? m_db->getDoc(num, doc, percent) : false;
}

int DocSequenceDb::getResCnt()
{
    if (!m_db)
	return -1;
    if (m_rescnt < 0) {
	m_rescnt= m_db->getResCnt();
    }
    return m_rescnt;
}

string DocSequenceDb::getAbstract(Rcl::Doc &doc)
{
    if (!m_db)
	return doc.abstract;
    string abstract;
    m_db->makeDocAbstract(doc, abstract);
    return abstract.empty() ? doc.abstract : abstract;
}

