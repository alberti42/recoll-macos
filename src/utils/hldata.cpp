/* Copyright (C) 2016 J.F.Dockes
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
#include "autoconfig.h"

#include <stdio.h>

#include "hldata.h"

using std::string;
using std::map;

void HighlightData::toString(string& out)
{
    out.append("\nUser terms (orthograph): ");
    for (std::set<string>::const_iterator it = uterms.begin();
            it != uterms.end(); it++) {
        out.append(" [").append(*it).append("]");
    }
    out.append("\nUser terms to Query terms:");
    for (map<string, string>::const_iterator it = terms.begin();
            it != terms.end(); it++) {
        out.append("[").append(it->first).append("]->[");
        out.append(it->second).append("] ");
    }
    out.append("\nGroups: ");
    char cbuf[200];
    sprintf(cbuf, "Groups size %d grpsugidx size %d ugroups size %d",
            int(groups.size()), int(grpsugidx.size()), int(ugroups.size()));
    out.append(cbuf);

    size_t ugidx = (size_t) - 1;
    for (unsigned int i = 0; i < groups.size(); i++) {
        if (ugidx != grpsugidx[i]) {
            ugidx = grpsugidx[i];
            out.append("\n(");
            for (unsigned int j = 0; j < ugroups[ugidx].size(); j++) {
                out.append("[").append(ugroups[ugidx][j]).append("] ");
            }
            out.append(") ->");
        }
        out.append(" {");
        for (unsigned int j = 0; j < groups[i].size(); j++) {
            out.append("[").append(groups[i][j]).append("]");
        }
        sprintf(cbuf, "%d", slacks[i]);
        out.append("}").append(cbuf);
    }
    out.append("\n");
}

void HighlightData::append(const HighlightData& hl)
{
    uterms.insert(hl.uterms.begin(), hl.uterms.end());
    terms.insert(hl.terms.begin(), hl.terms.end());
    size_t ugsz0 = ugroups.size();
    ugroups.insert(ugroups.end(), hl.ugroups.begin(), hl.ugroups.end());

    groups.insert(groups.end(), hl.groups.begin(), hl.groups.end());
    slacks.insert(slacks.end(), hl.slacks.begin(), hl.slacks.end());
    for (std::vector<size_t>::const_iterator it = hl.grpsugidx.begin();
            it != hl.grpsugidx.end(); it++) {
        grpsugidx.push_back(*it + ugsz0);
    }
}
