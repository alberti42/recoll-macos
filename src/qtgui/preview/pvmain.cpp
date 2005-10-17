#ifndef lint
static char rcsid[] = "@(#$Id: pvmain.cpp,v 1.3 2005-10-17 13:36:53 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include <stdio.h>
#include <unistd.h>

#include <string>
#include <list>
using std::string;
using std::list;
using std::pair;

#include <qapplication.h>
#include <qobject.h>
#include <qtextedit.h>
#include <qtabwidget.h>
#include <qlayout.h>

#include "preview.h"
#include "../plaintorich.h"
#include "readfile.h"

const char *filename = "/home/dockes/tmp/tstpv-utf8.txt";
int recollNeedsExit;
int main( int argc, char ** argv )
{
    QApplication a(argc, argv);
    Preview w;

    string text;
    if (!file_to_string(filename, text)) {
	fprintf(stderr, "Could not read %s\n", filename);
	exit(1);
    }
    list<string> terms;
    list<pair<int, int> > termoffs;
    string rich = plaintorich(text, terms, termoffs);
    QString str = QString::fromUtf8(rich.c_str(), rich.length());
    w.pvEdit->setText(str);

    //    QVBoxLayout *unnamedLayout = 
    //	new QVBoxLayout(0, 11, 6, "unnamedLayout"); 
    QWidget *anon = new QWidget(w.pvTab);
    QVBoxLayout *anonLayout = new QVBoxLayout(anon, 11, 6, "unnamedLayout"); 
    QTextEdit *newEd = new QTextEdit(anon, "pvEdit");
    anonLayout->addWidget(newEd);
    fprintf(stderr, "pvEdit %p newEd: %p\n", w.pvEdit, newEd);
    newEd->setReadOnly( TRUE );
    newEd->setUndoRedoEnabled( FALSE );
    newEd->setText(str);

    w.pvTab->addTab(anon, "Tab 2");
    w.show();
    return a.exec();
}
