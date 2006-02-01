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
/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/

#include <time.h>

#include <qtimer.h>
#include <qmessagebox.h>
#include <qimage.h>

#include "debuglog.h"
#include "recoll.h"
#include "guiutils.h"
#include "pathut.h"
#include "docseq.h"

#include "reslistb.h"

#ifndef MIN
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#endif

void ResListBase::init()
{
    m_winfirst = -1;
    m_mouseDrag = false;
    m_mouseDown = false;
    m_par = -1;
    m_car = -1;
    m_waitingdbl = false;
    m_dblclck = false;
    m_docsource = 0;
}

void ResListBase::destroy()
{
    if (m_docsource) 
	delete m_docsource;
}

// Acquire new docsource
void ResListBase::setDocSource(DocSequence *docsource)
{
    if (m_docsource)
	delete m_docsource;
    m_docsource = docsource;
    showResultPage();
}

bool ResListBase::getDoc(int docnum, Rcl::Doc &doc)
{
    if (docnum >= 0 && docnum >= int(m_winfirst) && 
	docnum < int(m_winfirst + m_curDocs.size())) {
	doc = m_curDocs[docnum - m_winfirst];
	return true;
    }
    return false;
}

// Get document number-in-window from paragraph number
int ResListBase::reldocnumfromparnum(int par)
{
    std::map<int,int>::iterator it = m_pageParaToReldocnums.find(par);
    int rdn;
    if (it != m_pageParaToReldocnums.end()) {
	rdn = it->second;
    } else {
	rdn = -1;
    }
    LOGDEB1(("reldocnumfromparnum: par %d reldoc %d\n", par, rdn));
    return rdn;
}

// Double click in result list
void ResListBase::doubleClicked(int par, int )
{
    LOGDEB(("ResListBase::doubleClicked: par %d\n", par));
    m_dblclck = true;
    int reldocnum =  reldocnumfromparnum(par);
    if (reldocnum < 0)
	return;
    emit docDoubleClicked(m_winfirst + reldocnum);
}

// Single click in result list: we don't actually do anything but
// start a timer because we want to check first if this might be a
// double click
void ResListBase::clicked(int par, int car)
{
    if (m_waitingdbl)
	return;
    LOGDEB(("ResListBase::clicked:wfirst %d par %d char %d drg %d\n", 
	    m_winfirst, par, car, m_mouseDrag));
    if (m_mouseDrag)
	return;

    // remember par and car
    m_par = par;
    m_car = car;
    m_waitingdbl = true;
    m_dblclck = false;
    // Wait to see if there's going to be a dblclck
    QTimer::singleShot(150, this, SLOT(delayedClick()) );
}


// This gets called by a timer 100mS after a single click in the
// result list. We don't want to start a preview if the user has
// requested a native viewer by double-clicking. If this was not actually
// a double-clik, we finally say it's a click, and change the active paragraph
void ResListBase::delayedClick()
{
    LOGDEB(("ResListBase::delayedClick:\n"));
    m_waitingdbl = false;
    if (m_dblclck) {
	LOGDEB1(("ResListBase::delayedclick: dbleclick\n"));
	m_dblclck = false;
	return;
    }

    int par = m_par;

    // Erase everything back to white
    {
	QColor color("white");
	for (int i = 1; i < reslistTE->paragraphs(); i++)
	    reslistTE->setParagraphBackgroundColor(i, color);
    }

    // Color the new active paragraph
    QColor color("lightblue");
    reslistTE->setParagraphBackgroundColor(par, color);

    // Document number
    int reldocnum = reldocnumfromparnum(par);

    if (reldocnum < 0) {
	emit headerClicked();
    } else {
	emit docClicked(m_winfirst + reldocnum);
    }
}


// Page Up/Down: we don't try to check if current paragraph is last or
// first. We just page up/down and check if viewport moved. If it did,
// fair enough, else we go to next/previous result page.
void ResListBase::resPageUpOrBack()
{
    int vpos = reslistTE->contentsY();
    reslistTE->moveCursor(QTextEdit::MovePgUp, false);
    if (vpos == reslistTE->contentsY())
	resultPageBack();
}


void ResListBase::resPageDownOrNext()
{
    int vpos = reslistTE->contentsY();
    reslistTE->moveCursor(QTextEdit::MovePgDown, false);
    LOGDEB(("ResListBase::resPageDownOrNext: vpos before %d, after %d\n",
	    vpos, reslistTE->contentsY()));
    if (vpos == reslistTE->contentsY()) 
	showResultPage();
}

// Show previous page of results. We just set the current number back
// 2 pages and show next page.
void ResListBase::resultPageBack()
{
    if (m_winfirst <= 0)
	return;
    m_winfirst -= 2 * prefs.respagesize;
    showResultPage();
}

static string displayableBytes(long size)
{
    char sizebuf[30];
    const char * unit = " B ";

    if (size > 1024 && size < 1024*1024) {
	unit = " KB ";
	size /= 1024;
    } else if (size  >= 1024*1204) {
	unit = " MB ";
	size /= (1024*1024);
    }
    sprintf(sizebuf, "%ld%s", size, unit);
    return string(sizebuf);
}

// Fill up result list window with next screen of hits
void ResListBase::showResultPage()
{
    if (!m_docsource)
	return;

    int percent;
    Rcl::Doc doc;

    int resCnt = m_docsource->getResCnt();

    LOGDEB(("showResultPage: rescnt %d, winfirst %d\n", resCnt,
	    m_winfirst));

    m_pageParaToReldocnums.clear();

    // If we are already on the last page, nothing to do:
    if (m_winfirst >= 0 && 
	(m_winfirst + prefs.respagesize > resCnt)) {
	emit nextPageAvailable(false);
	return;
    }

    if (m_winfirst < 0) {
	m_winfirst = 0;
	emit prevPageAvailable(false);
    } else {
	emit prevPageAvailable(true);
	m_winfirst += prefs.respagesize;
    }

    bool gotone = false;
    reslistTE->clear();

    int last = MIN(resCnt-m_winfirst, prefs.respagesize);

    m_curDocs.clear();
    // Insert results if any in result list window. We have to send
    // the text to the widgets, because we need the paragraph number
    // each time we add a result paragraph (its diffult and
    // error-prone to compute the paragraph numbers in parallel. We
    // would like to disable updates while we're doing this, but
    // couldn't find a way to make it work, the widget seems to become
    // confused if appended while updates are disabled
    //      reslistTE->setUpdatesEnabled(false);
    for (int i = 0; i < last; i++) {
	string sh;
	doc.erase();

	if (!m_docsource->getDoc(m_winfirst + i, doc, &percent, &sh)) {
	    // This may very well happen for history if the doc has
	    // been removed since. So don't treat it as fatal.
	    doc.abstract = string(tr("Unavailable document").utf8());
	}

	if (i == 0) {
	    // Display list header
	    // We could use a <title> but the textedit doesnt display
	    // it prominently
	    reslistTE->append("<qt><head></head><body>");
	    QString line = "<p><font size=+1><b>";
	    line += m_docsource->title().c_str();
	    line += "</b></font><br>";
	    reslistTE->append(line);
	    line = tr("<b>Displaying results starting at index"
		      " %1 (maximum set size %2)</b></p>\n")
		.arg(m_winfirst+1)
		.arg(resCnt);
	    reslistTE->append(line);
	}
	    
	gotone = true;
	
	// Determine icon to display if any
	string img_name;
	if (prefs.showicons) {
	    string iconpath;
	    string iconname = rclconfig->getMimeIconName(doc.mimetype,
							 &iconpath);
	    LOGDEB1(("Img file; %s\n", iconpath.c_str()));
	    QImage image(iconpath.c_str());
	    if (!image.isNull()) {
		img_name = string("img_") + iconname;
		QMimeSourceFactory::defaultFactory()->
		    setImage(img_name.c_str(), image);
	    }
	}

	// Percentage of 'relevance'
	char perbuf[10];
	sprintf(perbuf, "%3d%% ", percent);

	// Make title out of file name if none yet
	if (doc.title.empty()) 
	    doc.title = path_getsimple(doc.url);

	// Document date: either doc or file modification time
	char datebuf[100];
	datebuf[0] = 0;
	if (!doc.dmtime.empty() || !doc.fmtime.empty()) {
	    time_t mtime = doc.dmtime.empty() ?
		atol(doc.fmtime.c_str()) : atol(doc.dmtime.c_str());
	    struct tm *tm = localtime(&mtime);
	    strftime(datebuf, 99, 
		     "<i>Modified:</i>&nbsp;%Y-%m-%d&nbsp;%H:%M:%S", tm);
	}

	// Size information. We print both doc and file if they differ a lot
	long fsize = 0, dsize = 0;
	if (!doc.dbytes.empty())
	    dsize = atol(doc.dbytes.c_str());
	if (!doc.fbytes.empty())
	    fsize = atol(doc.fbytes.c_str());
	string sizebuf;
	if (dsize > 0) {
	    sizebuf = displayableBytes(dsize);
	    if (fsize > 10 * dsize)
		sizebuf += string(" / ") + displayableBytes(fsize);
	}

	// Abstract
	string abst = escapeHtml(doc.abstract);
	LOGDEB1(("Abstract: {%s}\n", abst.c_str()));

	// Concatenate chunks to build the result list paragraph:
	string result;
	if (!sh.empty())
	    result += string("<p><b>") + sh + "</p>\n<p>";
	else
	    result = "<p>";
	if (!img_name.empty()) {
	    result += "<img source=\"" + img_name + "\" align=\"left\">";
	}
	result += string(perbuf) + sizebuf + "<b>" + doc.title + "</b><br>";
	result += doc.mimetype + "&nbsp;" + 
	    (datebuf[0] ? string(datebuf) + "<br>" : string("<br>"));
	result += "<i>" + doc.url + +"</i><br>";
	if (!abst.empty())
	    result +=  abst + "<br>";
	if (!doc.keywords.empty())
	    result += doc.keywords + "<br>";
	result += "</p>\n";

	QString str = QString::fromUtf8(result.c_str(), result.length());
	reslistTE->append(str);
	reslistTE->setCursorPosition(0,0);

	m_pageParaToReldocnums[reslistTE->paragraphs()-1] = i;
	m_curDocs.push_back(doc);
    }

    if (gotone) {
	reslistTE->append("</body></qt>");
	reslistTE->ensureCursorVisible();
    } else {
	// Restore first in win parameter that we shouln't have incremented
	reslistTE->append(tr("<p>"
			  /*"<img align=\"left\" source=\"myimage\">"*/
			  "<b>No results found</b>"
			  "<br>"));
	m_winfirst -= prefs.respagesize;
	if (m_winfirst < 0)
	    m_winfirst = -1;
    }

   //reslistTE->setUpdatesEnabled(true);reslistTE->sync();reslistTE->repaint();

#if 0
    {
	FILE *fp = fopen("/tmp/reslistdebug", "w");
	if (fp) {
	    const char *text = (const char *)reslistTE->text().utf8();
	    //const char *text = alltext.c_str();
	    fwrite(text, 1, strlen(text), fp);
	    fclose(fp);
	}
    }
#endif

    if (m_winfirst < 0 || 
	(m_winfirst >= 0 && 
	 m_winfirst + prefs.respagesize >= resCnt)) {
	emit nextPageAvailable(false);
    } else {
	emit nextPageAvailable(true);
    }
}
