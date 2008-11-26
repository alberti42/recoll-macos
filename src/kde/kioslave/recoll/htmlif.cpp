#ifndef lint
static char rcsid[] = "@(#$Id: htmlif.cpp,v 1.1 2008-11-26 15:03:41 dockes Exp $ (C) 2005 J.F.Dockes";
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
    if (!m_parent) return false;
    m_parent->data(QByteArray(data.c_str()));
    return true;
}

string RecollKioPager::detailsLink()
{
    string chunk = "<a href=\"recoll://command/QueryDetails\">";
    chunk += tr("(show query)") + "</a>";
    return chunk;
}

const static string parformat = 
    "<a href=\"%U\"><img src=\"%I\" align=\"left\"></a>"
    "%R %S "
    "<a href=\"%U\">Open</a>&nbsp;&nbsp;<b>%T</b><br>"
    "%M&nbsp;%D&nbsp;&nbsp; <i>%U</i><br>"
    "%A %K";
const string &RecollKioPager::parFormat()
{
    return parformat;
}

string RecollKioPager::pageTop()
{
    return "<p align=\"center\"><a href=\"recoll:///\">New Search</a></p>";
}

string RecollKioPager::nextUrl()
{
    int pagenum = pageNumber();
    if (pagenum < 0)
	pagenum = 0;
    else
	pagenum++;
    char buf[100];
    sprintf(buf, "recoll://command/Page%d", pagenum);
    return buf;
}
string RecollKioPager::prevUrl()
{
    int pagenum = pageNumber();
    if (pagenum <= 0)
	pagenum = 0;
    else
	pagenum--;
    char buf[100];
    sprintf(buf, "recoll://command/Page%d", pagenum);
    return buf;
}

static string welcomedata;

void RecollProtocol::welcomePage()
{
    kDebug() << endl;
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
    string tmp;
    map<char, string> subs;
    subs['Q'] = "";
    pcSubst(welcomedata, tmp, subs);
    data(tmp.c_str());
    kDebug() << "done" << endl;
}

void RecollProtocol::queryDetails()
{
    kDebug() << endl;
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
    os << "<p><a href=\"recoll://command/Page" << 
	m_pager.pageNumber() << "\">Return to results</a>" << endl;
    os << "</body></html>" << endl;
    data(array);
    kDebug() << "done" << endl;
}

void RecollProtocol::htmlDoSearch(const QString& q, char opt)
{
    kDebug() << endl;
    mimeType("text/html");
    doSearch(q, opt);
    m_pager.setDocSource(m_source);
    m_pager.resultPageNext();
    m_pager.displayPage();
    kDebug() << "done" << endl;
}

void RecollProtocol::outputError(const QString& errmsg)
{
    mimeType("text/html");
    QByteArray array;
    QTextStream os(&array, QIODevice::WriteOnly);

    os << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Strict//EN\">" << endl;
    os << "<html><head>" << endl;
    os << "<meta http-equiv=\"Content-Type\" content=\"text/html;"
	"charset=utf-8\">" << endl;
    os << "<title>" << "Recoll error" << "</title>\n" << endl;
    os << "</head>" << endl;
    os << "<body><p><b>Recoll error: </b>" << errmsg << "</p>" << endl;
    os << "<p><a href=\"recoll:///\">New query</a></p>"<< endl;
    os << "</body></html>" << endl;
    data(array);
}
