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
using std::pair;

#include <qmessagebox.h>
#include <qcstring.h>
#include <qtabwidget.h>
#include <qtimer.h>
#include <qstatusbar.h>
#include <qwindowdefs.h>
#include <qapplication.h>

#include "rcldb.h"
#include "rclconfig.h"
#include "debuglog.h"
#include "mimehandler.h"
#include "pathut.h"
#include "recoll.h"
#include "internfile.h"
#include "smallut.h"
#include "plaintorich.h"
#include "unacpp.h"
#include "advsearch.h"

extern "C" int XFlush(void *);

#ifndef MIN
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#endif

// Number of abstracts in a result page. This will avoid scrollbars
// with the default window size and font, most of the time.
static const int respagesize = 8;


void RecollMain::init()
{
    reslist_current = -1;
    reslist_winfirst = -1;
    curPreview = 0;
    asearchform = 0;
    reslist_mouseDrag = false;
    reslist_mouseDown = false;
    reslistTE_waitingdbl = false;
    reslistTE_dblclck = false;
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

// We want to catch ^Q everywhere to mean quit.
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
	    statusBar()->message("Indexing in progress");
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

// Double click in result list: use external viewer to display file
void RecollMain::reslistTE_doubleClicked(int par, int)
{
    LOGDEB(("RecollMain::reslistTE_doubleClicked: par %d\n", par));
    reslistTE_dblclck = true;

    Rcl::Doc doc;
    int reldocnum =  par - 1;
    if (!rcldb->getDoc(reslist_winfirst + reldocnum, doc, 0))
	return;
    
    // Look for appropriate viewer
    string cmd = getMimeViewer(doc.mimetype, rclconfig->getMimeConf());
    if (cmd.length() == 0) {
	QMessageBox::warning(0, "Recoll", 
			     QString("No external viewer configured for mime"
				     " type ") +
			     doc.mimetype.c_str());
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
	string msg = string("Executing: [") + ncmd.c_str() + "]";
	stb->message(msg.c_str(), 5000);
	stb->repaint(false);
	XFlush(qt_xdisplay());
    }
    system(ncmd.c_str());
}


// Display preview for the selected document, and highlight entry. The
// paragraph number is doc number in window + 1
// We don't actually do anything but start a timer because we want to
// check first if this might be a double click
void RecollMain::reslistTE_clicked(int par, int car)
{
    if (reslistTE_waitingdbl)
	return;
    LOGDEB(("RecollMain::reslistTE_clckd:winfirst %d par %d char %d drg %d\n", 
	    reslist_winfirst, par, car, reslist_mouseDrag));
    if (reslist_winfirst == -1 || reslist_mouseDrag)
	return;

    // remember par and car
    reslistTE_par = par;
    reslistTE_car = car;
    reslistTE_waitingdbl = true;
    reslistTE_dblclck = false;
    // Wait to see if there's going to be a dblclck
    QTimer::singleShot(150, this, SLOT(reslistTE_delayedclick()) );
}


// User asked to start query. Send it to the db aand call
// listNextPB_clicked to fetch and display the first page of results
void RecollMain::queryText_returnPressed()
{
    LOGDEB(("RecollMain::queryText_returnPressed()\n"));
    string reason;
    if (!maybeOpenDb(reason)) {
	QMessageBox::critical(0, "Recoll", QString(reason.c_str()));
	return;
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
    if (!rcldb)
	return;

    int percent;
    Rcl::Doc doc;

    // Need to fetch one document before we can get the result count 
    rcldb->getDoc(0, doc, &percent);
    int resCnt = rcldb->getResCnt();

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
	doc.erase();

	if (!rcldb->getDoc(reslist_winfirst + i, doc, &percent)) {
	    if (i == 0) 
		reslist_winfirst = -1;
	    break;
	}
	if (i == 0) {
	    reslistTE->append("<qt><head></head><body><p>");
	    char line[80];
	    sprintf(line, "<p><b>Displaying results starting at index"
		    " %d (maximum set size %d)</b><br>",
		    reslist_winfirst+1, resCnt);
	    reslistTE->append(line);
	}
	    
	gotone = true;

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
	string result = "<p>" + 
	    string(perbuf) + " <b>" + doc.title + "</b><br>" +
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
	reslistTE->append("<p><b>No results found</b><br>");
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
	asearchform = new advsearch(0, "Advanced search", FALSE,
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
    string reason;
    if (!maybeOpenDb(reason)) {
	QMessageBox::critical(0, "Recoll", QString(reason.c_str()));
	return;
    }

    if (stemlang.empty())
	getQueryStemming(dostem, stemlang);

    reslist_current = -1;
    reslist_winfirst = -1;

    if (!rcldb->setQuery(sdata,  stemlang))
	return;
    curPreview = 0;
    listNextPB_clicked();
}



// This gets called by a timer 100mS after a single click in the
// result list. We don't want to start a preview if the user has
// requested a native viewer by double-clicking
void RecollMain::reslistTE_delayedclick()
{
    reslistTE_waitingdbl = false;
    if (reslistTE_dblclck) {
	reslistTE_dblclck = false;
	return;
    }

    int par = reslistTE_par;

    if (reslist_current != -1) {
	QColor color("white");
	reslistTE->setParagraphBackgroundColor(reslist_current+1, color);
    }
    QColor color("lightblue");
    reslistTE->setParagraphBackgroundColor(par, color);

    int reldocnum = par - 1;
    if (curPreview && reslist_current == reldocnum)
	return;

    reslist_current = reldocnum;
    startPreview(reslist_winfirst + reldocnum);
}


// Open a preview window for a given document
// docnum is a db query index
void RecollMain::startPreview(int docnum)
{
    Rcl::Doc doc;
    if (!rcldb->getDoc(docnum, doc, 0)) {
	QMessageBox::warning(0, "Recoll",
			     QString("Cannot retrieve document info" 
				     " from database"));
	return;
    }
	
    // Go to the file system to retrieve / convert the document text
    // for preview:
    string fn = urltolocalpath(doc.url);
    struct stat st;
    if (stat(fn.c_str(), &st) < 0) {
	QMessageBox::warning(0, "Recoll",
			     QString("Cannot access document file: ") +
			     fn.c_str());
	return;
    }

    QStatusBar *stb = statusBar();
    char csz[20];
    sprintf(csz, "%lu", (unsigned long)st.st_size);
    string msg = string("Loading: ") + fn + " (size " + csz + " bytes)";
    stb->message(msg.c_str());
    qApp->processEvents();

    Rcl::Doc fdoc;
    FileInterner interner(fn, rclconfig, tmpdir);
    if (interner.internfile(fdoc, doc.ipath) != FileInterner::FIDone) {
	QMessageBox::warning(0, "Recoll",
			     QString("Can't turn doc into internal rep ") +
			     doc.mimetype.c_str());
	return;
    }

    stb->message("Creating preview text");
    qApp->processEvents();

    list<string> terms;
    rcldb->getQueryTerms(terms);
    list<pair<int, int> > termoffsets;
    string rich = plaintorich(fdoc.text, terms, termoffsets);

    stb->message("Creating preview window");
    qApp->processEvents();
    QTextEdit *editor;
    if (curPreview == 0) {
	curPreview = new Preview(0, "Preview");
	curPreview->setCaption(queryText->text());
	connect(curPreview, SIGNAL(previewClosed(Preview *)), 
		this, SLOT(previewClosed(Preview *)));
	if (curPreview == 0) {
	    QMessageBox::warning(0, "Warning", 
				 "Can't create preview window",  
				 QMessageBox::Ok, 
				 QMessageBox::NoButton);
	    return;
	}
	curPreview->show();
	editor = curPreview->pvEdit;
    } else {
	QWidget *anon = new QWidget((QWidget *)curPreview->pvTab);
	QVBoxLayout *anonLayout = new QVBoxLayout(anon, 1, 1, "anonLayout"); 
	editor = new QTextEdit(anon, "pvEdit");
	editor->setReadOnly( TRUE );
	editor->setUndoRedoEnabled( FALSE );
	anonLayout->addWidget(editor);
	curPreview->pvTab->addTab(anon, "Tab");
	curPreview->pvTab->showPage(anon);
    }
    string tabname;
    if (!doc.title.empty()) {
	tabname = doc.title;
    } else {
	tabname = path_getsimple(doc.url);
    }
    if (tabname.length() > 20) {
	tabname = tabname.substr(0, 10) + "..." + 
	    tabname.substr(tabname.length()-10);
    }
    curPreview->pvTab->changeTab(curPreview->pvTab->currentPage(), 
				 QString::fromUtf8(tabname.c_str(), 
						   tabname.length()));

    if (doc.title.empty()) 
	doc.title = path_getsimple(doc.url);
    char datebuf[100];
    datebuf[0] = 0;
    if (!doc.fmtime.empty() || !doc.dmtime.empty()) {
	time_t mtime = doc.dmtime.empty() ? 
	    atol(doc.fmtime.c_str()) : atol(doc.dmtime.c_str());
	struct tm *tm = localtime(&mtime);
	strftime(datebuf, 99, "%F %T", tm);
    }
    string tiptxt = doc.url + string("\n");
    tiptxt += doc.mimetype + " " + string(datebuf) + "\n";
    if (!doc.title.empty())
	tiptxt += doc.title + "\n";
    curPreview->pvTab->setTabToolTip(curPreview->pvTab->currentPage(),
				     QString::fromUtf8(tiptxt.c_str(),
						       tiptxt.length()));

    QStyleSheetItem *item = 
	new QStyleSheetItem(editor->styleSheet(), "termtag" );
    item->setColor("blue");
    item->setFontWeight(QFont::Bold);

    QString str = QString::fromUtf8(rich.c_str(), rich.length());
    stb->message("Loading preview text");
    qApp->processEvents();

    editor->setText(str);
    int para = 0, index = 1;
    if (!termoffsets.empty()) {
	index = (termoffsets.begin())->first;
	LOGDEB(("Set cursor position: para %d, character index %d\n",
		para,index));
	editor->setCursorPosition(0, index);
    }
    editor->ensureCursorVisible();
    editor->getCursorPosition(&para, &index);
    LOGDEB(("PREVIEW len %d paragraphs: %d. Cpos: %d %d\n", 
	    editor->length(), editor->paragraphs(),  para, index));
    stb->clear();
}
