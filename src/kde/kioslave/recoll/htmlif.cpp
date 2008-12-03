#ifndef lint
static char rcsid[] = "@(#$Id: htmlif.cpp,v 1.6 2008-12-03 17:04:20 dockes Exp $ (C) 2005 J.F.Dockes";
#endif
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h> 

#include <string>

using namespace std;

#include <kdebug.h>
#include <kstandarddirs.h>

#include "rclconfig.h"
#include "rcldb.h"
#include "rclinit.h"
#include "pathut.h"
#include "searchdata.h"
#include "rclquery.h"
#include "wasastringtoquery.h"
#include "wasatorcl.h"
#include "kio_recoll.h"
#include "docseqdb.h"
#include "readfile.h"
#include "smallut.h"

using namespace KIO;

bool RecollKioPager::append(const string& data)
{
    if (!m_parent) 
	return false;
    m_parent->data(QByteArray(data.c_str()));
    return true;
}

string RecollProtocol::makeQueryUrl(int page, bool isdet)
{
    char buf[100];
    sprintf(buf, "recoll://search/query?q=%s&qtp=%s&p=%d", 
	    (const char*)m_query.query.toUtf8(), 
	    (const char*)m_query.opt.toUtf8(), 
	    page);
    string ret(buf);
    if (isdet)
	ret += "&det=1";
    return ret;
}

string RecollKioPager::detailsLink()
{
    string chunk = string("<a href=\"") + 
	m_parent->makeQueryUrl(m_parent->m_pager.pageNumber(), true) + "\">"
	+ "(show query)" + "</a>";
    return chunk;
}

const static string parformat = 
    "<a href=\"%U\"><img src=\"%I\" align=\"left\"></a>"
    "%R %S "
    "<a href=\"%U\">Open</a>&nbsp;&nbsp;<b>%T</b><br>"
    "%M&nbsp;%D&nbsp;&nbsp; <i>%U</i><br>"
    "%A %K";
const string& RecollKioPager::parFormat()
{
    return parformat;
}

string RecollKioPager::pageTop()
{
    return "<p align=\"center\"><a href=\"recoll:///search.html\">New Search</a></p>";
}

string RecollKioPager::nextUrl()
{
    int pagenum = pageNumber();
    if (pagenum < 0)
	pagenum = 0;
    else
	pagenum++;
    return m_parent->makeQueryUrl(pagenum);
}

string RecollKioPager::prevUrl()
{
    int pagenum = pageNumber();
    if (pagenum <= 0)
	pagenum = 0;
    else
	pagenum--;
    return m_parent->makeQueryUrl(pagenum);
}

static string welcomedata;

void RecollProtocol::searchPage()
{
    kDebug();
    mimeType("text/html");
    if (welcomedata.empty()) {
	QString location = 
	    KStandardDirs::locate("data", "kio_recoll/welcome.html");
	string reason;
	if (location.isEmpty() || 
	    !file_to_string((const char *)location.toUtf8(), 
			    welcomedata, &reason)) {
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
	for (list<string>::iterator it = cats.begin(); it != cats.end();it++) {
	    catgq += "\n<input type=\"radio\" name=\"ct\" value=\"" +
		*it + "\">" + *it ;
	}
    }
#endif 

    string tmp;
    map<char, string> subs;
    subs['Q'] = "";
    subs['C'] = catgq;
    subs['S'] = "";
    pcSubst(welcomedata, tmp, subs);
    data(tmp.c_str());
}

void RecollProtocol::queryDetails()
{
    kDebug();
    mimeType("text/html");
    QByteArray array;
    QTextStream os(&array, QIODevice::WriteOnly);

    os << "<html><head>" << endl;
    os << "<meta http-equiv=\"Content-Type\" content=\"text/html;"
	"charset=utf-8\">" << endl;
    os << "<title>" << "Recoll query details" << "</title>\n" << endl;
    os << "</head>" << endl;
    os << "<body><h3>Query details:</h3>" << endl;
    os << "<p>" << m_pager.queryDescription().c_str() <<"</p>"<< endl;
    os << "<p><a href=\"" << makeQueryUrl(m_pager.pageNumber()).c_str() << 
	"\">Return to results</a>" << endl;
    os << "</body></html>" << endl;
    data(array);
}

void RecollProtocol::htmlDoSearch(const QueryDesc& qd)
{
    kDebug() << "q" << qd.query << "opt" << qd.opt << "page" << qd.page <<
	"isdet" << qd.isDetReq;

    mimeType("text/html");

    bool samesearch;
    if (!syncSearch(qd, &samesearch))
	return;
    if (!samesearch) {
	m_pager.setDocSource(m_source);
	m_pager.resultPageNext();
    }
    if (qd.isDetReq) {
	queryDetails();
	return;
    }

    // Check / adjust page number
    if (qd.page > m_pager.pageNumber()) {
	int npages = qd.page - m_pager.pageNumber();
	for (int i = 0; i < npages; i++)
	    m_pager.resultPageNext();
    } else if (qd.page < m_pager.pageNumber()) {
	int npages = m_pager.pageNumber() - qd.page;
	for (int i = 0; i < npages; i++) 
	    m_pager.resultPageBack();
    }
    // Display
    m_pager.displayPage();
}
