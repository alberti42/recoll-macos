#ifndef lint
static char rcsid[] = "@(#$Id: pvmain.cpp,v 1.1 2005-09-27 06:20:16 dockes Exp $ (C) 2005 J.F.Dockes";
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

#include "preview.h"
#include "../plaintorich.h"
#include "readfile.h"

const char *filename = "/home/dockes/tmp/tstpv-utf8.txt";

int main( int argc, char ** argv )
{
    QApplication a(argc, argv);
    Preview w;
    w.show();

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

    return a.exec();
}
