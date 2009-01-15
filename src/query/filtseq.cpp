#ifndef lint
static char rcsid[] = "@(#$Id: filtseq.cpp,v 1.4 2008-10-13 11:46:06 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include <algorithm>

#include "debuglog.h"
#include "filtseq.h"

using std::string;

static bool filter(const DocSeqFiltSpec& fs, const Rcl::Doc *x)
{
    // Compare using each criterion in term. We're doing an or:
    // 1st ok ends 
    for (unsigned int i = 0; i < fs.crits.size(); i++) {
	switch (fs.crits[i]) {
	case DocSeqFiltSpec::DSFS_MIMETYPE:
	    LOGDEB1((" MIMETYPE\n"));
	    if (x->mimetype == fs.values[i])
		return 1;
	}
    }
    // Did all comparisons
    return 0;
} 

DocSeqFiltered::DocSeqFiltered(RefCntr<DocSequence> iseq, 
			       DocSeqFiltSpec &filtspec,
			       const std::string &t)
    :  DocSeqModifier(t, iseq), m_spec(filtspec)
{
}

bool DocSeqFiltered::setFiltSpec(DocSeqFiltSpec &filtspec)
{
    m_spec = filtspec;
    m_dbindices.clear();
    return true;
}

bool DocSeqFiltered::getDoc(int idx, Rcl::Doc &doc, string *)
{
    LOGDEB1(("DocSeqFiltered: fetching %d\n", idx));

    if (idx >= (int)m_dbindices.size()) {
	// Have to fetch xapian docs and filter until we get enough or
	// fail
	m_dbindices.reserve(idx+1);

	// First backend seq doc we fetch is the one after last stored 
	int backend_idx = m_dbindices.size() > 0 ? m_dbindices.back() + 1 : 0;

	// Loop until we get enough docs
	Rcl::Doc tdoc;
	int i = 0;
	while (idx >= (int)m_dbindices.size()) {
	    if (!m_seq->getDoc(backend_idx, tdoc)) 
		return false;
	    if (filter(m_spec, &tdoc)) {
		m_dbindices.push_back(backend_idx);
	    }
	    backend_idx++;
	}
	doc = tdoc;
    } else {
	// The corresponding backend indice is already known
	if (!m_seq->getDoc(m_dbindices[idx], doc)) 
	    return false;
    }
    return true;
}
