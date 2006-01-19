#ifndef _PLAINTORICH_H_INCLUDED_
#define _PLAINTORICH_H_INCLUDED_
/* @(#$Id: plaintorich.h,v 1.3 2006-01-19 12:01:43 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

/**
 * Transform plain text into qt rich text for the preview window.
 *
 * We mainly escape characters like < or &, and add qt rich text tags to 
 * colorize the query terms.
 * 
 * @param in          raw text out of internfile.
 * @param terms       list of query terms
 * @param termoffsets output: character offsets where we find terms.
 */
extern string plaintorich(const string &in,
			  const list<string>& terms,
			  list<pair<int, int> >&termoffsets);

#endif /* _PLAINTORICH_H_INCLUDED_ */
