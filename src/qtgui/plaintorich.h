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
/* @(#$Id: plaintorich.h,v 1.15 2007-10-18 10:39:41 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>
using std::list;
using std::string;

// A data struct to hold words and groups of words to be highlighted
struct HiliteData {
    vector<string> terms;
    vector<vector<string> > groups;
    vector<int> gslks; // group slacks (number of permitted non-matched words)
};

/**
 * Transform plain text into qt rich text for the preview window.
 *
 * We escape characters like < or &, and add qt rich text tags to 
 * colorize the query terms. The latter is a quite complicated matter because
 * of phrase/near searches. We treat all such searches as "near", not "phrase"
 * 
 * @param in          raw text out of internfile.
 * @param out         rich text output, divided in chunks (to help our caller
 *          avoid inserting half tags into textedit which doesnt like it)
 * @param hdata       terms and groups to be highlighted. These are
 *                     lowercase and unaccented.
 * @param noHeader    if true don't output header (<qt><title>...)
 * @param needBeacons Need to navigate highlighted terms, mark them.
 */
extern bool plaintorich(const string &in, list<string> &out,
			const HiliteData& hdata,
			bool noHeader,
			bool needBeacons,
			int chunksize = 50000
			);

extern string termAnchorName(int i);

#define QT_SCROLL_TO_ANCHOR_BUG
#ifdef QT_SCROLL_TO_ANCHOR_BUG
// For some reason, can't get scrollToAnchor() to work. We use a special 
// string as a beacon for the match area.
extern const char *firstTermBeacon;
#endif

#endif /* _PLAINTORICH_H_INCLUDED_ */
