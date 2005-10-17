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

#include <utility>
using std::pair;

#include <qmessagebox.h>
#include <qcstring.h>
#include <qtabwidget.h>

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

#ifndef MIN
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#endif

// Number of abstracts in a result page. This will avoid scrollbars
// with the default window size and font, most of the time.
static const int respagesize = 8;


void RecollMain::init()
{
    curPreview = 0;
}

// We want to catch ^Q everywhere to mean quit.
bool RecollMain::eventFilter( QObject * target, QEvent * event )
{
    if (event->type() == QEvent::KeyPress) {
	QKeyEvent *keyEvent = (QKeyEvent *)event;
	if (keyEvent->key() == Key_Q && (keyEvent->state() & ControlButton)) {
	    recollNeedsExit = 1;
	}
    }
    return QWidget::eventFilter(target, event);
}

void RecollMain::fileExit()
{
    LOGDEB1(("RecollMain: fileExit\n"));
    exit(0);
}

// Misnomer. This is called on a 100ms timer and actually checks for different 
// things apart from a need to exit
void RecollMain::checkExit()
{
    // Check if indexing thread done
    if (indexingstatus) {
	indexingstatus = false;
	// Make sure we reopen the db to get the results.
	LOGINFO(("Indexing done: closing query database\n"));
	rcldb->close();
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
    LOGDEB(("Executing: '%s'\n", ncmd.c_str()));
    system(ncmd.c_str());
}


// Display preview for the selected document, and highlight entry. The
// paragraph number is doc number in window + 1
void RecollMain::reslistTE_clicked(int par, int car)
{
    LOGDEB(("RecollMain::reslistTE_clicked: par %d, char %d\n", par, car));
    if (reslist_winfirst == -1)
	return;

    Rcl::Doc doc;
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

    if (!rcldb->getDoc(reslist_winfirst + reldocnum, doc, 0)) {
	QMessageBox::warning(0, "Recoll",
			     QString("Can't retrieve document from database"));
	return;
    }
	
    // Go to the file system to retrieve / convert the document text
    // for preview:
    string fn = urltolocalpath(doc.url);
    Rcl::Doc fdoc;
    FileInterner interner(fn, rclconfig, tmpdir);
    if (interner.internfile(fdoc, doc.ipath) != FileInterner::FIDone) {
	QMessageBox::warning(0, "Recoll",
			     QString("Can't turn doc into internal rep ") +
			     doc.mimetype.c_str());
	return;
    }
    list<string> terms;
    rcldb->getQueryTerms(terms);
    list<pair<int, int> > termoffsets;
    string rich = plaintorich(fdoc.text, terms, termoffsets);

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
    if (!doc.mtime.empty()) {
	time_t mtime = atol(doc.mtime.c_str());
	struct tm *tm = localtime(&mtime);
	strftime(datebuf, 99, "%F %T", tm);
    }
    string tiptxt = doc.url + string("\n");
    tiptxt += doc.mimetype + " " 
	+ (doc.mtime.empty() ? "\n" : string(datebuf) + "\n");
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


// User asked to start query. Run it and call listNextPB_clicked to display
// first page of results
void RecollMain::queryText_returnPressed()
{
    LOGDEB(("RecollMain::queryText_returnPressed()\n"));
    if (!rcldb->isopen()) {
	string dbdir;
	if (rclconfig->getConfParam(string("dbdir"), dbdir) == 0) {
	    QMessageBox::critical(0, "Recoll",
				  QString("No db directory in configuration"));
	    exit(1);
	}
	dbdir = path_tildexpand(dbdir);
	if (!rcldb->open(dbdir, Rcl::Db::DbRO)) {
	    QMessageBox::information(0, "Recoll",
				     QString("Could not open database in ") + 
				     QString(dbdir) + " wait for indexing " +
				     "to complete?");
	    return;
	}
    }
    if (stemlang.empty()) {
	string param;
	if (rclconfig->getConfParam("querystemming", param))
	    dostem = ConfTree::stringToBool(param);
	else
	    dostem = false;
	if (!rclconfig->getConfParam("querystemminglanguage", stemlang))
	    stemlang = "english";
    }

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
    fprintf(stderr, "listNextPB_clicked\n");
    if (!rcldb)
	return;
    int percent;
    Rcl::Doc doc;
    rcldb->getDoc(0, doc, &percent);
    int resCnt = rcldb->getResCnt();
    fprintf(stderr, "listNextPB_clicked rescnt\n");
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
	    sprintf(line, "<p><b>Displaying results %d-%d out of %d</b><br>",
		    reslist_winfirst+1, reslist_winfirst+last, resCnt);
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
	if (!doc.mtime.empty()) {
	    time_t mtime = atol(doc.mtime.c_str());
	    struct tm *tm = localtime(&mtime);
	    strftime(datebuf, 99, "<i>Modified:</i>&nbsp;%F&nbsp;%T", tm);
	}
	string abst = stripMarkup(doc.abstract);
	LOGDEB(("Abstract: {%s}\n", abst.c_str()));
	string result = "<p>" + 
	    string(perbuf) + " <b>" + doc.title + "</b><br>" +
	    doc.mimetype + "&nbsp;" +
	    (!doc.mtime.empty() ? string(datebuf) + "<br>" : string("<br>")) +
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
	// Display preview for 1st doc in list
	// reslistTE_clicked(1, 0);
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



#include "advsearch.h"

advsearch *asearchform;

void RecollMain::advSearchPB_clicked()
{
    if (asearchform == 0) {
	// Couldn't find way to have a normal wm frame
	asearchform = new advsearch(this, "Advanced search", FALSE,
				    WStyle_Customize | WStyle_NormalBorder | 
				    WStyle_Title | WStyle_SysMenu);
	asearchform->setSizeGripEnabled(FALSE);
	connect(asearchform, SIGNAL(startSearch(AdvSearchData)), 
		this, SLOT(startAdvSearch(AdvSearchData)));
	asearchform->show();
    } else {
	asearchform->show();
    }
}

void RecollMain::startAdvSearch(AdvSearchData sdata)
{
    LOGDEB(("RecollMain::startAdvSearch\n"));
    LOGDEB((" allwords: %s\n", sdata.allwords.c_str()));
    LOGDEB((" phrase: %s\n", sdata.phrase.c_str()));
    LOGDEB((" orwords: %s\n", sdata.orwords.c_str()));
    LOGDEB((" nowords: %s\n", sdata.nowords.c_str()));
    string ft;
    for (list<string>::iterator it = sdata.filetypes.begin(); 
	 it != sdata.filetypes.end(); it++) {
	ft += *it + " ";
    }
    if (!ft.empty()) 
	LOGDEB(("Searched file types: %s\n", ft.c_str()));
    if (!sdata.topdir.empty())
	LOGDEB(("Restricted to: %s\n", sdata.topdir.c_str()));

}



