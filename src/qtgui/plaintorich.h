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
#ifndef _PLAINTORICH_H_INCLUDED_
#define _PLAINTORICH_H_INCLUDED_
/* @(#$Id: plaintorich.h,v 1.5 2006-01-30 11:15:28 dockes Exp $  (C) 2004 J.F.Dockes */

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
extern bool plaintorich(const string &in, string &out,
			const list<string>& terms,
			list<pair<int, int> >& termoffsets);

#endif /* _PLAINTORICH_H_INCLUDED_ */
