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

#include <regex.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <utility>
#ifndef NO_NAMESPACES
using std::pair;
#endif /* NO_NAMESPACES */

#include <qmessagebox.h>
#include <qcstring.h>
#include <qtabwidget.h>
#include <qtimer.h>
#include <qstatusbar.h>
#include <qwindowdefs.h>
#include <qapplication.h>

#include "recoll.h"
#include "debuglog.h"
#include "mimehandler.h"
#include "pathut.h"
#include "smallut.h"
#include "plaintorich.h"
#include "advsearch.h"
#include "rclversion.h"

extern "C" int XFlush(void *);

#ifndef MIN
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#endif

// Number of abstracts in a result page. This will avoid scrollbars
// with the default window size and font, most of the time.
static const int respagesize = 8;

void RecollMain::init()
{
    reslist_winfirst = -1;
    reslist_current = -1;
    reslist_mouseDrag = false;
    reslist_mouseDown = false;
    reslist_par = -1;
    reslist_car = -1;
    reslist_waitingdbl = false;
    reslist_dblclck = false;
    dostem = false;
    curPreview = 0;
    asearchform = 0;
    docsource = 0;
    reslistTE->viewport()->installEventFilter(this);
}

// We also want to get rid of the advanced search form and previews
// when we exit (not our children so that it's not systematically
// created over the main form). 
bool RecollMain::close(bool)
{
    fileExit();
    return false;
}

#if 0
static const char *eventTypeToStr(int tp)
{
    switch (tp) {
    case 0: return "None";
    case 1: return "Timer";
    case 2: return "MouseButtonPress";
    case 3: return "MouseButtonRelease";
    case 4: return "MouseButtonDblClick";
    case 5: return "MouseMove";
    case 6: return "KeyPress";
    case 7: return "KeyRelease";
    case 8: return "FocusIn";
    case 9: return "FocusOut";
    case 10: return "Enter";
    case 11: return "Leave";
    case 12: return "Paint";
    case 13: return "Move";
    case 14: return "Resize";
    case 15: return "Create";
    case 16: return "Destroy";
    case 17: return "Show";
    case 18: return "Hide";
    case 19: return "Close";
    case 20: return "Quit";
    case 21: return "Reparent";
    case 22: return "ShowMinimized";
    case 23: return "ShowNormal";
    case 24: return "WindowActivate";
    case 25: return "WindowDeactivate";
    case 26: return "ShowToParent";
    case 27: return "HideToParent";
    case 28: return "ShowMaximized";
    case 29: return "ShowFullScreen";
    case 30: return "Accel";
    case 31: return "Wheel";
    case 32: return "AccelAvailable";
    case 33: return "CaptionChange";
    case 34: return "IconChange";
    case 35: return "ParentFontChange";
    case 36: return "ApplicationFontChange";
    case 37: return "ParentPaletteChange";
    case 38: return "ApplicationPaletteChange";
    case 39: return "PaletteChange";
    case 40: return "Clipboard";
    case 42: return "Speech";
    case 50: return "SockAct";
    case 51: return "AccelOverride";
    case 52: return "DeferredDelete";
    case 60: return "DragEnter";
    case 61: return "DragMove";
    case 62: return "DragLeave";
    case 63: return "Drop";
    case 64: return "DragResponse";
    case 70: return "ChildInserted";
    case 71: return "ChildRemoved";
    case 72: return "LayoutHint";
    case 73: return "ShowWindowRequest";
    case 74: return "WindowBlocked";
    case 75: return "WindowUnblocked";
    case 80: return "ActivateControl";
    case 81: return "DeactivateControl";
    case 82: return "ContextMenu";
    case 83: return "IMStart";
    case 84: return "IMCompose";
    case 85: return "IMEnd";
    case 86: return "Accessibility";
    case 87: return "TabletMove";
    case 88: return "LocaleChange";
    case 89: return "LanguageChange";
    case 90: return "LayoutDirectionChange";
    case 91: return "Style";
    case 92: return "TabletPress";
    case 93: return "TabletRelease";
    case 94: return "OkRequest";
    case 95: return "HelpRequest";
    case 96: return "WindowStateChange";
    case 97: return "IconDrag";
    case 1000: return "User";
    case 65535: return "MaxUser";
    default: return "Unknown";
    }
}
#endif

// There are a number of events that we want to process. Not sure the
// ^Q thing is necessary (we have an action for this)?
bool RecollMain::eventFilter( QObject * target, QEvent * event )
{
    //    LOGDEB(("RecollMain::eventFilter target %p, event %s\n", target,
    //	    eventTypeToStr(int(event->type()))));
    if (event->type() == QEvent::KeyPress) {
	QKeyEvent *keyEvent = (QKeyEvent *)event;
	if (keyEvent->key() == Key_Q && (keyEvent->state() & ControlButton)) {
	    recollNeedsExit = 1;
	}
    } else if (target == reslistTE->viewport()) { 
	// We don't want btdown+drag+btup to be a click ! So monitor
	// mouse events
	if (event->type() == QEvent::MouseMove) {
	    LOGDEB1(("reslistTE: MouseMove\n"));
	    if (reslist_mouseDown)
		reslist_mouseDrag = true;
	} else if (event->type() == QEvent::MouseButtonPress) {
	    LOGDEB1(("reslistTE: MouseButtonPress\n"));
	    reslist_mouseDown = true;
	    reslist_mouseDrag = false;
	} else if (event->type() == QEvent::MouseButtonRelease) {
	    LOGDEB1(("reslistTE: MouseButtonRelease\n"));
	    reslist_mouseDown = false;
	} else if (event->type() == QEvent::MouseButtonDblClick) {
	    LOGDEB1(("reslistTE: MouseButtonDblClick\n"));
	    reslist_mouseDown = false;
	}
    }

    return QWidget::eventFilter(target, event);
}

void RecollMain::fileExit()
{
    LOGDEB1(("RecollMain: fileExit\n"));
    if (asearchform)
	delete asearchform;
    // Let the exit handler clean up things
    exit(0);
}

// This is called on a 100ms timer checks the status of the indexing
// thread and a possible need to exit
void RecollMain::periodic100()
{
    static int toggle;
    // Check if indexing thread done
    if (indexingstatus) {
	statusBar()->message("");
	indexingstatus = false;
	// Make sure we reopen the db to get the results.
	LOGINFO(("Indexing done: closing query database\n"));
	rcldb->close();
    } else if (indexingdone == 0) {
	if (toggle < 9) {
	    statusBar()->message(tr("Indexing in progress"));
	} else {
	    statusBar()->message("");
	}
	if (toggle >= 10)
	    toggle = 0;
	toggle++;
    }
    if (recollNeedsExit)
	fileExit();
}

void RecollMain::fileStart_IndexingAction_activated()
{
    if (indexingdone == 1)
	startindexing = 1;
}

// Note that all our 'urls' are like : file://...
static string urltolocalpath(string url)
{
    return url.substr(7, string::npos);
}
// Translate paragraph number in list window to doc number. This depends on 
// how we format the title etc..
static int reldocnumfromparnum(int par)
{
    return par - 2;
}
// Translate paragraph number in list window to doc number. This depends on 
// how we format the title etc..
static int parnumfromreldocnum(int docnum)
{
    return docnum + 2;
}

// Double click in result list: use external viewer to display file
void RecollMain::reslistTE_doubleClicked(int par, int)
{
    LOGDEB(("RecollMain::reslistTE_doubleClicked: par %d\n", par));
    reslist_dblclck = true;

    Rcl::Doc doc;
    int reldocnum =  reldocnumfromparnum(par);
    if (!docsource->getDoc(reslist_winfirst + reldocnum, doc, 0, 0))
	return;
    
    // Look for appropriate viewer
    string cmd = rclconfig->getMimeViewerDef(doc.mimetype);
    if (cmd.length() == 0) {
	QMessageBox::warning(0, "Recoll", 
			     tr("No external viewer configured for mime type ")
			     + doc.mimetype.c_str());
	return;
    }

    string fn = urltolocalpath(doc.url);

    // Substitute %u (url) and %f (file name) inside prototype command
    string ncmd;
    string::const_iterator it1;
    for (it1 = cmd.begin(); it1 != cmd.end();it1++) {
	if (*it1 == '%') {
	    if (++it1 == cmd.end()) {
		ncmd += '%';
		break;
	    }
	    if (*it1 == '%')
		ncmd += '%';
	    if (*it1 == 'u')
		ncmd += "'" + doc.url + "'";
	    if (*it1 == 'f')
		ncmd += "'" + fn + "'";
	} else {
	    ncmd += *it1;
	}
    }

    ncmd += " &";
    QStatusBar *stb = statusBar();
    if (stb) {
	QString msg = tr("Executing: [") + ncmd.c_str() + "]";
	stb->message(msg, 5000);
	stb->repaint(false);
	XFlush(qt_xdisplay());
    }
    history->enterDocument(fn, doc.ipath);
    system(ncmd.c_str());
}


// Display preview for the selected document, and highlight entry. The
// paragraph number is doc number in window + 1
// We don't actually do anything but start a timer because we want to
// check first if this might be a double click
void RecollMain::reslistTE_clicked(int par, int car)
{
    if (reslist_waitingdbl)
	return;
    LOGDEB(("RecollMain::reslistTE_clckd:winfirst %d par %d char %d drg %d\n", 
	    reslist_winfirst, par, car, reslist_mouseDrag));
    if (reslist_winfirst == -1 || reslist_mouseDrag)
	return;

    // remember par and car
    reslist_par = par;
    reslist_car = car;
    reslist_waitingdbl = true;
    reslist_dblclck = false;
    // Wait to see if there's going to be a dblclck
    QTimer::singleShot(150, this, SLOT(reslistTE_delayedclick()) );
}

// This gets called by a timer 100mS after a single click in the
// result list. We don't want to start a preview if the user has
// requested a native viewer by double-clicking
void RecollMain::reslistTE_delayedclick()
{
    reslist_waitingdbl = false;
    if (reslist_dblclck) {
	reslist_dblclck = false;
	return;
    }

    int par = reslist_par;

    if (reslist_current != -1) {
	QColor color("white");
	reslistTE->
	    setParagraphBackgroundColor(parnumfromreldocnum(reslist_current), 
					color);
    }
    QColor color("lightblue");
    reslistTE->setParagraphBackgroundColor(par, color);

    int reldocnum = reldocnumfromparnum(par);
    if (curPreview && reslist_current == reldocnum)
	return;

    reslist_current = reldocnum;
    startPreview(reslist_winfirst + reldocnum);
}

// User asked to start query. Send it to the db aand call
// listNextPB_clicked to fetch and display the first page of results
void RecollMain::queryText_returnPressed()
{
    LOGDEB(("RecollMain::queryText_returnPressed()\n"));
    // The db may have been closed at the end of indexing
    string reason;
    if (!maybeOpenDb(reason)) {
	QMessageBox::critical(0, "Recoll", QString(reason.c_str()));
	exit(1);
    }
    if (stemlang.empty())
	getQueryStemming(dostem, stemlang);

    reslist_current = -1;
    reslist_winfirst = -1;

    QCString u8 =  queryText->text().utf8();

    if (!rcldb->setQuery(string((const char *)u8), dostem ? 
			 Rcl::Db::QO_STEM : Rcl::Db::QO_NONE, stemlang))
	return;
    curPreview = 0;

    if (docsource)
	delete docsource;
    docsource = new DocSequenceDb(rcldb);
    listNextPB_clicked();
}


void RecollMain::Search_clicked()
{
    queryText_returnPressed();
}

void RecollMain::clearqPB_clicked()
{
    queryText->clear();
}

void RecollMain::listPrevPB_clicked()
{
    if (reslist_winfirst <= 0)
	return;
    reslist_winfirst -= 2*respagesize;
    listNextPB_clicked();
}


// Fill up result list window with next screen of hits
void RecollMain::listNextPB_clicked()
{
    if (!docsource)
	return;

    int percent;
    Rcl::Doc doc;

    int resCnt = docsource->getResCnt();

    LOGDEB(("listNextPB_clicked: rescnt %d, winfirst %d\n", resCnt,
	    reslist_winfirst));

    // If we are already on the last page, nothing to do:
    if (reslist_winfirst >= 0 && (reslist_winfirst + respagesize > resCnt))
	return;

    if (reslist_winfirst < 0)
	reslist_winfirst = 0;
    else
	reslist_winfirst += respagesize;

    bool gotone = false;
    reslistTE->clear();

    int last = MIN(resCnt-reslist_winfirst, respagesize);

    // Insert results if any in result list window 
    for (int i = 0; i < last; i++) {
	string sh;
	doc.erase();

	if (!docsource->getDoc(reslist_winfirst + i, doc, &percent, &sh)) {
	    if (i == 0) 
		reslist_winfirst = -1;
	    break;
	}
	if (i == 0) {
	    // We could use a <title> but the textedit doesnt display
	    // it prominently
	    reslistTE->append("<qt><head></head><body>");
	    QString line = "<p><font size=+1><b>";
	    line += docsource->title().c_str();
	    line += "</b></font><br>";
	    reslistTE->append(line);
	    line = tr("<b>Displaying results starting at index"
		      " %1 (maximum set size %2)</b></p>")
		.arg(reslist_winfirst+1)
		.arg(resCnt);
	    reslistTE->append(line);
	}
	    
	gotone = true;

	string img_name;
	if (showicons) {
	    string iconname = rclconfig->getMimeIconName(doc.mimetype);
	    if (iconname.empty())
		iconname = "document";
	    string imgfile = iconsdir + "/" + iconname + ".png";

	    LOGDEB1(("Img file; %s\n", imgfile.c_str()));
	    QImage image(imgfile.c_str());
	    if (!image.isNull()) {
		img_name = string("img_") + iconname;
		QMimeSourceFactory::defaultFactory()->
		    setImage(img_name.c_str(),image);
	    }
	}

	// Result list display: TOBEDONE
	//  - move abstract/keywords to  Detail window ?
	//  - keywords matched
	//  - language
        //  - size
	char perbuf[10];
	sprintf(perbuf, "%3d%%", percent);
	if (doc.title.empty()) 
	    doc.title = path_getsimple(doc.url);
	char datebuf[100];
	datebuf[0] = 0;
	if (!doc.dmtime.empty() || !doc.fmtime.empty()) {
	    time_t mtime = doc.dmtime.empty() ?
		atol(doc.fmtime.c_str()) : atol(doc.dmtime.c_str());
	    struct tm *tm = localtime(&mtime);
	    strftime(datebuf, 99, "<i>Modified:</i>&nbsp;%F&nbsp;%T", tm);
	}
	string abst = stripMarkup(doc.abstract);
	LOGDEB1(("Abstract: {%s}\n", abst.c_str()));
	string result;
	if (!sh.empty())
	    result += string("<br><b>") + sh + "</b><br><br>\n";
	result += string("<p>");
	if (!img_name.empty()) {
	    result += "<img source=\"" + img_name + "\" align=\"left\">";
	}
	result += string(perbuf) + " <b>" + doc.title + "</b><br>" +
	    doc.mimetype + "&nbsp;" +
	    (datebuf[0] ? string(datebuf) + "<br>" : string("<br>")) +
	    (!abst.empty() ? abst + "<br>" : string("")) +
	    (!doc.keywords.empty() ? doc.keywords + "<br>" : string("")) +
	    "<i>" + doc.url + +"</i><br>" +
	    "</p>";

	QString str = QString::fromUtf8(result.c_str(), result.length());
	reslistTE->append(str);
    }

    reslist_current = -1;

    if (gotone) {
	reslistTE->append("</body></qt>");
	reslistTE->setCursorPosition(0,0);
	reslistTE->ensureCursorVisible();
    } else {
	// Restore first in win parameter that we shouln't have incremented
	reslistTE->append(tr("<p>"
			     /*"<img align=\"left\" source=\"myimage\">"*/
			     "<b>No results found</b>"
			     "<br>"));
	reslist_winfirst -= respagesize;
	if (reslist_winfirst < 0)
	    reslist_winfirst = -1;
    }
}

// If a preview (toplevel) window gets closed by the user, we need to
// clean up because there is no way to reopen it. And check the case
// where the current one is closed
void RecollMain::previewClosed(Preview *w)
{
    if (w == curPreview) {
	LOGDEB(("Active preview closed\n"));
	curPreview = 0;
    } else {
	LOGDEB(("Old preview closed\n"));
    }
    delete w;
}

// Open advanced search dialog.
void RecollMain::advSearchPB_clicked()
{
    if (asearchform == 0) {
	asearchform = new advsearch(0, tr("Advanced search"), FALSE,
				    WStyle_Customize | WStyle_NormalBorder | 
				    WStyle_Title | WStyle_SysMenu);
	asearchform->setSizeGripEnabled(FALSE);
	connect(asearchform, SIGNAL(startSearch(Rcl::AdvSearchData)), 
		this, SLOT(startAdvSearch(Rcl::AdvSearchData)));
	asearchform->show();
    } else {
	asearchform->show();
    }
}

// Execute an advanced search query. The parameters normally come from
// the advanced search dialog
void RecollMain::startAdvSearch(Rcl::AdvSearchData sdata)
{
    LOGDEB(("RecollMain::startAdvSearch\n"));
    // The db may have been closed at the end of indexing
    string reason;
    if (!maybeOpenDb(reason)) {
	QMessageBox::critical(0, "Recoll", QString(reason.c_str()));
	exit(1);
    }
    if (stemlang.empty())
	getQueryStemming(dostem, stemlang);

    reslist_current = -1;
    reslist_winfirst = -1;

    if (!rcldb->setQuery(sdata, dostem ? 
			 Rcl::Db::QO_STEM : Rcl::Db::QO_NONE, stemlang))
	return;
    curPreview = 0;
    if (docsource)
	delete docsource;
    docsource = new DocSequenceDb(rcldb);
    listNextPB_clicked();
}


/** 
 * Open a preview window for a given document, or load it into new tab of 
 * existing window.
 *
 * @param docnum db query index
 */
void RecollMain::startPreview(int docnum)
{
    Rcl::Doc doc;
    if (!docsource->getDoc(docnum, doc, 0)) {
	QMessageBox::warning(0, "Recoll",
			     tr("Cannot retrieve document info" 
				     " from database"));
	return;
    }
	
    // Check file exists in file system
    string fn = urltolocalpath(doc.url);
    struct stat st;
    if (stat(fn.c_str(), &st) < 0) {
	QMessageBox::warning(0, "Recoll",
			     tr("Cannot access document file: ") +
			     fn.c_str());
	return;
    }

    if (curPreview == 0) {
	curPreview = new Preview(0, tr("Preview"));
	if (curPreview == 0) {
	    QMessageBox::warning(0, tr("Warning"), 
				 tr("Can't create preview window"),
				 QMessageBox::Ok, 
				 QMessageBox::NoButton);
	    return;
	}

	curPreview->setCaption(queryText->text());
	connect(curPreview, SIGNAL(previewClosed(Preview *)), 
		this, SLOT(previewClosed(Preview *)));
	curPreview->show();
    } else {
	if (curPreview->makeDocCurrent(fn, doc)) {
	    // Already there
	    return;
	}
	(void)curPreview->addEditorTab();
    }
    history->enterDocument(fn, doc.ipath);
    curPreview->loadFileInCurrentTab(fn, st.st_size, doc);
}


void RecollMain::showAboutDialog()
{
    string vstring = string("Recoll ") + rclversion + 
	"<br>" + "http://www.recoll.org";
    QMessageBox::information(this, tr("About Recoll"), vstring.c_str());
}


void RecollMain::showDocHistory()
{
    LOGDEB(("RecollMain::showDocHistory\n"));
    reslist_current = -1;
    reslist_winfirst = -1;
    curPreview = 0;

    if (docsource)
	delete docsource;
    docsource = new DocSequenceHistory(rcldb, history);
    listNextPB_clicked();
}
