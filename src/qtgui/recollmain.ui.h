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

#include <qmessagebox.h>
#include <qcstring.h>

#include "rcldb.h"
#include "rclconfig.h"
#include "debuglog.h"
#include "mimehandler.h"
#include "pathut.h"
#include "recoll.h"

void RecollMain::fileExit()
{
    LOGDEB(("RecollMain: fileExit\n"));
    exit(0);
}

// Misnomer. This is called on a 100ms timer and actually checks for different 
// things apart from a need to exit
void RecollMain::checkExit()
{
    if (indexingstatus) {
	indexingstatus = false;
	// Make sure we reopen the db to get the results.
	fprintf(stderr, "Indexing done: closing query database\n");
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

static string plaintorich(const string &in)
{
    string out = "<qt><head><title></title></head><body><p>";
    for (unsigned int i = 0; i < in.length() ; i++) {
	if (in[i] == '\n') {
	    out += "<br>";
	} else {
	    out += in[i];
	}
    }
    return out;
}

static string urltolocalpath(string url)
{
    return url.substr(7, string::npos);
}

// Use external viewer to display file
void RecollMain::reslistTE_doubleClicked(int par, int)
{
    //    restlistTE_clicked(par, car);
    Rcl::Doc doc;
    int reldocnum =  par - 1;
    if (!rcldb->getDoc(reslist_winfirst + reldocnum, doc, 0))
	return;
    
    // Look for appropriate viewer
    string cmd = getMimeViewer(doc.mimetype, rclconfig->getMimeConf());
    if (cmd.length() == 0) {
	QMessageBox::warning(0, "Recoll", QString("No viewer for mime type ") +
			     doc.mimetype.c_str());
	return;
    }

    string fn = urltolocalpath(doc.url);
    // substitute 
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
		ncmd += doc.url;
	    if (*it1 == 'f')
		ncmd += fn;
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

    // If same doc, don't bother redisplaying
    if (reslist_current == par - 1)
	return;

    Rcl::Doc doc;
    if (reslist_current != -1) {
	QColor color("white");
	reslistTE->setParagraphBackgroundColor(reslist_current+1, color);
    }
    QColor color("lightblue");
    reslistTE->setParagraphBackgroundColor(par, color);

    int reldocnum = par - 1;
    reslist_current = reldocnum;
    previewTextEdit->clear();

    if (!rcldb->getDoc(reslist_winfirst + reldocnum, doc, 0)) {
	QMessageBox::warning(0, "Recoll",
			     QString("Can't retrieve document from database"));
	return;
    }
	
    // Go to the file system to retrieve / convert the document text
    // for preview:

    // Look for appropriate handler
    MimeHandler *handler = 
	getMimeHandler(doc.mimetype, rclconfig->getMimeConf());
    if (!handler) {
	QMessageBox::warning(0, "Recoll",
			     QString("No mime handler for mime type ") +
			     doc.mimetype.c_str());
	return;
    }

    string fn = urltolocalpath(doc.url);
    Rcl::Doc fdoc;
    if (!handler->worker(rclconfig, fn,  doc.mimetype, fdoc)) {
	QMessageBox::warning(0, "Recoll",
			     QString("Failed to convert document for preview!\n") +
			     fn.c_str() + " mimetype " + 
			     doc.mimetype.c_str());
	delete handler;
	return;
    }
    delete handler;

    string rich = plaintorich(fdoc.text);

#if 0
    //Highlighting; pass a list of (search term, style name) to plaintorich
    // and create the corresponding styles with different colors here
    // We need to :
    //  - Break the query into terms : wait for the query analyzer
    //  - Break the text into words. This should use a version of 
    //    textsplit with an option to keep the punctuation (see how to do
    //    this). We do want the same splitter code to be used here and 
    //    when indexing.
    QStyleSheetItem *item = 
	new QStyleSheetItem( previewTextEdit->styleSheet(), "mytag" );
    item->setColor("red");
    item->setFontWeight(QFont::Bold);
#endif

    QString str = QString::fromUtf8(rich.c_str(), rich.length());
    previewTextEdit->setTextFormat(RichText);
    previewTextEdit->setText(str);
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
    reslist_current = -1;
    reslist_winfirst = -1;

    QCString u8 =  queryText->text().utf8();

    if (!rcldb->setQuery(string((const char *)u8)))
	return;
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

static const int respagesize = 10;
void RecollMain::listPrevPB_clicked()
{
    reslist_winfirst -= 2*respagesize;
    listNextPB_clicked();
}

#ifndef MIN
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#endif

// Fill up result list window with next screen of hits
void RecollMain::listNextPB_clicked()
{
    LOGDEB(("listNextPB_clicked: winfirst %d\n", reslist_winfirst));

    if (reslist_winfirst < 0)
	reslist_winfirst = 0;
    else
	reslist_winfirst += respagesize;

    // Insert results if any in result list window 
    bool gotone = false;
    for (int i = 0; i < respagesize; i++) {
	Rcl::Doc doc;
	doc.erase();
	int percent;
	if (i == 0) {
	    reslistTE->clear();
	    previewTextEdit->clear();
	}
	if (!rcldb->getDoc(reslist_winfirst + i, doc, &percent)) {
	    if (i == 0) 
		reslist_winfirst = -1;
	    break;
	}
	int resCnt = rcldb->getResCnt();
	int last = MIN(resCnt, reslist_winfirst+respagesize);
	if (i == 0) {
	    reslistTE->append("<qt><head></head><body><p>");
	    char line[80];
	    sprintf(line, "<p><b>Displaying results %d-%d out of %d</b><br>",
		    reslist_winfirst+1, last, resCnt);
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
	LOGDEB(("Abstract: %s\n", doc.abstract.c_str()));
	string result = "<p>" + 
	    string(perbuf) + " <b>" + doc.title + "</b><br>" +
	    doc.mimetype + "&nbsp;" +
	    (!doc.mtime.empty() ? string(datebuf) + "<br>" : string("")) +
	    (!doc.abstract.empty() ? doc.abstract + "<br>" : string("")) +
	    (!doc.keywords.empty() ? doc.keywords + "<br>" : string("")) +
	    "<i>" + doc.url + +"</i><br>" +
	    "</p>";

	QString str = QString::fromUtf8(result.c_str(), result.length());
	reslistTE->append(str);
    }

    if (gotone) {
	reslistTE->append("</body></qt>");
	reslistTE->setCursorPosition(0,0);
	reslistTE->ensureCursorVisible();
	// Display preview for 1st doc in list
	reslistTE_clicked(1, 0);
    } else {
	// Restore first in win parameter that we shouln't have incremented
	reslist_winfirst -= respagesize;
	if (reslist_winfirst < 0)
	    reslist_winfirst = 0;
    }
}


