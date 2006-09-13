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
/* @(#$Id: plaintorich.h,v 1.7 2006-09-13 14:57:56 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

/**
 * Transform plain text into qt rich text for the preview window.
 *
 * We mainly escape characters like < or &, and add qt rich text tags to 
 * colorize the query terms.
 * 
 * @param in          raw text out of internfile.
 * @param out         rich text output
 * @param terms       list of query terms. These are out of Rcl::Db and dumb
 * @param firstTerm   out: value of the first search term in text.
 * @param noHeader    if true don't output header (<qt><title>...)
 */
extern bool plaintorich(const string &in, string &out,
			const list<string>& terms,
			string* firstTerm, bool noHeader = false);

#endif /* _PLAINTORICH_H_INCLUDED_ */
