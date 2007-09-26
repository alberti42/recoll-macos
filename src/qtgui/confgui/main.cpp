#ifndef lint
static char rcsid[] = "@(#$Id: main.cpp,v 1.1 2007-09-26 12:16:48 dockes Exp $ (C) 2005 J.F.Dockes";
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

#include "autoconfig.h"

#include <string>

using std::string;

#include <unistd.h>

#include <qapplication.h>
#include <qtranslator.h>

#include <qtextcodec.h> 
#include <qthread.h>
#include <qtimer.h>
#include <qlayout.h>

#include "pathut.h"
#include "confgui.h"
#include "debuglog.h"

using namespace confgui;

const string recoll_datadir = RECOLL_DATADIR;

static const char *thisprog;
static int    op_flags;
#define OPT_MOINS 0x1
#define OPT_h     0x4 
#define OPT_c     0x20
static const char usage [] =
"\n"
"trconf\n"
    "   -h : Print help and exit\n"
    ;
//"   -c <configdir> : specify config directory, overriding $RECOLL_CONFDIR\n"
//;
static void
Usage(void)
{
    FILE *fp = (op_flags & OPT_h) ? stdout : stderr;
    fprintf(fp, "%s: Usage: %s", thisprog, usage);
    exit((op_flags & OPT_h)==0);
}

class ConfLinkNull : public ConfLink {
    public:
    virtual bool set(const string& val) 
    {
	fprintf(stderr, "Setting value to [%s]\n", val.c_str());
	return true;
    }
    virtual bool get(string& val) {val = ""; return true;}
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    //    fprintf(stderr, "Application created\n");
    string a_config;
    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
	(*argv)++;
	if (!(**argv))
	    Usage();
	while (**argv)
	    switch (*(*argv)++) {
	    case 'h': op_flags |= OPT_h; Usage();break;
	    }
	//	 b1: 
	argc--; argv++;
    }

    DebugLog::getdbl()->setloglevel(DEBDEB1);
    DebugLog::setfilename("stderr");

    // Translation file for Qt
    QTranslator qt( 0 );
    qt.load( QString( "qt_" ) + QTextCodec::locale(), "." );
    app.installTranslator( &qt );

    // Translations for Recoll
    string translatdir = path_cat(recoll_datadir, "translations");
    QTranslator translator( 0 );
    // QTextCodec::locale() returns $LANG
    translator.load( QString("recoll_") + QTextCodec::locale(), 
		     translatdir.c_str() );
    app.installTranslator( &translator );
    //    fprintf(stderr, "Translations installed\n");



    QWidget w;
    ConfLinkNull lnk;

    QVBoxLayout *vboxLayout = new QVBoxLayout(&w);
    vboxLayout->setSpacing(6);
    vboxLayout->setMargin(11);

    ConfParamIntW *e1 = new ConfParamIntW(&w, lnk, "The text for the label",
					  "The text for the tooltip");
    vboxLayout->addWidget(e1);

    ConfParamStrW *e2 = new ConfParamStrW(&w, lnk, 
					  "The text for the string label",
					  "The text for the string tooltip");
    vboxLayout->addWidget(e2);

    QStringList valuelist;
    valuelist.push_back("aone");
    valuelist.push_back("btwo");
    valuelist.push_back("cthree");
    valuelist.push_back("dfour");

    ConfParamCStrW *e21 = new ConfParamCStrW(&w, lnk, 
					    "The text for the string label",
					    "The text for the string tooltip",
					    valuelist);
    vboxLayout->addWidget(e21);


    ConfParamBoolW *e3 = new ConfParamBoolW(&w, lnk, 
					  "The text for the Bool label",
					  "The text for the Bool tooltip");
    vboxLayout->addWidget(e3);

    ConfParamFNW *e4 = new ConfParamFNW(&w, lnk, 
					"The text for the File Name label",
					"The text for the File Name tooltip");
    vboxLayout->addWidget(e4);

    ConfParamSLW *e5 = new ConfParamSLW(&w, lnk, 
					"The text for the String List label",
					"The text for the String List tooltip");
    vboxLayout->addWidget(e5);

    ConfParamFNLW *e6 = new ConfParamFNLW(&w, lnk, 
					"The text for the File List label",
					"The text for the File List tooltip");
    vboxLayout->addWidget(e6);

    ConfParamCSLW *e7 = new ConfParamCSLW(&w, lnk, 
					  "The text for the File List label",
					  "The text for the File List tooltip",
					  valuelist);
    vboxLayout->addWidget(e7);

    QSize size(0, 0);
    size = size.expandedTo(w.minimumSizeHint());
    w.resize(size);
    //w.setSizeGripEnabled(true);
    w.show();

    // Connect exit handlers etc.. Beware, apparently this must come
    // after mainWindow->show() , else the QMessageBox above never
    // returns.
    app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
    //    fprintf(stderr, "Go\n");
    // Let's go
    return app.exec();
}
