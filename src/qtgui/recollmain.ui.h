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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex.h>

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
#include <qcheckbox.h>
#include <qfontdialog.h>
#include <qspinbox.h>
#include <qcombobox.h>

#include "recoll.h"
#include "debuglog.h"
#include "mimehandler.h"
#include "pathut.h"
#include "smallut.h"
#include "plaintorich.h"
#include "advsearch.h"
#include "rclversion.h"
#include "sortseq.h"
#include "uiprefs.h"

extern "C" int XFlush(void *);

#ifndef MIN
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#endif

void RecollMain::init()
{
    reslist_winfirst = -1;
    reslist_mouseDrag = false;
    reslist_mouseDown = false;
    reslist_par = -1;
    reslist_car = -1;
    reslist_waitingdbl = false;
    reslist_dblclck = false;
    curPreview = 0;
    asearchform = 0;
    sortform = 0;
    docsource = 0;
    sortwidth = 0;
    uiprefs = 0;

    // We manage pgup/down, but let ie the arrows for the editor to process
    reslistTE->installEventFilter(this);
    reslistTE->viewport()->installEventFilter(this);
    // reslistTE->viewport()->setFocusPolicy(QWidget::NoFocus);

    // Set the focus to the search terms entry:
    queryText->setFocus();

    // Set result list font according to user preferences.
    if (prefs_reslistfontfamily.length()) {
	QFont nfont(prefs_reslistfontfamily, prefs_reslistfontsize);
	reslistTE->setFont(nfont);
    }
}

// We also want to get rid of the advanced search form and previews
// when we exit (not our children so that it's not systematically
// created over the main form). 
bool RecollMain::close(bool)
{
    fileExit();
    return false;
}

//#define SHOWEVENTS
#if defined(SHOWEVENTS)
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
#if defined(SHOWEVENTS)
    LOGDEB(("RecollMain::eventFilter target %p, event %s\n", target,
	    eventTypeToStr(int(event->type()))));
#endif
    if (event->type() == QEvent::KeyPress) {
	QKeyEvent *keyEvent = (QKeyEvent *)event;
	if (keyEvent->key() == Key_Q && (keyEvent->state() & ControlButton)) {
	    recollNeedsExit = 1;
	} else if (keyEvent->key() == Key_Prior) {
	    resPageUpOrBack();
	    return true;
	} else if (keyEvent->key() == Key_Next) {
	    resPageDownOrNext();
	    return true;
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
int RecollMain::reldocnumfromparnum(int par)
{
    std::map<int,int>::iterator it = pageParaToReldocnums.find(par);
    int rdn;
    if (it != pageParaToReldocnums.end()) {
	rdn = it->second;
    } else {
	rdn = -1;
    }
    LOGDEB1(("reldocnumfromparnum: par %d reldoc %d\n", par, rdn));
    return rdn;
}

// Double click in result list: use external viewer to display file
void RecollMain::reslistTE_doubleClicked(int par, int)
{
    LOGDEB(("RecollMain::reslistTE_doubleClicked: par %d\n", par));
    reslist_dblclck = true;
    int reldocnum =  reldocnumfromparnum(par);
    if (reldocnum < 0)
	return;
    startNativeViewer(reslist_winfirst + reldocnum);
}


// Display preview for the selected document, and highlight entry. The
// paragraph number is doc number in window + 1
// We don't actually do anything but start a timer because we want to
// check first if this might be a double click
void RecollMain::reslistTE_clicked(int par, int car)
{
    if (reslist_waitingdbl)
	return;
    LOGDEB(("RecollMain::reslistTE_clicked:wfirst %d par %d char %d drg %d\n", 
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
    LOGDEB(("RecollMain::reslistTE_delayedclick:\n"));
    reslist_waitingdbl = false;
    if (reslist_dblclck) {
	LOGDEB1(("RecollMain::reslistTE_delayedclick: dbleclick\n"));
	reslist_dblclck = false;
	return;
    }

    int par = reslist_par;

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
	// Bad number: must have clicked on header. Show details of query
	QString desc = tr("Query details") + ": " + 
	    QString::fromUtf8(currentQueryData.description.c_str());
	QMessageBox::information(this, tr("Query details"), desc);
	return;
    } else {
	startPreview(reslist_winfirst + reldocnum);
    }
}

// User asked to start query. Send it to the db aand call
// listNextPB_clicked to fetch and display the first page of results
void RecollMain::startSimpleSearch()
{
    LOGDEB(("RecollMain::queryText_returnPressed()\n"));
    // The db may have been closed at the end of indexing
    Rcl::AdvSearchData sdata;

    QCString u8 =  queryText->text().utf8();
    if (allTermsCB->isChecked())
	sdata.allwords = u8;
    else
	sdata.orwords = u8;

    startAdvSearch(sdata);
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

    reslist_winfirst = -1;

    if (!rcldb->setQuery(sdata, prefs_queryStemLang.length() > 0 ? 
			 Rcl::Db::QO_STEM : Rcl::Db::QO_NONE, 
			 prefs_queryStemLang.ascii()))
	return;
    curPreview = 0;
    if (docsource)
	delete docsource;

    if (sortwidth > 0) {
	DocSequenceDb myseq(rcldb, tr("Query results"));
	docsource = new DocSeqSorted(myseq, sortwidth, sortspecs,
				     tr("Query results (sorted)"));
    } else {
	docsource = new DocSequenceDb(rcldb, tr("Query results"));
    }
    currentQueryData = sdata;
    showResultPage();
}

// Page Up/Down: we don't try to check if current paragraph is last or
// first. We just page up/down and check if viewport moved. If it did,
// fair enough, else we go to next/previous result page.
void RecollMain::resPageUpOrBack()
{
    int vpos = reslistTE->contentsY();
    reslistTE->moveCursor(QTextEdit::MovePgUp, false);
    if (vpos == reslistTE->contentsY())
	resultPageBack();
}
void RecollMain::resPageDownOrNext()
{
    int vpos = reslistTE->contentsY();
    reslistTE->moveCursor(QTextEdit::MovePgDown, false);
    LOGDEB(("RecollMain::resPageDownOrNext: vpos before %d, after %d\n",
	    vpos, reslistTE->contentsY()));
    if (vpos == reslistTE->contentsY()) 
	showResultPage();
}

// Show previous page of results. We just set the current number back
// 2 pages and show next page.
void RecollMain::resultPageBack()
{
    if (reslist_winfirst <= 0)
	return;
    reslist_winfirst -= 2 * prefs_respagesize;
    showResultPage();
}


// Fill up result list window with next screen of hits
void RecollMain::showResultPage()
{
    if (!docsource)
	return;

    int percent;
    Rcl::Doc doc;

    int resCnt = docsource->getResCnt();

    LOGDEB(("showResultPage: rescnt %d, winfirst %d\n", resCnt,
	    reslist_winfirst));

    pageParaToReldocnums.clear();

    // If we are already on the last page, nothing to do:
    if (reslist_winfirst >= 0 && 
	(reslist_winfirst + prefs_respagesize > resCnt)) {
	nextPageAction->setEnabled(false);
	return;
    }

    if (reslist_winfirst < 0) {
	reslist_winfirst = 0;
	prevPageAction->setEnabled(false);
    } else {
	prevPageAction->setEnabled(true);
	reslist_winfirst += prefs_respagesize;
    }

    bool gotone = false;
    reslistTE->clear();

    int last = MIN(resCnt-reslist_winfirst, prefs_respagesize);


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

	if (!docsource->getDoc(reslist_winfirst + i, doc, &percent, &sh)) {
	    // This may very well happen for history if the doc has
	    // been removed since. So don't treat it as fatal.
	    doc.abstract = string(tr("Unavailable document").utf8());
	}
	if (i == 0) {
	    // Display header
	    // We could use a <title> but the textedit doesnt display
	    // it prominently
	    reslistTE->append("<qt><head></head><body>");
	    QString line = "<p><font size=+1><b>";
	    line += docsource->title().c_str();
	    line += "</b></font><br>";
	    reslistTE->append(line);
	    line = tr("<b>Displaying results starting at index"
		      " %1 (maximum set size %2)</b></p>\n")
		.arg(reslist_winfirst+1)
		.arg(resCnt);
	    reslistTE->append(line);
	}
	    
	gotone = true;

	// Result list entry display: this must be exactly one paragraph
	// TOBEDONE
	//  - move abstract/keywords to  Detail window ?
	//  - keywords matched ?
	//  - language ?
        //  - size ?

	string result;
	if (!sh.empty())
	    result += string("<p><b>") + sh + "</p>\n<p>";
	else
	    result = "<p>";

	string img_name;
	if (prefs_showicons) {
	    string iconname = rclconfig->getMimeIconName(doc.mimetype);
	    if (iconname.empty())
		iconname = "document";
	    string imgfile = iconsdir + "/" + iconname + ".png";

	    LOGDEB1(("Img file; %s\n", imgfile.c_str()));
	    QImage image(imgfile.c_str());
	    if (!image.isNull()) {
		img_name = string("img_") + iconname;
		QMimeSourceFactory::defaultFactory()->
		    setImage(img_name.c_str(), image);
	    }
	}

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
	    strftime(datebuf, 99, 
		     "<i>Modified:</i>&nbsp;%Y-%m-%d&nbsp;%H:%M:%S", tm);
	}
	string abst = escapeHtml(doc.abstract);
	LOGDEB1(("Abstract: {%s}\n", abst.c_str()));
	if (!img_name.empty()) {
	    result += "<img source=\"" + img_name + "\" align=\"left\">";
	}
	result += string(perbuf) + " <b>" + doc.title + "</b><br>" +
	    doc.mimetype + "&nbsp;" +
	    (datebuf[0] ? string(datebuf) + "<br>" : string("<br>")) +
	    (!abst.empty() ? abst + "<br>" : string("")) +
	    (!doc.keywords.empty() ? doc.keywords + "<br>" : string("")) +
	    "<i>" + doc.url + +"</i><br></p>\n";

	QString str = QString::fromUtf8(result.c_str(), result.length());
	reslistTE->append(str);

	pageParaToReldocnums[reslistTE->paragraphs()-1] = i;
    }

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
	reslist_winfirst -= prefs_respagesize;
	if (reslist_winfirst < 0)
	    reslist_winfirst = -1;
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

    if (reslist_winfirst < 0 || 
	(reslist_winfirst >= 0 && 
	 reslist_winfirst + prefs_respagesize >= resCnt)) {
	nextPageAction->setEnabled(false);
    } else {
	nextPageAction->setEnabled(true);
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
void RecollMain::showAdvSearchDialog()
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
	// Close and reopen, in hope that makes us visible...
	asearchform->close();
	asearchform->show();
    }
}

void RecollMain::showSortDialog()
{
    if (sortform == 0) {
	sortform = new SortForm(0, tr("Sort criteria"), FALSE,
				    WStyle_Customize | WStyle_NormalBorder | 
				    WStyle_Title | WStyle_SysMenu);
	sortform->setSizeGripEnabled(FALSE);
	connect(sortform, SIGNAL(sortDataChanged(int, RclSortSpec)), 
		this, SLOT(sortDataChanged(int, RclSortSpec)));
	sortform->show();
    } else {
	// Close and reopen, in hope that makes us visible...
	sortform->close();
        sortform->show();
    }

}

void RecollMain::showUIPrefs()
{
    if (uiprefs == 0) {
	uiprefs = new UIPrefsDialog(0, tr("User interface preferences"), FALSE,
				    WStyle_Customize | WStyle_NormalBorder | 
				    WStyle_Title | WStyle_SysMenu);
	uiprefs->setSizeGripEnabled(FALSE);
	connect(uiprefs, SIGNAL(uiprefsDone()), this, SLOT(setUIPrefs()));
	uiprefs->show();
    } else {
	// Close and reopen, in hope that makes us visible...
	uiprefs->close();
        uiprefs->show();
    }
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
	QMessageBox::warning(0, "Recoll", tr("Cannot access document file: ") +
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

void RecollMain::startNativeViewer(int docnum)
{
    Rcl::Doc doc;
    if (!docsource->getDoc(docnum, doc, 0, 0)) {
	QMessageBox::warning(0, "Recoll",
			     tr("Cannot retrieve document info" 
				" from database"));
	return;
    }
    
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


void RecollMain::showAboutDialog()
{
    string vstring = string("Recoll ") + rclversion + 
	"<br>" + "http://www.recoll.org";
    QMessageBox::information(this, tr("About Recoll"), vstring.c_str());
}

void RecollMain::startManual()
{
    startHelpBrowser();
}

void RecollMain::showDocHistory()
{
    LOGDEB(("RecollMain::showDocHistory\n"));
    reslist_winfirst = -1;
    curPreview = 0;
    if (docsource)
	delete docsource;

    if (sortwidth > 0) {
	DocSequenceHistory myseq(rcldb, history, tr("Document history"));
	docsource = new DocSeqSorted(myseq, sortwidth, sortspecs,
				     tr("Document history (sorted)"));
    } else {
	docsource = new DocSequenceHistory(rcldb, history, 
					   tr("Document history"));
    }
    currentQueryData.erase();
    currentQueryData.description = tr("History data").utf8();
    showResultPage();
}


void RecollMain::searchTextChanged(const QString & text)
{
    if (text.isEmpty()) {
	searchPB->setEnabled(false);
	clearqPB->setEnabled(false);
    } else {
	searchPB->setEnabled(true);
	clearqPB->setEnabled(true);
    }

}

void RecollMain::sortDataChanged(int cnt, RclSortSpec spec)
{
    LOGDEB(("RecollMain::sortDataChanged\n"));
    sortwidth = cnt;
    sortspecs = spec;
}

// This could be handled inside the dialog's accept(), but we may want to
// do something (ie: redisplay reslist?)
void RecollMain::setUIPrefs()
{
    if (!uiprefs)
	return;
    LOGDEB(("Recollmain::setUIPrefs\n"));
    prefs_showicons = uiprefs->useIconsCB->isChecked();
    prefs_respagesize = uiprefs->pageLenSB->value();

    prefs_reslistfontfamily = uiprefs->reslistFontFamily;
    prefs_reslistfontsize = uiprefs->reslistFontSize;
    if (prefs_reslistfontfamily.length()) {
	QFont nfont(prefs_reslistfontfamily, prefs_reslistfontsize);
	reslistTE->setFont(nfont);
    } else {
	reslistTE->setFont(this->font());
    }

    if (uiprefs->stemLangCMB->currentItem() == 0) {
	prefs_queryStemLang = "";
    } else {
	prefs_queryStemLang = uiprefs->stemLangCMB->currentText();
    }
}
