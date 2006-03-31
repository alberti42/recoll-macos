#ifndef lint
static char rcsid[] = "@(#$Id: rclreslist.cpp,v 1.8 2006-03-31 17:19:45 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include <time.h>

#include <qvariant.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qtimer.h>
#include <qmessagebox.h>
#include <qimage.h>
#include <qclipboard.h>

#include "debuglog.h"
#include "recoll.h"
#include "guiutils.h"
#include "pathut.h"
#include "docseq.h"
#include "transcode.h"
#include "pathut.h"
#include "mimehandler.h"

#include "rclreslist.h"
#include "moc_rclreslist.cpp"

#ifndef MIN
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#endif

RclResList::RclResList(QWidget* parent, const char* name)
    : QTextBrowser(parent, name) 
{
    if ( !name )
	setName( "rclResList" );
    setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)5, 2, 0, this->sizePolicy().hasHeightForWidth() ) );
    setTextFormat( RclResList::RichText );
    setReadOnly( TRUE );
    setUndoRedoEnabled( FALSE );
    languageChange();
    resize( QSize(198, 144).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );

    // signals and slots connections
    connect(this, SIGNAL(clicked(int, int)), this, SLOT(clicked(int,int)));
    connect(this, SIGNAL(linkClicked(const QString &)), 
	    this, SLOT(linkWasClicked(const QString &)));


    // Code from init:
    m_winfirst = -1;
    m_docsource = 0;
}


RclResList::~RclResList()
{
    if (m_docsource) 
	delete m_docsource;
}

void RclResList::languageChange()
{
    setCaption(tr("Result list"));
}

// Acquire new docsource
void RclResList::setDocSource(DocSequence *docsource)
{
    if (m_docsource)
	delete m_docsource;
    m_docsource = docsource;
    showResultPage();
}

// Get document number from paragraph number
int RclResList::docnumfromparnum(int par)
{
    if (m_winfirst == -1)
	return -1;
    std::map<int,int>::iterator it = m_pageParaToReldocnums.find(par);
    int dn;
    if (it != m_pageParaToReldocnums.end()) {
        dn = m_winfirst + it->second;
    } else {
        dn = -1;
    }
    return dn;
}

bool RclResList::getDoc(int docnum, Rcl::Doc &doc)
{
    if (docnum >= 0 && docnum >= int(m_winfirst) && 
	docnum < int(m_winfirst + m_curDocs.size())) {
	doc = m_curDocs[docnum - m_winfirst];
	return true;
    }
    return false;
}

void RclResList::keyPressEvent( QKeyEvent * e )
{
    if (e->key() == Key_Q && (e->state() & ControlButton)) {
	recollNeedsExit = 1;
	return;
    } else if (e->key() == Key_Prior) {
	resPageUpOrBack();
	return;
    } else if (e->key() == Key_Next) {
	resPageDownOrNext();
	return;
    }
    QTextBrowser::keyPressEvent(e);
}

// Page Up/Down: we don't try to check if current paragraph is last or
// first. We just page up/down and check if viewport moved. If it did,
// fair enough, else we go to next/previous result page.
void RclResList::resPageUpOrBack()
{
    int vpos = contentsY();
    moveCursor(QTextEdit::MovePgUp, false);
    if (vpos == contentsY())
	resultPageBack();
}


void RclResList::resPageDownOrNext()
{
    int vpos = contentsY();
    moveCursor(QTextEdit::MovePgDown, false);
    LOGDEB(("RclResList::resPageDownOrNext: vpos before %d, after %d\n",
	    vpos, contentsY()));
    if (vpos == contentsY()) 
	showResultPage();
}

// Show previous page of results. We just set the current number back
// 2 pages and show next page.
void RclResList::resultPageBack()
{
    if (m_winfirst <= 0)
	return;
    m_winfirst -= 2 * prefs.respagesize;
    showResultPage();
}

// Convert byte count into unit (KB/MB...) appropriate for display
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
void RclResList::showResultPage()
{
    if (!m_docsource)
	return;

    int percent;
    Rcl::Doc doc;

    int resCnt = m_docsource->getResCnt();
    m_pageParaToReldocnums.clear();

    LOGDEB(("showResultPage: rescnt %d, winfirst %d\n", resCnt,
	    m_winfirst));

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
    clear();

    int last = MIN(resCnt-m_winfirst, prefs.respagesize);

    m_curDocs.clear();

    // Insert results if any in result list window. We have to send
    // the text to the widgets, because we need the paragraph number
    // each time we add a result paragraph (its diffult and
    // error-prone to compute the paragraph numbers in parallel). We
    // would like to disable updates while we're doing this, but
    // couldn't find a way to make it work, the widget seems to become
    // confused if appended while updates are disabled
    //      setUpdatesEnabled(false);
    for (int i = 0; i < last; i++) {
	string sh;
	doc.erase();

	if (!m_docsource->getDoc(m_winfirst + i, doc, &percent, &sh)) {
	    // Error or end of docs, stop.
	    break;
	}
	if (percent == -1) {
	    percent = 0;
	    // Document not available, maybe other further, will go on.
	    doc.abstract = string(tr("Unavailable document").utf8());
	}

	if (i == 0) {
	    // Display list header
	    // We could use a <title> but the textedit doesnt display
	    // it prominently
	    append("<qt><head></head><body>");
	    // Note: have to append text in chunks that make sense
	    // html-wise. If we break things up to much, the editor
	    // gets confused. Hence the use of the 'chunk' text
	    // accumulator
	    QString chunk = "<p><font size=+1><b>";
	    chunk += QString::fromUtf8(m_docsource->title().c_str());
	    chunk += "</b></font><br>";
	    chunk += "<a href=\"H-1\">";
	    chunk += tr("Show query details");
	    chunk += "</a><br>";
	    append(chunk);
	    chunk = tr("<b>Displaying results starting at index"
		      " %1 (maximum set size %2)</b></p>\n")
		.arg(m_winfirst+1)
		.arg(resCnt);
	    append(chunk);
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
	string fcharset = rclconfig->getDefCharset(true);
	if (doc.title.empty()) {
	    transcode(path_getsimple(doc.url), doc.title, fcharset, "UTF-8");
	}

	// Printable url: either utf-8 if transcoding succeeds, or url-encoded
	string url; int ecnt = 0;
	if (!transcode(doc.url, url, fcharset, "UTF-8", &ecnt) || ecnt) {
	    url = url_encode(doc.url, 7);
	}
	
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
	long fsize = -1, dsize = -1;
	if (!doc.dbytes.empty())
	    dsize = atol(doc.dbytes.c_str());
	if (!doc.fbytes.empty())
	    fsize = atol(doc.fbytes.c_str());
	string sizebuf;
	if (dsize >= 0) {
	    sizebuf = displayableBytes(dsize);
	    if (fsize > 10 * dsize && fsize - dsize > 1000)
		sizebuf += string(" / ") + displayableBytes(fsize);
	} else if (fsize >= 0) {
	    sizebuf = displayableBytes(fsize);
	}
	// Abstract
	string abst = escapeHtml(doc.abstract);
	LOGDEB1(("Abstract: {%s}\n", abst.c_str()));

	// Concatenate chunks to build the result list paragraph:
	string result;
	if (!sh.empty())
	    result += string("<p><b>") + sh + "</p>\n<p>";
	else
	    result += "<p>";
	result += string(perbuf) + sizebuf + "<b>" + doc.title + "</b><br>";
	result += doc.mimetype + "&nbsp;";
	result += string(datebuf) + "&nbsp;&nbsp;&nbsp;";


	// Set up the preview and edit links
	char vlbuf[100];
	if (canIntern(doc.mimetype, rclconfig)) { 
	    sprintf(vlbuf, "\"P%d\"", m_winfirst+i);
	    result += string("<a href=") + vlbuf + ">" + "Preview" + "</a>" 
		+ "&nbsp;&nbsp;";
	}
	if (!rclconfig->getMimeViewerDef(doc.mimetype).empty()) {
	    sprintf(vlbuf, "E%d", m_winfirst+i);
	    result += string("<a href=") + vlbuf + ">" + "Edit" + "</a>";
	}
	result += string("<br>");

	if (!img_name.empty()) {
	    result += "<img source=\"" + img_name + "\" align=\"left\">";
	}
	result += "<i>" + url + +"</i><br>";
	if (!abst.empty())
	    result +=  abst + "<br>";
	if (!doc.keywords.empty())
	    result += doc.keywords + "<br>";

	result += "</p>\n";

	QString str = QString::fromUtf8(result.c_str(), result.length());
	append(str);
	setCursorPosition(0,0);

        m_pageParaToReldocnums[paragraphs()-1] = i;
	m_curDocs.push_back(doc);
    }

    if (gotone) {
	append("</body></qt>");
	ensureCursorVisible();
    } else {
	// Restore first in win parameter that we shouln't have incremented
	append(tr("<p>"
			  /*"<img align=\"left\" source=\"myimage\">"*/
			  "<b>No results found</b>"
			  "<br>"));
	m_winfirst -= prefs.respagesize;
	if (m_winfirst < 0)
	    m_winfirst = -1;
    }

   //setUpdatesEnabled(true);sync();repaint();

#if 0
    {
	FILE *fp = fopen("/tmp/reslistdebug", "w");
	if (fp) {
	    const char *text = (const char *)text().utf8();
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

// Single click in result list: we don't actually do anything but
// start a timer because we want to check first if this might be a
// double click
void RclResList::clicked(int par, int car)
{
    LOGDEB(("RclResList::clicked:wfirst %d par %d char %d\n", 
	    m_winfirst, par, car));
    // Erase everything back to white
    {
	QColor color("white");
	for (int i = 0; i < paragraphs(); i++)
	    setParagraphBackgroundColor(i, color);
    }

    // Color the new active paragraph
    QColor color("lightblue");
    setParagraphBackgroundColor(par, color);
}

void RclResList::linkWasClicked(const QString &s)
{
    LOGDEB(("RclResList::linkClicked: [%s]\n", s.ascii()));
    int i = atoi(s.ascii()+1);
    int what = s.ascii()[0];
    switch (what) {
    case 'H': 
	emit headerClicked(); 
	break;
    case 'P': 
	emit docClicked(i); 
	break;
    case 'E': 
	emit docDoubleClicked(i);
	break;
    default: break;// ?? 
    }
}

QPopupMenu *RclResList::createPopupMenu(const QPoint& pos)
{
    int para = paragraphAt(pos);
    clicked(para, 0);
    m_docnum = docnumfromparnum(para);
    QPopupMenu *popup = new QPopupMenu(this, "qt_edit_menu");
    popup->insertItem(tr("&Preview"), this, SLOT(menuPreview()));
    popup->insertItem(tr("&Edit"), this, SLOT(menuEdit()));
    popup->insertItem(tr("&Copy File Name"), this, SLOT(menuCopyFN()));
    popup->insertItem(tr("Copy &Url"), this, SLOT(menuCopyURL()));
    return popup;
}

void RclResList::menuPreview()
{
    emit docClicked(m_docnum);
}
void RclResList::menuEdit()
{
    emit docDoubleClicked(m_docnum);
}
void RclResList::menuCopyFN()
{
    Rcl::Doc doc;
    if (getDoc(m_docnum, doc)) {
	// Our urls currently always begin with "file://"
	QApplication::clipboard()->setText(doc.url.c_str()+7, 
					   QClipboard::Selection);
    }
}
void RclResList::menuCopyURL()
{
    Rcl::Doc doc;
    if (getDoc(m_docnum, doc)) {
	string url =  url_encode(doc.url, 7);
	QApplication::clipboard()->setText(url.c_str(), 
					   QClipboard::Selection);
    }
}
