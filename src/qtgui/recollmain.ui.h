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


#include "rcldb.h"
#include "rclconfig.h"
#include "debuglog.h"
#include "mimehandler.h"
#include "pathut.h"
#include "recoll.h"
#include "internfile.h"
#include "textsplit.h"
#include "smallut.h"

#ifndef MIN
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#endif

void RecollMain::fileExit()
{
    LOGDEB1(("RecollMain: fileExit\n"));
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

// Text splitter callback used to take note of the query terms byte offsets 
// inside the text. This is then used to post highlight tags. 
class myTextSplitCB : public TextSplitCB {
 public:
    list<pair<int, int> > tboffs;
    const list<string> *terms;
    myTextSplitCB(const list<string>& terms) : terms(&terms) {}
    virtual bool takeword(const std::string& term, int, int bts,  int bte) {
	for (list<string>::const_iterator it = terms->begin(); 
	     it != terms->end(); it++) {
	    string dumb = term;
	    Rcl::dumb_string(term, dumb);
	    if (!stringlowercmp(*it, dumb)) {
		tboffs.push_back(pair<int, int>(bts, bte));
		break;
	    }
	}
	return true;
    }
};

static string plaintorich(const string &in, const list<string>& terms,
			  list<pair<int, int> >&termoffsets)
{
    {string t;
	for (list<string>::const_iterator it = terms.begin();
	     it != terms.end();it++) t += "'" + *it + "' ";
	LOGDEB(("plaintorich: terms: %s\n", t.c_str()));
    }

    myTextSplitCB cb(terms);
    TextSplit splitter(&cb, true);
    splitter.text_to_words(in);
    string out1;
    if (cb.tboffs.empty()) {
	out1 = in;
    } else { 
	list<pair<int, int> >::iterator it = cb.tboffs.begin();
	for (unsigned int i = 0; i < in.length() ; i++) {
	    if (it != cb.tboffs.end()) {
		if (i == (unsigned int)it->first) {
		    out1 += "<termtag>";
		} else if (i == (unsigned int)it->second) {
		    if (it != cb.tboffs.end())
			it++;
		    out1 += "</termtag>";
		}
	    }
	    out1 += in[i];
	}
    }
    string out = "<qt><head><title></title></head><body><p>";
    for (string::const_iterator it = out1.begin();it != out1.end(); it++) {
	if (*it == '\n') {
	    out += "<br>";
	    //	    out += '\n';
	} else {
	    out += *it;
	}
    }
    termoffsets = cb.tboffs;
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
    string fn = urltolocalpath(doc.url);
    Rcl::Doc fdoc;
    if (!internfile(fn, rclconfig, fdoc)) {
	QMessageBox::warning(0, "Recoll",
			     QString("Can't turn doc into internal rep ") +
			     doc.mimetype.c_str());
	return;
    }
    list<string> terms;
    rcldb->getQueryTerms(terms);
    list<pair<int, int> > termoffsets;
    string rich = plaintorich(fdoc.text, terms, termoffsets);

    QStyleSheetItem *item = 
	new QStyleSheetItem( previewTextEdit->styleSheet(), "termtag" );
    item->setColor("blue");
    item->setFontWeight(QFont::Bold);

    QString str = QString::fromUtf8(rich.c_str(), rich.length());
    previewTextEdit->setText(str);
    int para = 0, index = 1;
    if (!termoffsets.empty()) {
	index = (termoffsets.begin())->first;
	LOGDEB1(("Setting cursor position to para %d, index %d\n",para,index));
	previewTextEdit->setCursorPosition(0, index);
    }
    previewTextEdit->ensureCursorVisible();
    previewTextEdit->getCursorPosition(&para, &index);
    LOGDEB1(("PREVIEW Paragraphs: %d. Cpos: %d %d\n", 
	    previewTextEdit->paragraphs(), para, index));
}


// User asked to start query. Run it and call listNextPB_clicked to display
// first page of results
void RecollMain::queryText_returnPressed()
{
    LOGDEB1(("RecollMain::queryText_returnPressed()\n"));
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
    list<string> terms;
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
    previewTextEdit->clear();
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
	LOGDEB1(("Abstract: %s\n", doc.abstract.c_str()));
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
	    reslist_winfirst = -1;
    }
}




void RecollMain::helpQuick_startAction_activated()
{

}
