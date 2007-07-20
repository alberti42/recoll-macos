#ifndef lint
static char rcsid[] = "@(#$Id: preview_w.cpp,v 1.25 2007-07-20 11:38:18 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#include <list>
#include <utility>
#ifndef NO_NAMESPACES
using std::pair;
#endif /* NO_NAMESPACES */

#include <qmessagebox.h>
#include <qthread.h>
#include <qvariant.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#if (QT_VERSION < 0x040000)
#include <qtextedit.h>
#include <qprogressdialog.h>
#else
#include <q3textedit.h>
#include <q3progressdialog.h>
#include <q3stylesheet.h>
#endif
#include <qevent.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qapplication.h>
#include <qclipboard.h>

#include "debuglog.h"
#include "pathut.h"
#include "internfile.h"
#include "recoll.h"
#include "plaintorich.h"
#include "smallut.h"
#include "wipedir.h"
#include "cancelcheck.h"
#include "preview_w.h"
#include "guiutils.h"

#if (QT_VERSION < 0x030300)
#define wasCanceled wasCancelled
#endif

void Preview::init()
{
#if 0
    // Couldn't get a small button really in the corner. stays on the left of
    // the button area and looks ugly
    QPixmap px = QPixmap::fromMimeSource("cancel.png");
    QPushButton * bt = new QPushButton(px, "", this);
    bt->setFixedSize(px.size());
#else
    QPushButton * bt = new QPushButton(tr("Close Tab"), this);
#endif

    pvTab->setCornerWidget(bt);

    // signals and slots connections
    connect(searchTextLine, SIGNAL(textChanged(const QString&)), 
	    this, SLOT(searchTextLine_textChanged(const QString&)));
    connect(nextButton, SIGNAL(clicked()), this, SLOT(nextPressed()));
    connect(prevButton, SIGNAL(clicked()), this, SLOT(prevPressed()));
    connect(clearPB, SIGNAL(clicked()), searchTextLine, SLOT(clear()));
    connect(pvTab, SIGNAL(currentChanged(QWidget *)), 
	    this, SLOT(currentChanged(QWidget *)));
    connect(bt, SIGNAL(clicked()), this, SLOT(closeCurrentTab()));

    m_dynSearchActive = false;
    m_canBeep = true;
    m_tabData.push_back(TabData(pvTab->currentPage()));
    m_currentW = 0;
    if (prefs.pvwidth > 100) {
	resize(prefs.pvwidth, prefs.pvheight);
    }
    m_loading = false;
    currentChanged(pvTab->currentPage());
    m_justCreated = true;
}

void Preview::closeEvent(QCloseEvent *e)
{
    LOGDEB(("Preview::closeEvent. m_loading %d\n", m_loading));
    if (m_loading) {
	CancelCheck::instance().setCancel();
	e->ignore();
	return;
    }
    prefs.pvwidth = width();
    prefs.pvheight = height();
    emit previewExposed(this, m_searchId, -1);
    emit previewClosed(this);
    QWidget::closeEvent(e);
}

bool Preview::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() != QEvent::KeyPress) 
	return false;
    
    LOGDEB1(("Preview::eventFilter: keyEvent\n"));
    QKeyEvent *keyEvent = (QKeyEvent *)event;
    if (keyEvent->key() == Qt::Key_Q && 
	(keyEvent->state() & Qt::ControlButton)) {
	recollNeedsExit = 1;
	return true;
    } else if (keyEvent->key() == Qt::Key_Escape) {
	close();
	return true;
    } else if (keyEvent->key() == Qt::Key_Down &&
	       (keyEvent->state() & Qt::ShiftButton)) {
	// LOGDEB(("Preview::eventFilter: got Shift-Up\n"));
	TabData *d = tabDataForCurrent();
	if (d) 
	    emit(showNext(this, m_searchId, d->docnum));
	return true;
    } else if (keyEvent->key() == Qt::Key_Up &&
	       (keyEvent->state() & Qt::ShiftButton)) {
	// LOGDEB(("Preview::eventFilter: got Shift-Down\n"));
	TabData *d = tabDataForCurrent();
	if (d) 
	    emit(showPrev(this, m_searchId, d->docnum));
	return true;
    } else if (keyEvent->key() == Qt::Key_W &&
	       (keyEvent->state() & Qt::ControlButton)) {
	// LOGDEB(("Preview::eventFilter: got ^W\n"));
	closeCurrentTab();
	return true;
    } else if (m_dynSearchActive) {
	if (keyEvent->key() == Qt::Key_F3) {
	    doSearch(searchTextLine->text(), true, false);
	    return true;
	}
	if (target != searchTextLine)
	    return QApplication::sendEvent(searchTextLine, event);
    } else {
	QWidget *tw = pvTab->currentPage();
	QTextEdit *e = 0;
	if (tw)
	    e = (QTextEdit *)tw->child("pvEdit");
	LOGDEB1(("Widget: %p, edit %p, target %p\n", tw, e, target));
	if (e && target == e) {
	    if (keyEvent->key() == Qt::Key_Slash) {
		searchTextLine->setFocus();
		m_dynSearchActive = true;
		return true;
	    } else if (keyEvent->key() == Qt::Key_Space) {
		e->scrollBy(0, e->visibleHeight());
		return true;
	    } else if (keyEvent->key() == Qt::Key_BackSpace) {
		e->scrollBy(0, -e->visibleHeight());
		return true;
	    }
	}
    }

    return false;
}

void Preview::searchTextLine_textChanged(const QString & text)
{
    LOGDEB1(("search line text changed. text: '%s'\n", text.ascii()));
    if (text.isEmpty()) {
	m_dynSearchActive = false;
	//	nextButton->setEnabled(false);
	//	prevButton->setEnabled(false);
	clearPB->setEnabled(false);
    } else {
	m_dynSearchActive = true;
	//	nextButton->setEnabled(true);
	//	prevButton->setEnabled(true);
	clearPB->setEnabled(true);
	doSearch(text, false, false);
    }
}

#if (QT_VERSION >= 0x040000)
#define QTextEdit Q3TextEdit
#define QProgressDialog Q3ProgressDialog
#define QStyleSheetItem Q3StyleSheetItem
#endif

QTextEdit *Preview::getCurrentEditor()
{
    QWidget *tw = pvTab->currentPage();
    QTextEdit *edit = 0;
    if (tw) {
	edit = (QTextEdit*)tw->child("pvEdit");
    }
    return edit;
}

// Perform text search. If next is true, we look for the next match of the
// current search, trying to advance and possibly wrapping around. If next is
// false, the search string has been modified, we search for the new string, 
// starting from the current position
void Preview::doSearch(const QString &_text, bool next, bool reverse, 
		       bool wordOnly)
{
    LOGDEB(("Preview::doSearch: text [%s] txtlen %d next %d rev %d word %d\n", 
	    (const char *)_text.utf8(), _text.length(), int(next), 
	    int(reverse), int(wordOnly)));
    QString text = _text;

    bool matchCase = matchCheck->isChecked();
    QTextEdit *edit = getCurrentEditor();
    if (edit == 0) {
	// ??
	return;
    }

    if (text.isEmpty()) {
#ifdef QT_SCROLL_TO_ANCHOR_BUG
	text = QString::fromUtf8(firstTermBeacon);
	matchCase = false;
#else
#error "Cycling without beacons needs coding"
#endif
    }

    // If next is false, the user added characters to the current
    // search string.  We need to reset the cursor position to the
    // start of the previous match, else incremental search is going
    // to look for the next occurrence instead of trying to lenghten
    // the current match
    if (!next) {
	int ps, is, pe, ie;
	edit->getSelection(&ps, &is, &pe, &ie);
	if (is > 0)
	    is--;
	else if (ps > 0)
	    ps--;
	LOGDEB(("Preview::doSearch: setting cursor to %d %d\n", ps, is));
	edit->setCursorPosition(ps, is);
    }

    LOGDEB(("Preview::doSearch: first find call\n"));
    bool found = edit->find(text, matchCase, wordOnly, !reverse, 0, 0);
    LOGDEB(("Preview::doSearch: first find call return\n"));
    // If not found, try to wrap around. 
    if (!found && next) { 
	LOGDEB(("Preview::doSearch: wrapping around\n"));
	int mspara, msindex;
	if (reverse) {
	    mspara = edit->paragraphs();
	    msindex = edit->paragraphLength(mspara);
	} else {
	    mspara = msindex = 0;
	}
	LOGDEB(("Preview::doSearch: 2nd find call\n"));
	found = edit->find(text,matchCase, false, !reverse, &mspara, &msindex);
	LOGDEB(("Preview::doSearch: 2nd find call return\n"));
    }

    if (found) {
	m_canBeep = true;
    } else {
	if (m_canBeep)
	    QApplication::beep();
	m_canBeep = false;
    }
    LOGDEB(("Preview::doSearch: return\n"));
}

void Preview::nextPressed()
{
    doSearch(searchTextLine->text(), true, false);
}

void Preview::prevPressed()
{
    doSearch(searchTextLine->text(), true, true);
}

// Called when user clicks on tab
void Preview::currentChanged(QWidget * tw)
{
    QWidget *edit = (QWidget *)tw->child("pvEdit");
    m_currentW = tw;
    LOGDEB1(("Preview::currentChanged(). Editor: %p\n", edit));
    
    if (edit == 0) {
	LOGERR(("Editor child not found\n"));
	return;
    }
    edit->setFocus();
    // Connect doubleclick but cleanup first just in case this was
    // already connected.
    disconnect(edit, SIGNAL(doubleClicked(int, int)), this, 0);
    connect(edit, SIGNAL(doubleClicked(int, int)), 
	    this, SLOT(textDoubleClicked(int, int)));
#if (QT_VERSION >= 0x040000)
    connect(edit, SIGNAL(selectionChanged()), this, SLOT(selecChanged()));
#endif
    tw->installEventFilter(this);
    edit->installEventFilter(this);
    TabData *d = tabDataForCurrent();
    if (d) 
	emit(previewExposed(this, m_searchId, d->docnum));
}

#if (QT_VERSION >= 0x040000)
// I have absolutely no idea why this nonsense is needed to get
// q3textedit to copy to x11 primary selection when text is
// selected. This used to be automatic, and, looking at the code, it
// should happen inside q3textedit (the code here is copied from the
// private copyToClipboard method). To be checked again with a later
// qt version.
void Preview::selecChanged()
{
    LOGDEB1(("Selection changed\n"));
    if (!m_currentW)
	return;
    QTextEdit *edit = (QTextEdit *)m_currentW->child("pvEdit");
    if (edit == 0) {
	LOGERR(("Editor child not found\n"));
	return;
    }
    QClipboard *clipboard = QApplication::clipboard();
    if (edit->hasSelectedText()) {
	LOGDEB1(("Copying [%s] to primary selection.Clipboard sel supp: %d\n", 
		(const char *)edit->selectedText().ascii(),
		clipboard->supportsSelection()));
        disconnect(QApplication::clipboard(), SIGNAL(selectionChanged()), 
		   edit, 0);
	clipboard->setText(edit->selectedText(), QClipboard::Selection);
        connect(QApplication::clipboard(), SIGNAL(selectionChanged()),
		edit, SLOT(clipboardChanged()));
    }
}
#else 
void Preview::selecChanged(){}
#endif

void Preview::textDoubleClicked(int, int)
{
    LOGDEB2(("Preview::textDoubleClicked\n"));
    if (!m_currentW)
	return;
    QTextEdit *edit = (QTextEdit *)m_currentW->child("pvEdit");
    if (edit == 0) {
	LOGERR(("Editor child not found\n"));
	return;
    }
    if (edit->hasSelectedText())
	emit(wordSelect(edit->selectedText()));
}

void Preview::closeCurrentTab()
{
    LOGDEB(("Preview::closeCurrentTab: m_loading %d\n", m_loading));
    if (m_loading) {
	CancelCheck::instance().setCancel();
	return;
    }
    if (pvTab->count() > 1) {
	QWidget *tw = pvTab->currentPage();
	if (!tw) 
	    return;
	pvTab->removePage(tw);
	// Have to remove from tab data list
	for (list<TabData>::iterator it = m_tabData.begin(); 
	     it != m_tabData.end(); it++) {
	    if (it->w == tw) {
		m_tabData.erase(it);
		return;
	    }
	}
    } else {
	close();
    }
}

QTextEdit *Preview::addEditorTab()
{
    QWidget *anon = new QWidget((QWidget *)pvTab);
    QVBoxLayout *anonLayout = new QVBoxLayout(anon, 1, 1, "anonLayout"); 
    QTextEdit *editor = new QTextEdit(anon, "pvEdit");
    editor->setReadOnly(TRUE);
    editor->setUndoRedoEnabled(FALSE );
    anonLayout->addWidget(editor);
    pvTab->addTab(anon, "Tab");
    pvTab->showPage(anon);
    m_tabData.push_back(TabData(anon));
    return editor;
}

void Preview::setCurTabProps(const string &fn, const Rcl::Doc &doc,
			     int docnum)
{
    QString title;
    map<string,string>::const_iterator meta_it;
    if ((meta_it = doc.meta.find("title")) != doc.meta.end()) {
	    title = QString::fromUtf8(meta_it->second.c_str(), 
				      meta_it->second.length());
    }
    if (title.length() > 20) {
	title = title.left(10) + "..." + title.right(10);
    }
    QWidget *w = pvTab->currentPage();
    pvTab->changeTab(w, title);

    char datebuf[100];
    datebuf[0] = 0;
    if (!doc.fmtime.empty() || !doc.dmtime.empty()) {
	time_t mtime = doc.dmtime.empty() ? 
	    atol(doc.fmtime.c_str()) : atol(doc.dmtime.c_str());
	struct tm *tm = localtime(&mtime);
	strftime(datebuf, 99, "%Y-%m-%d %H:%M:%S", tm);
    }
    LOGDEB(("Doc.url: [%s]\n", doc.url.c_str()));
    string url;
    printableUrl(doc.url, url);
    string tiptxt = url + string("\n");
    tiptxt += doc.mimetype + " " + string(datebuf) + "\n";
    if (meta_it != doc.meta.end() && !meta_it->second.empty())
	tiptxt += meta_it->second + "\n";
    pvTab->setTabToolTip(w,QString::fromUtf8(tiptxt.c_str(), tiptxt.length()));

    for (list<TabData>::iterator it = m_tabData.begin(); 
	 it != m_tabData.end(); it++) {
	if (it->w == w) {
	    it->fn = fn;
	    it->ipath = doc.ipath;
	    it->docnum = docnum;
	    break;
	}
    }
}

TabData *Preview::tabDataForCurrent()
{
    QWidget *w = pvTab->currentPage();
    if (w == 0)
	return 0;
    for (list<TabData>::iterator it = m_tabData.begin(); 
	 it != m_tabData.end(); it++) {
	if (it->w == w) {
	    return &(*it);
	}
    }
    return 0;
}

bool Preview::makeDocCurrent(const string &fn, size_t sz, 
			     const Rcl::Doc& doc, int docnum, bool sametab)
{
    LOGDEB(("Preview::makeDocCurrent: %s\n", fn.c_str()));
    for (list<TabData>::iterator it = m_tabData.begin(); 
	 it != m_tabData.end(); it++) {
	LOGDEB2(("Preview::makeFileCurrent: compare to w %p, file %s\n", 
		 it->w, it->fn.c_str()));
	if (!it->fn.compare(fn) && !it->ipath.compare(doc.ipath)) {
	    pvTab->showPage(it->w);
	    return true;
	}
    }
    // if just created the first tab was created during init
    if (!sametab && !m_justCreated && !addEditorTab()) {
	return false;
    }
    m_justCreated = false;
    if (!loadFileInCurrentTab(fn, sz, doc, docnum)) {
	closeCurrentTab();
	return false;
    }
    return true;
}

/*
  Code for loading a file into an editor window. The operations that
  we call have no provision to indicate progression, and it would be
  complicated or impossible to modify them to do so (Ie: for external 
  format converters).

  We implement a complicated and ugly mechanism based on threads to indicate 
  to the user that the app is doing things: lengthy operations are done in 
  threads and we update a progress indicator while they proceed (but we have 
  no estimate of their total duration).
  
  An auxiliary thread object is used for short waits. Another option would be 
  to use signals/slots and return to the event-loop instead, but this would
  be even more complicated, and we probably don't want the user to click on
  things during this time anyway.

  It might be possible, but complicated (need modifications in
  handler) to implement a kind of bucket brigade, to have the
  beginning of the text displayed faster
*/

/* A thread to to the file reading / format conversion */
class LoadThread : public QThread {
    int *statusp;
    Rcl::Doc *out;
    string filename;
    string ipath;
    string *mtype;
    string tmpdir;
    int loglevel;
 public: 
    string missing;
    LoadThread(int *stp, Rcl::Doc *odoc, string fn, string ip, string *mt) 
	: statusp(stp), out(odoc), filename(fn), ipath(ip), mtype(mt) 
	{
	    loglevel = DebugLog::getdbl()->getlevel();
	}
    ~LoadThread() {
	if (tmpdir.length()) {
	    wipedir(tmpdir);
	    rmdir(tmpdir.c_str());
	}
    }
    virtual void run() {
	DebugLog::getdbl()->setloglevel(loglevel);
	string reason;
	if (!maketmpdir(tmpdir, reason)) {
	    QMessageBox::critical(0, "Recoll",
				  Preview::tr("Cannot create temporary directory"));
	    LOGERR(("Preview: %s\n", reason.c_str()));
	    *statusp = -1;
	    return;
	}
	struct stat st;
	if (stat(filename.c_str(), &st) < 0) {
	    LOGERR(("Preview: can't stat [%s]\n", filename.c_str()));
	    QMessageBox::critical(0, "Recoll",
				  Preview::tr("File does not exist"));
	    *statusp = -1;
	    return;
	}
	    
	FileInterner interner(filename, &st, rclconfig, tmpdir, mtype);
	try {
	    FileInterner::Status ret = interner.internfile(*out, ipath);
	    if (ret == FileInterner::FIDone || ret == FileInterner::FIAgain) {
		// FIAgain is actually not nice here. It means that the record
		// for the *file* of a multidoc was selected. Actually this
		// shouldn't have had a preview link at all, but we don't know
		// how to handle it now. Better to show the first doc than
		// a mysterious error. Happens when the file name matches a
		// a search term of course.
		*statusp = 0;
	    } else {
		out->mimetype = interner.getMimetype();
		interner.getMissingExternal(missing);
		*statusp = -1;
	    }
	} catch (CancelExcept) {
	    *statusp = -1;
	}
    }
};

/* A thread to convert to rich text (mark search terms) */
class ToRichThread : public QThread {
    string &in;
    const HiliteData &hdata;
    QString &out;
    int loglevel;
 public:
    ToRichThread(string &i, const HiliteData& hd, QString &o) 
	: in(i), hdata(hd), out(o)
    {
	    loglevel = DebugLog::getdbl()->getlevel();
    }
    virtual void run()
    {
	DebugLog::getdbl()->setloglevel(loglevel);
	string rich;
	try {
	    plaintorich(in, rich, hdata, false, true);
	} catch (CancelExcept) {
	}
	out = QString::fromUtf8(rich.c_str(), rich.length());
    }
};

/* A thread to implement short waiting. There must be a better way ! */
class WaiterThread : public QThread {
    int ms;
 public:
    WaiterThread(int millis) : ms(millis) {}
    virtual void run() {
	msleep(ms);
    }
};

#define CHUNKL 50*1000
#ifndef MIN
#define MIN(A,B) ((A)<(B)?(A):(B))
#endif
class LoadGuard {
    bool *m_bp;
public:
    LoadGuard(bool *bp) {m_bp = bp ; *m_bp = true;}
    ~LoadGuard() {*m_bp = false; CancelCheck::instance().setCancel(false);}
};

bool Preview::loadFileInCurrentTab(string fn, size_t sz, const Rcl::Doc &idoc,
				   int docnum)
{
    if (m_loading) {
	LOGERR(("ALready loading\n"));
	return false;
    }

    LoadGuard guard(&m_loading);
    CancelCheck::instance().setCancel(false);

    Rcl::Doc doc = idoc;

    if (doc.meta["title"].empty()) 
	doc.meta["title"] = path_getsimple(doc.url);

    setCurTabProps(fn, doc, docnum);

    char csz[20];
    sprintf(csz, "%lu", (unsigned long)sz);
    QString msg = QString("Loading: %1 (size %2 bytes)")
	.arg(QString::fromLocal8Bit(fn.c_str()))
	.arg(csz);

    // Create progress dialog and aux objects
    const int nsteps = 20;
    QProgressDialog progress(msg, tr("Cancel"), nsteps, this, "Loading", FALSE);
    progress.setMinimumDuration(2000);
    WaiterThread waiter(100);

    // Load and convert file
    Rcl::Doc fdoc;
    // Need to setup config to retrieve possibly local parameters
    rclconfig->setKeyDir(path_getfather(fn));
    int status = 1;
    LoadThread lthr(&status, &fdoc, fn, doc.ipath, &doc.mimetype);
    lthr.start();
    int prog;
    for (prog = 1;;prog++) {
	waiter.start();
	waiter.wait();
#if (QT_VERSION < 0x040000)
	if (lthr.finished())
	    break;
#else
	if (lthr.isFinished())
	    break;
#endif
	progress.setProgress(prog , prog <= nsteps-1 ? nsteps : prog+1);
	qApp->processEvents();
	if (progress.wasCanceled()) {
	    CancelCheck::instance().setCancel();
	}
	if (prog >= 5)
	    sleep(1);
    }

    LOGDEB(("LoadFileInCurrentTab: after file load: cancel %d status %d"
	    " text length %d\n", 
	    CancelCheck::instance().cancelState(), status, fdoc.text.length()));

    if (CancelCheck::instance().cancelState())
	return false;
    if (status != 0) {
	QString explain;
	if (!lthr.missing.empty()) {
	    explain = QString::fromAscii("<br>") +
		tr("Missing helper program: ") + 
		QString::fromLocal8Bit(lthr.missing.c_str());
	}
	QMessageBox::warning(0, "Recoll",
			     tr("Can't turn doc into internal "
				"representation for ") +
			     fdoc.mimetype.c_str() + explain);
	return false;
    }
    // Reset config just in case.
    rclconfig->setKeyDir("");

    // Create preview text: highlight search terms (if not too big):
    QString richTxt;
    bool highlightTerms = fdoc.text.length() < 2000 * 1024;
    if (highlightTerms) {
	progress.setLabelText(tr("Creating preview text"));
	ToRichThread rthr(fdoc.text, m_hData, richTxt);
	rthr.start();

	for (;;prog++) {
	    waiter.start();	waiter.wait();
#if (QT_VERSION < 0x040000)
	if (rthr.finished())
	    break;
#else
	if (rthr.isFinished())
	    break;
#endif
	    progress.setProgress(prog , prog <= nsteps-1 ? nsteps : prog+1);
	    qApp->processEvents();
	    if (progress.wasCanceled()) {
		CancelCheck::instance().setCancel();
	    }
	    if (prog >= 5)
		sleep(1);
	}

	// Conversion to rich text done
	if (CancelCheck::instance().cancelState()) {
	    if (richTxt.length() == 0) {
		// We cant call closeCurrentTab here as it might delete
		// the object which would be a nasty surprise to our
		// caller.
		return false;
	    } else {
		richTxt += "<b>Cancelled !</b>";
	    }
	}
    } else {
	richTxt = QString::fromUtf8(fdoc.text.c_str(), fdoc.text.length());
    }

    int pos = richTxt.find(QString::fromUtf8(firstTermBeacon));
    bool hasAnchors = (pos != -1);
    LOGDEB(("LoadFileInCurrentTab: rich: cancel %d txtln %d, hasAnchors %d (pos %d)\n", 
	    CancelCheck::instance().cancelState(), richTxt.length(), hasAnchors, pos));

    // Load into editor
    // Do it in several chunks 
    QTextEdit *editor = getCurrentEditor();
    editor->setText("");
    if (highlightTerms) {
	QStyleSheetItem *item = 
	    new QStyleSheetItem(editor->styleSheet(), "termtag" );
	item->setColor("blue");
	item->setFontWeight(QFont::Bold);
    }

    prog = 2 * nsteps / 3;
    progress.setLabelText(tr("Loading preview text into editor"));
    qApp->processEvents();
    int l = 0;
    for (int pos = 0; pos < (int)richTxt.length(); pos += l, prog++) {
	progress.setProgress(prog , prog <= nsteps-1 ? nsteps : prog+1);
	qApp->processEvents();
	
	l = MIN(CHUNKL, richTxt.length() - pos);
	// Avoid breaking inside a tag. Our tags are short (ie: <br>)
	if (pos + l != (int)richTxt.length()) {
	    for (int i = -15; i < 0; i++) {
		if (richTxt[pos+l+i] == '<') {
		    l = l+i;
		    break;
		}
	    }
	}
	editor->append(richTxt.mid(pos, l));
	// Stay at top
	if (pos < 5) {
	    editor->setCursorPosition(0,0);
	    editor->ensureCursorVisible();
	}

	if (progress.wasCanceled()) {
            editor->append("<b>Cancelled !</b>");
	    LOGDEB(("LoadFileInCurrentTab: cancelled in editor load\n"));
	    break;
	}
    }
    progress.close();

    if (searchTextLine->text().length() != 0) {
	m_canBeep = true;
	doSearch(searchTextLine->text(), true, false);
    } else {
	if (hasAnchors) {
	    QString aname = QString::fromUtf8(termAnchorName(1).c_str());
	    LOGDEB2(("Call scrolltoanchor(%s)\n", (const char *)aname.utf8()));
	    editor->scrollToAnchor(aname);
	    // The q3textedit version of find is slow to the point of being
	    // unusable (plus it does not always work)
#if (QT_VERSION < 0x040000)
#ifdef QT_SCROLL_TO_ANCHOR_BUG
	    bool ocanbeep = m_canBeep;
	    m_canBeep = false;
	    QString empty;
	    // doSearch(_text, next, reverse, wordOnly)
	    doSearch(empty, true, false, false);
	    m_canBeep = ocanbeep;
#endif
#endif // (QT_VERSION < 0x040000)
	}
    }
    // Enter document in document history
    g_dynconf->enterDoc(fn, doc.ipath);

    editor->setFocus();
    emit(previewExposed(this, m_searchId, docnum));
    LOGDEB(("LoadFileInCurrentTab: returning true\n"));
    return true;
}
