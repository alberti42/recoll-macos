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

void RecollMain::fileExit()
{
    exit(0);
}


#include <qmessagebox.h>

#include "rcldb.h"
#include "rclconfig.h"
#include "debuglog.h"
#include "mimehandler.h"

extern RclConfig *rclconfig;
extern Rcl::Db *rcldb;

static string plaintorich(const string &in)
{
    string out = "<qt><head><title></title></head><body><p>";
    for (unsigned int i = 0; i < in.length() ; i++) {
	if (in[i] == '\n') {
	    out += "<br>";
	} else {
	    out += in[i];
	}
	if (i == 10) {
	    out += "<mytag>";
	}
	if (i == 20) {
	    out += "</mytag>";
	}	    

    }
    return out;
}

// Click in the result list window: display preview for selected document, 
// and highlight entry. The paragraph number is doc number in window + 1
void RecollMain::resTextEdit_clicked(int par, int car)
{
    LOGDEB(("RecollMain::resTextEdi_clicked: par %d, char %d\n", par, car));
    if (reslist_winfirst == -1)
	return;
    Rcl::Doc doc;
    doc.erase();
    if (reslist_current != -1) {
	QColor color("white");
	resTextEdit->setParagraphBackgroundColor(reslist_current+1, color);
    }
    QColor color("lightblue");
    resTextEdit->setParagraphBackgroundColor(par, color);

    int reldocnum = par-1;
    reslist_current = reldocnum;
    previewTextEdit->clear();

    if (rcldb->getDoc(reslist_winfirst + reldocnum, doc, 0)) {
	
	// Go to the file system to retrieve / convert the document text
	// for preview:

	// Look for appropriate handler
	MimeHandlerFunc fun = 
	    getMimeHandler(doc.mimetype, rclconfig->getMimeConf());
	if (!fun) {
	    QMessageBox::warning(0, "Recoll",
				 QString("No mime handler for mime type ") +
				 doc.mimetype.c_str());
	    return;
	}

	string fn = doc.url.substr(6, string::npos);
	Rcl::Doc fdoc;
	if (!fun(rclconfig, fn,  doc.mimetype, fdoc)) {
	    QMessageBox::warning(0, "Recoll",
			 QString("Failed to convert document for preview!\n") +
				 fn.c_str() + " mimetype " + 
				 doc.mimetype.c_str());
	    return;
	}

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
}

#include "pathut.h"

void RecollMain::queryText_returnPressed()
{
    LOGDEB(("RecollMain::queryText_returnPressed()\n"));
    reslist_current = -1;
    reslist_winfirst = -1;

    string rawq =  queryText->text();
    rcldb->setQuery(rawq);
    listNextPB_clicked();
}


void RecollMain::Search_clicked()
{
    queryText_returnPressed();
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
	if (!rcldb->getDoc(reslist_winfirst + i, doc, &percent))
	    break;
	int resCnt = rcldb->getResCnt();
	int last = MIN(resCnt, reslist_winfirst+respagesize);
	if (i == 0) {
	    resTextEdit->clear();
	    previewTextEdit->clear();
	    resTextEdit->append("<qt><head></head><body><p>");
	    char line[80];
	    sprintf(line, "<p><b>Displaying results %d-%d out of %d</b><br>",
		    reslist_winfirst+1, last, resCnt);
	    resTextEdit->append(line);
	}
	    
	gotone = true;

	LOGDEB1(("Url: %s\n", doc.url.c_str()));
	LOGDEB1(("Mimetype: %s\n", doc.mimetype.c_str()));
	LOGDEB1(("Mtime: %s\n", doc.mtime.c_str()));
	LOGDEB1(("Origcharset: %s\n", doc.origcharset.c_str()));
	LOGDEB1(("Title: %s\n", doc.title.c_str()));
	LOGDEB1(("Text: %s\n", doc.text.c_str()));
	LOGDEB1(("Keywords: %s\n", doc.keywords.c_str()));
	LOGDEB1(("Abstract: %s\n", doc.abstract.c_str()));
	
	// Result list display. Standard Omega includes:
	//  - title or simple file name or url
	//  - abstract and keywords
	//  - url 
	//  - relevancy percentage + keywords matched
	//  - date de modification
	//  - langue
        //  - taille 
	char perbuf[10];
	sprintf(perbuf, "%3d%%", percent);
	if (doc.title.empty()) 
	    doc.title = path_getsimple(doc.url);
	char datebuf[100];
	datebuf[0] = 0;
	if (!doc.mtime.empty()) {
	    time_t mtime = atol(doc.mtime.c_str());
	    struct tm *tm = localtime(&mtime);
	    strftime(datebuf, 99, "<i>Modified:</i> %F %T", tm);
	}
	    
	string result = "<p>" + 
	    string(perbuf) + " <b>" + doc.title + "</b><br>" +
	    (!doc.mtime.empty() ? string(datebuf) + "<br>" : string("")) +
	    (!doc.abstract.empty() ? doc.abstract + "<br>" : string("")) +
	    (!doc.keywords.empty() ? doc.keywords + "<br>" : string("")) +
	    "<i>" + doc.url + +"</i><br>" +
	    "</p>";
	QString str = QString::fromUtf8(result.c_str(), result.length());

	resTextEdit->append(str);
    }

    if (gotone) {
	resTextEdit->append("</body></qt>");
	resTextEdit->setCursorPosition(0,0);
	resTextEdit->ensureCursorVisible();
	// Display preview for 1st doc in list
	resTextEdit_clicked(1, 0);
    } else {
	// Restore first in win parameter that we shouln't have incremented
	reslist_winfirst -= respagesize;
	if (reslist_winfirst < 0)
	    reslist_winfirst = 0;
    }
}
