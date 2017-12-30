/* Copyright (C) 2004-2017 J.F.Dockes
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

#include <math.h>

#include <map>
#include <unordered_map>
#include <deque>
#include <algorithm>

#include "log.h"
#include "rcldb.h"
#include "rcldb_p.h"
#include "rclquery.h"
#include "rclquery_p.h"
#include "textsplit.h"
#include "hldata.h"
#include "chrono.h"
#include "unacpp.h"
#include "zlibut.h"

using namespace std;


namespace Rcl {

#warning NEAR and PHRASE

// Text splitter for finding the match terms in the doc text.
class TextSplitABS : public TextSplit {
public:

    struct MatchEntry {
        // Start/End byte offsets of fragment in the document text
        int start;
        int stop;
        double coef;
        // Position of the first matched term.
        unsigned int hitpos;
        // "best term" for this match
        string term;
        // Hilight areas (each is one or several contiguous match terms).
        vector<pair<int,int>> hlzones;
        
        MatchEntry(int sta, int sto, double c, vector<pair<int,int>>& hl,
                   unsigned int pos, string& trm) 
            : start(sta), stop(sto), coef(c), hitpos(pos) {
            hlzones.swap(hl);
            term.swap(trm);
        }
    };


    TextSplitABS(const vector<string>& matchTerms,
                 unordered_map<string, double>& wordcoefs,
                 unsigned int ctxwords,
                 Flags flags = TXTS_NONE)
        :  TextSplit(flags),  m_terms(matchTerms.begin(), matchTerms.end()),
           m_wordcoefs(wordcoefs), m_ctxwords(ctxwords) {
        LOGDEB("TextSPlitABS: ctxwords " << ctxwords << endl);
    }

    // Accept a word and its position. If the word is a matched term,
    // add/update fragment definition.
    virtual bool takeword(const std::string& term, int pos, int bts, int bte) {
        LOGDEB2("takeword: " << term << endl);

        // Recent past
        m_prevterms.push_back(pair<int,int>(bts,bte));
        if (m_prevterms.size() > m_ctxwords+1) {
            m_prevterms.pop_front();
        }

        string dumb;
        if (o_index_stripchars) {
            if (!unacmaybefold(term, dumb, "UTF-8", UNACOP_UNACFOLD)) {
                LOGINFO("abstract: unac failed for [" << term << "]\n");
                return true;
            }
        } else {
            dumb = term;
        }

        if (m_terms.find(dumb) != m_terms.end()) {
            // This word is a search term. Extend or create fragment
            LOGDEB2("match: [" << dumb << "] current: " << m_curfrag.first <<
                   ", " << m_curfrag.second << " remain " <<
                   m_remainingWords << endl);
            double coef = m_wordcoefs[dumb];
            if (!m_remainingWords) {
                // No current fragment
                m_curhitpos = baseTextPosition + pos;
                m_curfrag.first = m_prevterms.front().first;
                m_curfrag.second = m_prevterms.back().second;
                m_curhlzones.push_back(pair<int,int>(bts, bte));
                m_curterm = term;
                m_curtermcoef = coef;
            } else {
                LOGDEB2("Extending current fragment: " << m_remainingWords <<
                       " -> " << m_ctxwords << endl);
                m_extcount++;
                if (m_prevwordhit) {
                    m_curhlzones.back().second = bte;
                } else {
                    m_curhlzones.push_back(pair<int,int>(bts, bte));
                }
                if (coef > m_curtermcoef) {
                    m_curterm = term;
                    m_curtermcoef = coef;
                }
            }
            m_prevwordhit = true;
            m_curfragcoef += coef;
            m_remainingWords = m_ctxwords + 1;
            if (m_extcount > 3) {
                // Limit expansion of contiguous fragments (this is to
                // avoid common terms in search causing long
                // heavyweight meaningless fragments. Also, limit length).
                m_remainingWords = 1;
                m_extcount = 0;
            }
        } else {
            m_prevwordhit = false;
        }
       
        
        if (m_remainingWords) {
            // Fragment currently open. Time to close ?
            m_remainingWords--;
            m_curfrag.second = bte;
            if (m_remainingWords == 0) {
                if (m_totalcoef < 5.0 || m_curfragcoef >= 1.0) {
                    // Don't push bad fragments if we have a lot already
                    m_fragments.push_back(MatchEntry(m_curfrag.first,
                                                     m_curfrag.second,
                                                     m_curfragcoef,
                                                     m_curhlzones,
                                                     m_curhitpos,
                                                     m_curterm
                                              ));
                }
                m_totalcoef += m_curfragcoef;
                m_curfragcoef = 0.0;
                m_curtermcoef = 0.0;
            }
        }
        return true;
    }
    const vector<MatchEntry>& getFragments() {
        return m_fragments;
    }

private:
    // Past terms because we need to go back for context before a hit
    deque<pair<int,int>>  m_prevterms;
    // Data about the fragment we are building
    pair<int,int> m_curfrag{0,0};
    double m_curfragcoef{0.0};
    unsigned int m_remainingWords{0};
    unsigned int m_extcount{0};
    vector<pair<int,int>> m_curhlzones;
    bool m_prevwordhit{false};
    // Current sum of fragment weights
    double m_totalcoef{0.0};
    // Position of 1st term match (for page number computations)
    unsigned int m_curhitpos{0};
    // "best" term
    string m_curterm;
    double m_curtermcoef{0.0};
    
    // Input
    set<string> m_terms;
    unordered_map<string, double>& m_wordcoefs;
    unsigned int m_ctxwords;

    // Result: begin and end byte positions of query terms/groups in text
    vector<MatchEntry> m_fragments;  
};

int Query::Native::abstractFromText(
    Rcl::Db::Native *ndb,
    Xapian::docid docid,
    const vector<string>& matchTerms,
    const multimap<double, vector<string>> byQ,
    double totalweight,
    int ctxwords,
    unsigned int maxtotaloccs,
    vector<Snippet>& vabs,
    Chrono&
    )
{
    Xapian::Database& xrdb(ndb->xrdb);
    Xapian::Document xdoc;

    string reason;
    XAPTRY(xdoc = xrdb.get_document(docid), xrdb, reason);
    if (!reason.empty()) {
        LOGERR("abstractFromText: could not get doc: " << reason << endl);
        return ABSRES_ERROR;
    }

    string rawtext, data;
#ifdef RAWTEXT_IN_DATA
    XAPTRY(data = xdoc.get_data(), xrdb, reason);
    if (!reason.empty()) {
        LOGERR("abstractFromText: could not get data: " << reason << endl);
        return ABSRES_ERROR;
    }
    Doc doc;
    if (ndb->dbDataToRclDoc(docid, data, doc)) {
        rawtext = doc.meta["RAWTEXT"];
    }
#endif
#ifdef RAWTEXT_IN_VALUE
    XAPTRY(rawtext = xdoc.get_value(VALUE_RAWTEXT), xrdb, reason);
    if (!reason.empty()) {
        LOGERR("abstractFromText: could not get value: " << reason << endl);
        return ABSRES_ERROR;
    }
    ZLibUtBuf cbuf;
    inflateToBuf(rawtext.c_str(), rawtext.size(), cbuf);
    rawtext.assign(cbuf.getBuf(), cbuf.getCnt());
#endif

    if (rawtext.empty()) {
        LOGDEB0("abstractFromText: no text\n");
        return ABSRES_ERROR;
    }

    // tryout the xapian internal method.
#if 0 && ! (XAPIAN_MAJOR_VERSION <= 1 && XAPIAN_MINOR_VERSION <= 2)  && \
    (defined(RAWTEXT_IN_DATA) || defined(RAWTEXT_IN_VALUE))
    string snippet = xmset.snippet(rawtext);
    LOGDEB("SNIPPET: [" << snippet << "] END SNIPPET\n");
#endif

    // We need the q coefs for individual terms
    unordered_map<string, double> wordcoefs;
    for (const auto& mment : byQ) {
        for (const auto& word : mment.second) {
            wordcoefs[word] = mment.first;
        }
    }
    TextSplitABS splitter(matchTerms, wordcoefs, ctxwords,
                          TextSplit::TXTS_ONLYSPANS);
    splitter.text_to_words(rawtext);
    const vector<TextSplitABS::MatchEntry>& res1 = splitter.getFragments();
    vector<TextSplitABS::MatchEntry> result(res1.begin(), res1.end());
    std::sort(result.begin(), result.end(),
              [](const TextSplitABS::MatchEntry& a,
                 const TextSplitABS::MatchEntry& b) -> bool { 
                  return a.coef > b.coef; 
              }
        );

    static const string cstr_nc("\n\r\x0c\\");
    vector<int> vpbreaks;
    ndb->getPagePositions(docid, vpbreaks);
    unsigned int count = 0;
    for (const auto& entry : result) {
        string frag = neutchars(
            rawtext.substr(entry.start, entry.stop - entry.start), cstr_nc);
#if 0
        static const string starthit("<span style='color: blue;'>");
        static const string endhit("</span>");
        size_t inslen = 0;
        for (const auto& hlzone: entry.hlzones) {
            frag.replace(hlzone.first - entry.start + inslen, 0, starthit);
            inslen += starthit.size();
            frag.replace(hlzone.second - entry.start + inslen, 0, endhit);
            inslen += endhit.size();
        }
#endif
        LOGDEB("=== FRAGMENT: Coef: " << entry.coef << ": " << frag << endl);
        int page = ndb->getPageNumberForPosition(vpbreaks, entry.hitpos);
        vabs.push_back(Snippet(page, frag).setTerm(entry.term));
        if (count++ >= maxtotaloccs)
            break;
    }
    return ABSRES_OK;
}

}
