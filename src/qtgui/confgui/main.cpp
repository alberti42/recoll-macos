#ifndef lint
static char rcsid[] = "@(#$Id: main.cpp,v 1.6 2008-11-24 15:23:12 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include <iostream>

using namespace std;

#include <unistd.h>

#include <qglobal.h>
#include <qobject.h>

#if QT_VERSION < 0x040000
#define QFRAME_INCLUDE <qframe.h>
#define QFILEDIALOG_INCLUDE <qfiledialog.h>
#define QLISTBOX_INCLUDE <qlistbox.h>
#include <qtabdialog.h>
#define QFILEDIALOG QFileDialog 
#define QFRAME QFrame
#define QHBOXLAYOUT QHBoxLayout
#define QLISTBOX QListBox
#define QLISTBOXITEM QListBoxItem
#define QVBOXLAYOUT QVBoxLayout
#define QTABDIALOG QTabDialog

#else // Qt4 -> 

#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <Q3TabDialog>
#include <QFrame>
#define QFRAME_INCLUDE <q3frame.h>

#include <QFileDialog>
#define QFILEDIALOG_INCLUDE <q3filedialog.h>
#define QLISTBOX_INCLUDE <q3listbox.h>

#define QFILEDIALOG Q3FileDialog 
#define QFRAME Q3Frame
#define QHBOXLAYOUT Q3HBoxLayout
#define QLISTBOX Q3ListBox
#define QLISTBOXITEM Q3ListBoxItem
#define QVBOXLAYOUT Q3VBoxLayout
#define QTABDIALOG Q3TabDialog
#endif // QT 3/4

#include <qobject.h>
#include <qapplication.h>
#include <qtranslator.h>
#include <qtextcodec.h> 
#include <qthread.h>
#include <qtimer.h>
#include <qlayout.h>
#include QFRAME_INCLUDE
#include <qwidget.h>
#include <qlabel.h>

#include "pathut.h"
#include "confguiindex.h"
#include "debuglog.h"
#include "rclconfig.h"
#include "execmd.h"
#include "conflinkrcl.h"

using namespace confgui;

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


static RclConfig *config;
RclConfig *RclConfig::getMainConfig() 
{
    return config;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    //    fprintf(stderr, "Application created\n");
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

    string a_config = "tstconfdir";
    config = new RclConfig(&a_config);
    if (config == 0 || !config->ok()) {
	cerr << "Cant read configuration in: " << a_config << endl;
	exit(1);
    }
    cerr << "Working with configuration file in: " << config->getConfDir() 
	 << endl;

    // Translation file for Qt
    QTranslator qt( 0 );
    qt.load( QString( "qt_" ) + QTextCodec::locale(), "." );
    app.installTranslator( &qt );

    // Translations for Recoll
    string translatdir = path_cat(config->getDatadir(), "translations");
    QTranslator translator( 0 );
    // QTextCodec::locale() returns $LANG
    translator.load( QString("recoll_") + QTextCodec::locale(), 
		     translatdir.c_str() );
    app.installTranslator( &translator );
    //    fprintf(stderr, "Translations installed\n");

    ConfIndexW w(0, config);
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
