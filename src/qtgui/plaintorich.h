#ifndef _PLAINTORICH_H_INCLUDED_
#define _PLAINTORICH_H_INCLUDED_
/* @(#$Id: plaintorich.h,v 1.1 2005-09-22 16:22:34 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

/**
 * Fix result text for display inside the gui text window.
 * 
 * @param in          raw text out of internfile.
 * @param terms       list of query terms
 * @param termoffsets character offsets where we find terms
 */
extern string plaintorich(const string &in,
			  const list<string>& terms,
			  list<pair<int, int> >&termoffsets);

#endif /* _PLAINTORICH_H_INCLUDED_ */
