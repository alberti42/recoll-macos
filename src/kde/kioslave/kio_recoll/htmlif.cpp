/* Copyright (C) 2005 J.F.Dockes
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
#undef QT_NO_CAST_FROM_ASCII

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <string>

#include <QStandardPaths>

#include "plaintorich.h"
#include "rclconfig.h"
#include "rclinit.h"
#include "internfile.h"
#include "docseqdb.h"
#include "wasatorcl.h"
#include "rcldb.h"
#include "rclquery.h"
#include "searchdata.h"
#include "hldata.h"
#include "pathut.h"
#include "readfile.h"
#include "smallut.h"

#include "kio_recoll.h"

using namespace std;
using namespace KIO;

bool RecollKioPager::append(const string& data)
{
    if (!m_parent) {
        return false;
    }
    m_parent->data(QByteArray(data.c_str()));
    return true;
}
#include <sstream>
string RecollProtocol::makeQueryUrl(int page, bool isdet)
{
    ostringstream str;
    str << "hrecoll://search/query?q=" <<
        path_pcencode((const char*)m_query.query.toUtf8().data()) <<
        "&qtp=" << (const char*)m_query.opt.toUtf8().data();
    if (page >= 0) {
        str << "&p=" << page;
    }
    if (isdet) {
        str << "&det=1";
    }
    return str.str();
}

string RecollKioPager::detailsLink()
{
    string chunk = string("<a href=\"") +
                   m_parent->makeQueryUrl(m_parent->m_pager->pageNumber(), true) + "\">"
                   + "(show query)" + "</a>";
    return chunk;
}

static string parformat;
const string& RecollKioPager::parFormat()
{
    // Need to escape the % inside the query url
    string qurl = m_parent->makeQueryUrl(-1, false), escurl;
    for (string::size_type pos = 0; pos < qurl.length(); pos++) {
        switch (qurl.at(pos)) {
        case '%':
            escurl += "%%";
            break;
        default:
            escurl += qurl.at(pos);
        }
    }

    ostringstream str;
    str <<
        "<a href=\"%U\"><img src=\"%I\" align=\"left\"></a>"
        "%R %S "
        "<a href=\"" << escurl << "&cmd=pv&dn=%N\">Preview</a>&nbsp;&nbsp;" <<
        "<a href=\"%U\">Open</a> " <<
        "<b>%T</b><br>"
        "%M&nbsp;%D&nbsp;&nbsp; <i>%U</i>&nbsp;&nbsp;%i<br>"
        "%A %K";
    return parformat = str.str();
}

string RecollKioPager::pageTop()
{
    string pt = "<p align=\"center\"> <a href=\"hrecoll:///search.html?q=";
    pt += path_pcencode(string(m_parent->m_query.query.toUtf8().data()));
    pt += "\">New Search</a>";
    return pt;
// Would be nice to have but doesnt work because the query may be executed
// by another kio instance which has no idea of the current page o
#if 0
    " &nbsp;&nbsp;&nbsp;<a href=\"hrecoll:///" +
        path_pcencode(string(m_parent->m_query.query.toUtf8().data())) +
    "/\">Directory view</a> (you may need to reload the page)"
#endif
}

string RecollKioPager::nextUrl()
{
    int pagenum = pageNumber();
    if (pagenum < 0) {
        pagenum = 0;
    } else {
        pagenum++;
    }
    return m_parent->makeQueryUrl(pagenum);
}

string RecollKioPager::prevUrl()
{
    int pagenum = pageNumber();
    if (pagenum <= 0) {
        pagenum = 0;
    } else {
        pagenum--;
    }
    return m_parent->makeQueryUrl(pagenum);
}

static string welcomedata;

void RecollProtocol::searchPage()
{
    mimeType("text/html");
    if (welcomedata.empty()) {
        QString location =
            QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kio_recoll/welcome.html");
        string reason;
        if (location.isEmpty() ||
            !file_to_string((const char *)location.toUtf8().data(), welcomedata, &reason)) {
            welcomedata = "<html><head><title>Recoll Error</title></head>"
                          "<body><p>Could not locate Recoll welcome.html file: ";
            welcomedata += reason;
            welcomedata += "</p></body></html>";
        }
    }

    string catgq;
#if 0
    // Catg filtering. A bit complicated to do because of the
    // stateless thing (one more thing to compare to check if same
    // query) right now. Would be easy by adding to the query
    // language, but not too useful in this case, so scrap it for now.
    list<string> cats;
    if (o_rclconfig->getMimeCategories(cats) && !cats.empty()) {
        catgq = "<p>Filter on types: "
                "<input type=\"radio\" name=\"ct\" value=\"All\" checked>All";
        for (list<string>::iterator it = cats.begin(); it != cats.end(); it++) {
            catgq += "\n<input type=\"radio\" name=\"ct\" value=\"" +
                     *it + "\">" + *it ;
        }
    }
#endif

    string tmp;
    map<char, string> subs;
    subs['Q'] = (const char *)m_query.query.toUtf8().data();
    subs['C'] = catgq;
    subs['S'] = "";
    pcSubst(welcomedata, tmp, subs);
    data(tmp.c_str());
}

void RecollProtocol::queryDetails()
{
    mimeType("text/html");
    QByteArray array;
    QTextStream os(&array, QIODevice::WriteOnly);

    os << "<html><head>" << "\n";
    os << "<meta http-equiv=\"Content-Type\" content=\"text/html;"
       "charset=utf-8\">" << "\n";
    os << "<title>" << "Recoll query details" << "</title>\n" << "\n";
    os << "</head>" << "\n";
    os << "<body><h3>Query details:</h3>" << "\n";
    os << "<p>" << m_pager->queryDescription().c_str() << "</p>" << "\n";
    os << "<p><a href=\"" << makeQueryUrl(m_pager->pageNumber()).c_str() <<
       "\">Return to results</a>" << "\n";
    os << "</body></html>" << "\n";
    data(array);
}

class PlainToRichKio : public PlainToRich {
public:
    PlainToRichKio(const string& nm)
        : m_name(nm) {
    }

    virtual string header() override {
        if (m_inputhtml) {
            return std::string();
        } else {
            return string("<html><head>"
                          "<META http-equiv=\"Content-Type\""
                          "content=\"text/html;charset=UTF-8\"><title>").
                   append(m_name).
                   append("</title></head><body><pre>");
        }
    }

    virtual string startMatch(unsigned int) override {
        return string("<font color=\"blue\">");
    }

    virtual string endMatch() override {
        return string("</font>");
    }

    const string& m_name;
};

void RecollProtocol::showPreview(const Rcl::Doc& idoc)
{
    FileInterner interner(idoc, o_rclconfig, FileInterner::FIF_forPreview);
    Rcl::Doc fdoc;
    string ipath = idoc.ipath;
    if (!interner.internfile(fdoc, ipath)) {
#if KIO_VERSION < KIO_WORKER_SWITCH_VERSION
        error(ERR_SLAVE_DEFINED, QString::fromUtf8("Cannot convert file to internal format"));
#endif
        return;
    }
    if (!interner.get_html().empty()) {
        fdoc.text = interner.get_html();
        fdoc.mimetype = "text/html";
    }

    mimeType("text/html");

    string fname =  path_getsimple(fdoc.url).c_str();
    PlainToRichKio ptr(fname);
    ptr.set_inputhtml(!fdoc.mimetype.compare("text/html"));
    list<string> otextlist;
    HighlightData hdata;
    if (m_source) {
        m_source->getTerms(hdata);
    }
    ptr.plaintorich(fdoc.text, otextlist, hdata);
    //qDebug() << "PREVIEW: got: " << fdoc.text.c_str();
    QByteArray out;
    for (const auto& chunk : otextlist) {
        out.append(chunk.c_str(), chunk.size());
    }
    out.append("</pre></body></html>");
    data(out);
}

void RecollProtocol::htmlDoSearch(const QueryDesc& qd)
{
    qDebug() << "q" << qd.query << "option" << qd.opt << "page" << qd.page <<
             "isdet" << qd.isDetReq << "\n";

    mimeType("text/html");

    if (!syncSearch(qd)) {
        return;
    }
    // syncSearch/doSearch do the setDocSource when needed
    if (m_pager->pageNumber() < 0) {
        m_pager->resultPageNext();
    }
    if (qd.isDetReq) {
        queryDetails();
        return;
    }

    // Check / adjust page number
    if (qd.page > m_pager->pageNumber()) {
        int npages = qd.page - m_pager->pageNumber();
        for (int i = 0; i < npages; i++) {
            m_pager->resultPageNext();
        }
    } else if (qd.page < m_pager->pageNumber()) {
        int npages = m_pager->pageNumber() - qd.page;
        for (int i = 0; i < npages; i++) {
            m_pager->resultPageBack();
        }
    }
    // Display
    m_pager->displayPage(o_rclconfig);
}
