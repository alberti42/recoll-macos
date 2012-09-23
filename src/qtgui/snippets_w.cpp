/* Copyright (C) 2012 J.F.Dockes
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

#include <unistd.h>
#include <stdio.h>

#include <string>
#include <vector>
using namespace std;

#include "debuglog.h"
#include "recoll.h"
#include "snippets_w.h"
#include "guiutils.h"
#include "rcldb.h"
#include "rclhelp.h"
#include "plaintorich.h"

class PlainToRichQtSnippets : public PlainToRich {
public:
    virtual string startMatch(unsigned int)
    {
	return string("<span class='rclmatch' style='color: ")
	    + string((const char *)prefs.qtermcolor.toAscii()) + string("'>");
    }
    virtual string endMatch() 
    {
	return string("</span>");
    }
};
static PlainToRichQtSnippets g_hiliter;

void SnippetsW::init()
{
    if (m_source.isNull())
	return;

    // Make title out of file name if none yet
    string titleOrFilename;
    string utf8fn;
    m_doc.getmeta(Rcl::Doc::keytt, &titleOrFilename);
    m_doc.getmeta(Rcl::Doc::keyfn, &utf8fn);
    if (titleOrFilename.empty()) {
	titleOrFilename = utf8fn;
    }

    setWindowTitle(QString::fromUtf8(titleOrFilename.c_str()));

    vector<pair<int, string> > vpabs;
    m_source->getAbstract(m_doc, vpabs);

    HighlightData hdata;
    m_source->getTerms(hdata);

    QString html = QString::fromAscii(
	"<html><head>"
	"<meta http-equiv=\"content-type\" "
	"content=\"text/html; charset=utf-8\"></head>"
	"<body style='overflow-x: scroll; white-space: nowrap'>"
	"<table>"
				      );

    g_hiliter.set_inputhtml(false);

    for (vector<pair<int, string> >::const_iterator it = vpabs.begin(); 
	 it != vpabs.end(); it++) {
	html += "<tr><td>";
	if (it->first > 0) {
	    char buf[100];
	    sprintf(buf, "P.&nbsp;%d", it->first);
	    html += "<a href=\"";
	    html += buf;
	    html += "\">";
	    html += buf;
	    html += "</a>";
	}
	html += "</td><td>";
	list<string> lr;
	g_hiliter.plaintorich(it->second, lr, hdata);
	html.append(QString::fromUtf8(lr.front().c_str()));
	html.append("</td></tr>\n");
    }
    html.append("</body></html>");
    webView->setHtml(html);
    connect(webView, SIGNAL(linkClicked(const QUrl &)), 
	    this, SLOT(linkWasClicked(const QUrl &)));
    webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
}


void SnippetsW::linkWasClicked(const QUrl &url)
{
    string ascurl = (const char *)url.toString().toAscii();;
    LOGDEB(("Snippets::linkWasClicked: [%s]\n", ascurl.c_str()));

    if (ascurl.size() > 3) {
	int what = ascurl[0];
	switch (what) {
	case 'P': 
	{
	    int page = atoi(ascurl.c_str()+2);
	    emit startNativeViewer(m_doc, page);
	    return;
	}
	}
    }
    LOGERR(("Snippets::linkWasClicked: bad link [%s]\n", ascurl.c_str()));
}

