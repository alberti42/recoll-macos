/* Copyright (C) 2006 J.F.Dockes
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

// Handle translation from rcl's SearchData structures to XML. Used for
// complex search history storage in the GUI

#include "autoconfig.h"

#include <stdio.h>

#include <string>
#include <vector>
#include <sstream>

#include "searchdata.h"
#include "log.h"
#include "base64.h"

using namespace std;

namespace Rcl {

static string tpToString(SClType tp)
{
    switch (tp) {
    case SCLT_AND: return "AND";
    case SCLT_OR: return "OR";
    case SCLT_FILENAME: return "FN";
    case SCLT_PHRASE: return "PH";
    case SCLT_NEAR: return "NE";
    case SCLT_RANGE: return "RG";
    case SCLT_SUB: return "SU"; // Unsupported actually
    default: return "UN";
    }
}

string SearchData::asXML()
{
    LOGDEB("SearchData::asXML\n" );
    ostringstream os;

    // Searchdata
    os << "<SD>" << "\n";

    // Clause list
    os << "<CL>" << "\n";

    // List conjunction: default is AND, else print it.
    if (m_tp != SCLT_AND)
        os << "<CLT>" << tpToString(m_tp) << "</CLT>" << "\n";

    for (unsigned int i = 0; i <  m_query.size(); i++) {
        SearchDataClause *c = m_query[i];
        if (c->getTp() == SCLT_SUB) {
            LOGERR("SearchData::asXML: can't do subclauses !\n" );
            continue;
        }
        if (c->getTp() == SCLT_PATH) {
            // Keep these apart, for compat with the older history format. NEG
            // is ignored here, we have 2 different tags instead.
            SearchDataClausePath *cl = dynamic_cast<SearchDataClausePath*>(c);
            if (cl->getexclude()) {
                os << "<ND>" << base64_encode(cl->gettext()) << "</ND>" << "\n";
            } else {
                os << "<YD>" << base64_encode(cl->gettext()) << "</YD>" << "\n";
            }
            continue;
        } else {

            os << "<C>" << "\n";

            if (c->getexclude())
                os << "<NEG/>" << "\n";

            if (c->getTp() != SCLT_AND) {
                os << "<CT>" << tpToString(c->getTp()) << "</CT>" << "\n";
            }
            if (c->getTp() == SCLT_FILENAME) {
                SearchDataClauseFilename *cl = dynamic_cast<SearchDataClauseFilename*>(c);
                os << "<T>" << base64_encode(cl->gettext()) << "</T>" << "\n";
            } else {
                SearchDataClauseSimple *cl = dynamic_cast<SearchDataClauseSimple*>(c);
                if (!cl->getfield().empty()) {
                    os << "<F>" << base64_encode(cl->getfield()) << "</F>" << "\n";
                }
                os << "<T>" << base64_encode(cl->gettext()) << "</T>" << "\n";
                if (cl->getTp() == SCLT_RANGE) {
                    SearchDataClauseRange *clr = dynamic_cast<SearchDataClauseRange*>(cl);
                    const string& t = clr->gettext2();
                    if (!t.empty()) {
                        os << "<T2>" << base64_encode(clr->gettext2()) << "</T2>" << "\n";
                    }
                }
                if (cl->getTp() == SCLT_NEAR || cl->getTp() == SCLT_PHRASE) {
                    SearchDataClauseDist *cld = dynamic_cast<SearchDataClauseDist*>(cl);
                    os << "<S>" << cld->getslack() << "</S>" << "\n";
                }
            }
            os << "</C>" << "\n";
        }
    }
    os << "</CL>" << "\n";

    if (m_haveDates) {
        if (m_dates.y1 > 0) {
            os << "<DMI>" << 
                "<D>" << m_dates.d1 << "</D>" <<
                "<M>" << m_dates.m1 << "</M>" << 
                "<Y>" << m_dates.y1 << "</Y>" 
               << "</DMI>" << "\n";
        }
        if (m_dates.y2 > 0) {
            os << "<DMA>" << 
                "<D>" << m_dates.d2 << "</D>" <<
                "<M>" << m_dates.m2 << "</M>" << 
                "<Y>" << m_dates.y2 << "</Y>" 
               << "</DMA>" << "\n";
        }
    }

    if (m_minSize != -1) {
        os << "<MIS>" << m_minSize << "</MIS>" << "\n";
    }
    if (m_maxSize != -1) {
        os << "<MAS>" << m_maxSize << "</MAS>" << "\n";
    }

    if (!m_filetypes.empty()) {
        os << "<ST>";
        for (const auto& ft : m_filetypes) {
            os << ft << " ";
        }
        os << "</ST>" << "\n";
    }

    if (!m_nfiletypes.empty()) {
        os << "<IT>";
        for (const auto& nft : m_nfiletypes) {
            os << nft << " ";
        }
        os << "</IT>" << "\n";
    }

    os << "</SD>";
    return os.str();
}


}

