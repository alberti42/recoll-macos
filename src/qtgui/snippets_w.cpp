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

    vector<Rcl::Snippet> vpabs;
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

    for (vector<Rcl::Snippet>::const_iterator it = vpabs.begin(); 
	 it != vpabs.end(); it++) {
	html += "<tr><td>";
	if (it->page > 0) {
	    char txt[100];
	    sprintf(txt, "P.&nbsp;%d", it->page);
	    char url[100];
	    sprintf(url, "P%dT%s", it->page, it->term.c_str());
	    html += "<a href=\"";
	    html += url;
	    html += "\">";
	    html += txt;
	    html += "</a>";
	}
	html += "</td><td>";
	list<string> lr;
	g_hiliter.plaintorich(it->snippet, lr, hdata);
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
    string ascurl = (const char *)url.toString().toAscii();
    LOGDEB(("Snippets::linkWasClicked: [%s]\n", ascurl.c_str()));

    if (ascurl.size() > 3) {
	int what = ascurl[0];
	switch (what) {
	case 'P': 
	{
	    string::size_type numpos = ascurl.find_first_of("0123456789");
	    if (numpos == string::npos)
		return;
	    int page = atoi(ascurl.c_str() + numpos);
	    string::size_type termpos = ascurl.find_first_of("T");
	    string term;
	    if (termpos != string::npos)
		term = ascurl.substr(termpos+1);
	    emit startNativeViewer(m_doc, page, 
				   QString::fromUtf8(term.c_str()));
	    return;
	}
	}
    }
    LOGERR(("Snippets::linkWasClicked: bad link [%s]\n", ascurl.c_str()));
}

