/* Copyright (C) 2006-2024 J.F.Dockes
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
#include "picoxml.h"

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

class SDHXMLHandler : public PicoXMLParser {
public:
    SDHXMLHandler(const std::string& in)
        : PicoXMLParser(in) {
        resetTemps();
    }
    void startElement(
        const std::string& nm,
        const std::map<std::string, std::string>& attrs) {

        LOGDEB2("SDHXMLHandler::startElement: name [" << nm << "]\n");
        if (nm == "SD") {
            // Advanced search history entries have no type. So we're good
            // either if type is absent, or if it's searchdata
            auto attr = attrs.find("type");
            if (attr != attrs.end() && attr->second != "searchdata") {
                LOGDEB("XMLTOSD: bad type: " << attr->second << endl);
                contentsOk = false;
                return;
            }
            resetTemps();
            // A new search descriptor. Allocate data structure
            sd = std::make_shared<SearchData>();
            if (!sd) {
                LOGERR("SDHXMLHandler::startElement: out of memory\n");
                contentsOk = false;
                return;
            }
        }    
        return;
    }

    void endElement(const string & nm) {
        LOGDEB2("SDHXMLHandler::endElement: name [" << nm << "]\n");
        string curtxt{currentText};
        trimstring(curtxt, " \t\n\r");
        if (nm == "CLT") {
            if (curtxt == "OR") {
                sd->setTp(SCLT_OR);
            }
        } else if (nm == "CT") {
            whatclause = curtxt;
        } else if (nm == "NEG") {
            exclude = true;
        } else if (nm == "F") {
            field = base64_decode(curtxt);
        } else if (nm == "T") {
            text = base64_decode(curtxt);
        } else if (nm == "T2") {
            text2 = base64_decode(curtxt);
        } else if (nm == "S") {
            slack = atoi(curtxt.c_str());
        } else if (nm == "C") {
            SearchDataClause *c;
            if (whatclause == "AND" || whatclause.empty()) {
                c = new SearchDataClauseSimple(SCLT_AND, text, field);
                c->setexclude(exclude);
            } else if (whatclause == "OR") {
                c = new SearchDataClauseSimple(SCLT_OR, text, field);
                c->setexclude(exclude);
            } else if (whatclause == "RG") {
                c = new SearchDataClauseRange(text, text2, field);
                c->setexclude(exclude);
            } else if (whatclause == "EX") {
                // Compat with old hist. We don't generete EX
                // (SCLT_EXCL) anymore it's replaced with OR + exclude
                // flag
                c = new SearchDataClauseSimple(SCLT_OR, text, field);
                c->setexclude(true);
            } else if (whatclause == "FN") {
                c = new SearchDataClauseFilename(text);
                c->setexclude(exclude);
            } else if (whatclause == "PH") {
                c = new SearchDataClauseDist(SCLT_PHRASE, text, slack, field);
                c->setexclude(exclude);
            } else if (whatclause == "NE") {
                c = new SearchDataClauseDist(SCLT_NEAR, text, slack, field);
                c->setexclude(exclude);
            } else {
                LOGERR("Bad clause type ["  << whatclause << "]\n");
                contentsOk = false;
                return;
            }
            sd->addClause(c);
            whatclause = "";
            text.clear();
            field.clear();
            slack = 0;
            exclude = false;
        } else if (nm == "D") {
            d = atoi(curtxt.c_str());
        } else if (nm == "M") {
            m = atoi(curtxt.c_str());
        } else if (nm == "Y") {
            y = atoi(curtxt.c_str());
        } else if (nm == "DMI") {
            di.d1 = d;
            di.m1 = m;
            di.y1 = y;
            hasdates = true;
        } else if (nm == "DMA") {
            di.d2 = d;
            di.m2 = m;
            di.y2 = y;
            hasdates = true;
        } else if (nm == "MIS") {
            sd->setMinSize(atoll(curtxt.c_str()));
        } else if (nm == "MAS") {
            sd->setMaxSize(atoll(curtxt.c_str()));
        } else if (nm == "ST") {
            string types = curtxt.c_str();
            vector<string> vt;
            stringToTokens(types, vt);
            for (unsigned int i = 0; i < vt.size(); i++) 
                sd->addFiletype(vt[i]);
        } else if (nm == "IT") {
            vector<string> vt;
            stringToTokens(curtxt, vt);
            for (unsigned int i = 0; i < vt.size(); i++) 
                sd->remFiletype(vt[i]);
        } else if (nm == "YD") {
            string d;
            base64_decode(curtxt, d);
            sd->addClause(new SearchDataClausePath(d));
        } else if (nm == "ND") {
            string d;
            base64_decode(curtxt, d);
            sd->addClause(new SearchDataClausePath(d, true));
        } else if (nm == "SD") {
            // Closing current search descriptor. Finishing touches...
            if (hasdates)
                sd->setDateSpan(&di);
            resetTemps();
            isvalid = contentsOk;
        } 
        currentText.clear();
        return;
    }

    void characterData(const std::string &str) {
        currentText += str;
    }

    // The object we set up
    std::shared_ptr<SearchData> sd;
    bool isvalid{false};
    bool contentsOk{true};
    
private:
    void resetTemps() {
        currentText = whatclause = "";
        text.clear();
        text2.clear();
        field.clear();
        slack = 0;
        d = m = y = di.d1 = di.m1 = di.y1 = di.d2 = di.m2 = di.y2 = 0;
        hasdates = false;
        exclude = false;
    }

    // Temporary data while parsing.
    std::string currentText;
    std::string whatclause;
    std::string field, text, text2;
    int slack;
    int d, m, y;
    DateInterval di;
    bool hasdates;
    bool exclude;
};

std::shared_ptr<Rcl::SearchData> SearchData::fromXML(const string& xml, bool verbose)
{
    SDHXMLHandler handler(xml);
    if (!handler.Parse() || !handler.isvalid) {
        if (verbose) {
            LOGERR("SearchData::fromXML: parse failed for ["  << xml << "]\n");
        }
        return std::shared_ptr<SearchData>();
    }
    return handler.sd;
}

} // namespace Rcl
