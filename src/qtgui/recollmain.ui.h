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


void RecollMain::helpIndex()
{

}


void RecollMain::helpContents()
{

}


void RecollMain::helpAbout()
{

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

void RecollMain::resTextEdit_clicked( int par, int car )
{
    fprintf(stderr, "Clicked at paragraph %d, char %d\n", par, car);
    Rcl::Doc doc;
    doc.erase();
    if (rcldb->getDoc(par, doc)) {
	
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

void RecollMain::queryText_returnPressed()
{
    LOGDEB(("RecollMain::queryText_returnPressed()\n"));
    resTextEdit->clear();
    previewTextEdit->clear();

    string rawq =  queryText->text();
    rcldb->setQuery(rawq);
    Rcl::Doc doc;

    // Insert results if any in result list window 
    QString result;
    resTextEdit->append("<qt><head></head><body>");
    for (int i = 0;; i++) {
	doc.erase();
 if (!rcldb->getDoc(i, doc))
     break;
 LOGDEB(("Url: %s\n", doc.url.c_str()));
 LOGDEB(("Mimetype: \n", doc.mimetype.c_str()));
 LOGDEB(("Mtime: \n", doc.mtime.c_str()));
 LOGDEB(("Origcharset: \n", doc.origcharset.c_str()));
 LOGDEB(("Title: \n", doc.title.c_str()));
 LOGDEB(("Text: \n", doc.text.c_str()));
 LOGDEB(("Keywords: \n", doc.keywords.c_str()));
 LOGDEB(("Abstract: \n", doc.abstract.c_str()));

 result = "<p>" + doc.url + "</p>";
 resTextEdit->append(result);
    }
    resTextEdit->append("</body></qt>");

    // Display preview for 1st doc in list
    resTextEdit_clicked(0, 0);
}


void RecollMain::Search_clicked()
{
    queryText_returnPressed();
}
