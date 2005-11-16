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
#include <unistd.h>

#include <utility>
using std::pair;

#include <qmessagebox.h>
#include <qprogressdialog.h>
#include <qthread.h>

#include "debuglog.h"
#include "pathut.h"
#include "internfile.h"
#include "recoll.h"
#include "plaintorich.h"

void Preview::init()
{
    connect(pvTab, SIGNAL(currentChanged(QWidget *)), 
	    this, SLOT(currentChanged(QWidget *)));
    searchTextLine->installEventFilter(this);
    dynSearchActive = false;
    canBeep = true;
}

void Preview::closeEvent(QCloseEvent *e)
{
    emit previewClosed(this);
    QWidget::closeEvent(e);
}

extern int recollNeedsExit;

bool Preview::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() != QEvent::KeyPress) 
	return QWidget::eventFilter(target, event);
    
    LOGDEB1(("Preview::eventFilter: keyEvent\n"));
    QKeyEvent *keyEvent = (QKeyEvent *)event;
    if (keyEvent->key() == Key_Q && (keyEvent->state() & ControlButton)) {
	recollNeedsExit = 1;
	return true;
    } else if (keyEvent->key() ==Key_W &&(keyEvent->state() & ControlButton)) {
	// LOGDEB(("Preview::eventFilter: got ^W\n"));
	closeCurrentTab();
	return true;
    } else if (dynSearchActive) {
	if (keyEvent->key() == Key_F3) {
	    doSearch(true, false);
	    return true;
	}
	if (target != searchTextLine)
	    return QApplication::sendEvent(searchTextLine, event);
    } else {
	QWidget *tw = pvTab->currentPage();
	QWidget *e = 0;
	if (tw)
	    e = (QTextEdit *)tw->child("pvEdit");
	LOGDEB1(("Widget: %p, edit %p, target %p\n", tw, e, target));
	if (e && target == tw && keyEvent->key() == Key_Slash) {
	    searchTextLine->setFocus();
	    dynSearchActive = true;
	    return true;
	}
    }

    return QWidget::eventFilter(target, event);
}

void Preview::searchTextLine_textChanged(const QString & text)
{
    LOGDEB1(("search line text changed. text: '%s'\n", text.ascii()));
    if (text.isEmpty()) {
	dynSearchActive = false;
	nextButton->setEnabled(false);
	prevButton->setEnabled(false);
	clearPB->setEnabled(false);
    } else {
	dynSearchActive = true;
	nextButton->setEnabled(true);
	prevButton->setEnabled(true);
	clearPB->setEnabled(true);
	doSearch(false, false);
    }
}

QTextEdit * Preview::getCurrentEditor()
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
void Preview::doSearch(bool next, bool reverse)
{
    LOGDEB1(("Preview::doSearch: next %d rev %d\n", int(next), int(reverse)));
    QTextEdit *edit = getCurrentEditor();
    if (edit == 0) {
	// ??
	return;
    }
    bool matchCase = matchCheck->isChecked();
    int mspara, msindex, mepara, meindex;
    edit->getSelection(&mspara, &msindex, &mepara, &meindex);
    if (mspara == -1)
	mspara = msindex = mepara = meindex = 0;

    if (next) {
	// We search again, starting from the current match
	if (reverse) {
	    // when searching backwards, have to move back one char
	    if (msindex > 0)
		msindex --;
	    else if (mspara > 0) {
		mspara --;
		msindex = edit->paragraphLength(mspara);
	    }
	} else {
	    // Forward search: start from end of selection
	    mspara = mepara;
	    msindex = meindex;
	    LOGDEB1(("New para: %d index %d\n", mspara, msindex));
	}
    }

    bool found = edit->find(searchTextLine->text(), matchCase, false, 
			      !reverse, &mspara, &msindex);

    if (!found && next && true) { // need a 'canwrap' test here
	if (reverse) {
	    mspara = edit->paragraphs();
	    msindex = edit->paragraphLength(mspara);
	} else {
	    mspara = msindex = 0;
	}
	found = edit->find(searchTextLine->text(), matchCase, false, 
			     !reverse, &mspara, &msindex);
    }

    if (found) {
	canBeep = true;
    } else {
	if (canBeep)
	    QApplication::beep();
	canBeep = false;
    }
}


void Preview::nextPressed()
{
    doSearch(true, false);
}


void Preview::prevPressed()
{
    doSearch(true, true);
}


void Preview::currentChanged(QWidget * tw)
{
    QObject *o = tw->child("pvEdit");
    LOGDEB1(("Preview::currentChanged(). Edit %p\n", o));
    
    if (o == 0) {
	LOGERR(("Editor child not found\n"));
    } else {
	tw->installEventFilter(this);
	o->installEventFilter(this);
	((QWidget*)o)->setFocus();
    }
}


void Preview::closeCurrentTab()
{
    if (pvTab->count() > 1) {
	QWidget *tw = pvTab->currentPage();
	if (tw) 
	    pvTab->removePage(tw);
    } else {
	close();
    }
}


QTextEdit * Preview::addEditorTab()
{
    QWidget *anon = new QWidget((QWidget *)pvTab);
    QVBoxLayout *anonLayout = new QVBoxLayout(anon, 1, 1, "anonLayout"); 
    QTextEdit *editor = new QTextEdit(anon, "pvEdit");
    editor->setReadOnly( TRUE );
    editor->setUndoRedoEnabled( FALSE );
    anonLayout->addWidget(editor);
    pvTab->addTab(anon, "Tab");
    pvTab->showPage(anon);
    return editor;
}

void Preview::setCurTabProps(const Rcl::Doc &doc)
{
    QString title = QString::fromUtf8(doc.title.c_str(), 
				      doc.title.length());
    if (title.length() > 20) {
	title = title.left(10) + "..." + title.right(10);
    }
    pvTab->changeTab(pvTab->currentPage(), title);

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
    pvTab->setTabToolTip(pvTab->currentPage(),
			 QString::fromUtf8(tiptxt.c_str(), tiptxt.length()));
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

  No cancel button is implemented, but this could conceivably be done
*/

/* A thread to to the file reading / format conversion */
class LoadThread : public QThread {
    int *statusp;
    Rcl::Doc *out;
    string filename;
    string ipath;
    string *mtype;

 public: 
    LoadThread(int *stp, Rcl::Doc *odoc, string fn, string ip, string *mt) 
	: statusp(stp), out(odoc), filename(fn), ipath(ip), mtype(mt) 
    {}
   virtual void run() 
   {
       FileInterner interner(filename, rclconfig, tmpdir, mtype);
       if (interner.internfile(*out, ipath) != FileInterner::FIDone) {
	   *statusp = -1;
       } else {
	   *statusp = 0;
       }
   }
};

/* A thread to convert to rich text (mark search terms) */
class ToRichThread : public QThread {
    string &in;
    list<string> &terms;
    list<pair<int, int> > &termoffsets;
    QString &out;
 public:
    ToRichThread(string &i, list<string> &trms, 
		 list<pair<int, int> > &toffs, QString &o) 
	: in(i), terms(trms), termoffsets(toffs), out(o)
    {}
    virtual void run()
    {
	string rich = plaintorich(in, terms, termoffsets);
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

void Preview::loadFileInCurrentTab(string fn, size_t sz, const Rcl::Doc &idoc)
{
    Rcl::Doc doc = idoc;

    if (doc.title.empty()) 
	doc.title = path_getsimple(doc.url);

    setCurTabProps(doc);

    char csz[20];
    sprintf(csz, "%lu", (unsigned long)sz);
    QString msg = QString("Loading: %1 (size %2 bytes)")
	.arg(fn)
	.arg(csz);

    // Create progress dialog and aux objects
    const int nsteps = 10;
    QProgressDialog progress( msg, "", nsteps, this, "Loading", TRUE );
    QPushButton *pb = new QPushButton("", this);
    pb->setEnabled(false);
    progress.setCancelButton(pb);
    progress.setMinimumDuration(1000);
    WaiterThread waiter(100);

    // Load and convert file
    Rcl::Doc fdoc;
    int status = 1;
    LoadThread lthr(&status, &fdoc, fn, doc.ipath, &doc.mimetype);
    lthr.start();
    int i;
    for (i = 1;;i++) {
	waiter.start();
	waiter.wait();
	if (lthr.finished())
	    break;
	progress.setProgress(i , i <= nsteps-1 ? nsteps : i+1);
	qApp->processEvents();
	if (i >= 5)
	    sleep(1);
    }
    if (status != 0) {
	QMessageBox::warning(0, "Recoll",
			     tr("Can't turn doc into internal rep for ") +
			     doc.mimetype.c_str());
	return;
    }
    LOGDEB(("Load file done\n"));

    // Highlight search terms:
    progress.setLabelText(tr("Creating preview text"));
    list<string> terms;
    rcldb->getQueryTerms(terms);
    list<pair<int, int> > termoffsets;
    QString str;
    ToRichThread rthr(fdoc.text, terms, termoffsets, str);
    rthr.start();

    for (;;i++) {
	waiter.start();
	waiter.wait();
	if (rthr.finished())
	    break;
	progress.setProgress(i , i <= nsteps-1 ? nsteps : i+1);
	qApp->processEvents();
	if (i >= 5)
	    sleep(1);
    }
    LOGDEB(("Plaintorich done\n"));

    // Load into editor
    QTextEdit *editor = getCurrentEditor();
    QStyleSheetItem *item = 
	new QStyleSheetItem(editor->styleSheet(), "termtag" );
    item->setColor("blue");
    item->setFontWeight(QFont::Bold);

    progress.setLabelText(tr("Loading preview text into editor"));
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
}
