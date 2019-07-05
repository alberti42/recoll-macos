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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "autoconfig.h"

#include "hldata.h"

#include <algorithm>
#include <limits.h>

#include "log.h"
#include "smallut.h"

using std::string;
using std::map;
using std::vector;
using std::pair;

#undef DEBUGGROUPS
#ifdef DEBUGGROUPS
#define LOGRP LOGDEB
#else
#define LOGRP LOGDEB1
#endif

static inline void setWinMinMax(int pos, int& sta, int& sto)
{
    if (pos < sta) {
        sta = pos;
    }
    if (pos > sto) {
        sto = pos;
    }
}

/*
 * @param window the total width for the "near" area, in positions.

 * @param plists the position vectors for the terms. The array is
 *    sorted shorted first for optimization. The function does a
 *    recursive call on the next array if the match is still possible
 *    after dealing with the current one

 * @param plist_idx the index for the position list we will work with.
 * @param min, max the current minimum and maximum term positions.
 * @param[output] sp, ep, the start and end positions of the found match.
 * @param minpos  Highest end of a found match. While looking for
 *   further matches, we don't want the search to extend before
 *   this, because it does not make sense for highlight regions to
 *   overlap.
 * @param isphrase if true, the position lists are in term order, and
 *     we only look for the next match beyond the current window top.
 */
static bool do_proximity_test(
    const int window, vector<const vector<int>*>& plists,
    unsigned int plist_idx, int min, int max, int *sp, int *ep, int minpos,
    bool isphrase)
{
    LOGINF("do_prox_test: win " << window << " plist_idx " << plist_idx <<
           " min " <<  min << " max " << max << " minpos " << minpos <<
           " isphrase " << isphrase << "\n");

    // Overlap interdiction: possibly adjust window start by input minpos
    int actualminpos = isphrase ? max + 1 : max + 1 - window;
    if (actualminpos < minpos)
        actualminpos = minpos;

    // Find 1st position bigger than window start
    auto it = plists[plist_idx]->begin();
    while (it != plists[plist_idx]->end() && *it < actualminpos)
        it++;

    // Look for position inside window. If not found, no match. If
    // found: if this is the last list we're done, else recurse on
    // next list after adjusting the window
    while (it != plists[plist_idx]->end()) {
        int pos = *it;
        if (pos > min + window - 1) 
            return false;
        if (plist_idx + 1 == plists.size()) {
            // Done: set return values
            setWinMinMax(pos, *sp, *ep);
            return true;
        }
        setWinMinMax(pos, min, max);
        if (do_proximity_test(window,plists, plist_idx + 1,
                              min, max, sp, ep, minpos)) {
            return true;
        }
        it++;
    }
    return false;
}


// Find matches for one group of terms
bool matchGroup(const HighlightData& hldata,
                unsigned int grpidx,
                const map<string, vector<int>>& inplists,
                const map<int, pair<int,int>>& gpostobytes,
                vector<GroupMatchEntry>& tboffs,
                bool isphrase
    )
{
    isphrase=true;
    const vector<string>& terms = hldata.index_term_groups[grpidx];
    int window = int(terms.size() + hldata.slacks[grpidx]);

    LOGRP("TextSplitPTR::matchGroup:d " << window << ": " <<
          stringsToString(terms) << "\n");

    // The position lists we are going to work with. We extract them from the 
    // (string->plist) map
    vector<const vector<int>*> plists;
    // A revert plist->term map. This is so that we can find who is who after
    // sorting the plists by length.
    map<const vector<int>*, string> plistToTerm;

    // Find the position list for each term in the group. It is
    // possible that this particular group was not actually matched by
    // the search, so that some terms are not found.
    for (const auto& term : terms) {
        map<string, vector<int> >::const_iterator pl = inplists.find(term);
        if (pl == inplists.end()) {
            LOGRP("TextSplitPTR::matchGroup: [" << term <<
                  "] not found in plists\n");
            return false;
        }
        plists.push_back(&(pl->second));
        plistToTerm[&(pl->second)] = term;
    }
    // I think this can't actually happen, was useful when we used to
    // prune the groups, but doesn't hurt.
    if (plists.size() < 2) {
        LOGRP("TextSplitPTR::matchGroup: no actual groups found\n");
        return false;
    }

    if (!isphrase) {
        // Sort the positions lists so that the shorter is first
        std::sort(plists.begin(), plists.end(),
                  [](const vector<int> *a, const vector<int> *b) -> bool {
                      return a->size() < b->size();
                  }
            );
    }
    
    if (0) { // Debug
        auto it = plistToTerm.find(plists[0]);
        if (it == plistToTerm.end()) {
            // SuperWeird
            LOGERR("matchGroup: term for first list not found !?!\n");
            return false;
        }
        LOGRP("matchGroup: walking the shortest plist. Term [" <<
              it->second << "], len " << plists[0]->size() << "\n");
    }

    // Minpos is the highest end of a found match. While looking for
    // further matches, we don't want the search to extend before
    // this, because it does not make sense for highlight regions to
    // overlap
    int minpos = 0;
    // Walk the shortest plist and look for matches
    for (int pos : *(plists[0])) {
        int sta = INT_MAX, sto = 0;
        LOGDEB2("MatchGroup: Testing at pos " << pos << "\n");
        if (do_proximity_test(
                window, plists, 1, pos, pos, &sta, &sto, minpos, isphrase)) {
            setWinMinMax(pos, sta, sto);
            LOGINF("TextSplitPTR::matchGroup: MATCH termpos [" << sta <<
                   "," << sto << "]\n"); 
            minpos = sto + 1;
            // Translate the position window into a byte offset window
            auto i1 =  gpostobytes.find(sta);
            auto i2 =  gpostobytes.find(sto);
            if (i1 != gpostobytes.end() && i2 != gpostobytes.end()) {
                LOGDEB2("TextSplitPTR::matchGroup: pushing bpos " <<
                        i1->second.first << " " << i2->second.second << "\n");
                tboffs.push_back(GroupMatchEntry(i1->second.first, 
                                                 i2->second.second, grpidx));
            } else {
                LOGDEB0("matchGroup: no bpos found for " << sta << " or "
                        << sto << "\n");
            }
        } else {
            LOGRP("matchGroup: no group match found at this position\n");
        }
    }

    return true;
}

void HighlightData::toString(string& out) const
{
    out.append("\nUser terms (orthograph): ");
    for (const auto& term : uterms) {
        out.append(" [").append(term).append("]");
    }
    out.append("\nUser terms to Query terms:");
    for (const auto& entry: terms) {
        out.append("[").append(entry.first).append("]->[");
        out.append(entry.second).append("] ");
    }
    out.append("\nGroups: ");
    char cbuf[200];
    sprintf(cbuf, "Groups size %d grpsugidx size %d ugroups size %d",
            int(index_term_groups.size()), int(grpsugidx.size()),
            int(ugroups.size()));
    out.append(cbuf);

    size_t ugidx = (size_t) - 1;
    for (unsigned int i = 0; i < index_term_groups.size(); i++) {
        if (ugidx != grpsugidx[i]) {
            ugidx = grpsugidx[i];
            out.append("\n(");
            for (unsigned int j = 0; j < ugroups[ugidx].size(); j++) {
                out.append("[").append(ugroups[ugidx][j]).append("] ");
            }
            out.append(") ->");
        }
        out.append(" {");
        for (unsigned int j = 0; j < index_term_groups[i].size(); j++) {
            out.append("[").append(index_term_groups[i][j]).append("]");
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

    index_term_groups.insert(index_term_groups.end(),
                             hl.index_term_groups.begin(),
                             hl.index_term_groups.end());
    slacks.insert(slacks.end(), hl.slacks.begin(), hl.slacks.end());
    for (std::vector<size_t>::const_iterator it = hl.grpsugidx.begin();
         it != hl.grpsugidx.end(); it++) {
        grpsugidx.push_back(*it + ugsz0);
    }
}
