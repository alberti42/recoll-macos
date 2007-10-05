#ifndef lint
static char rcsid[] = "@(#$Id: main.cpp,v 1.62 2007-10-05 08:03:01 dockes Exp $ (C) 2005 J.F.Dockes";
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

#include <unistd.h>

//#define WITH_KDE
#ifdef WITH_KDE
#include <kapplication.h>
#include <kmainwindow.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#else
#include <qapplication.h>
#include <qtranslator.h>
#endif

#include <qtextcodec.h> 
#include <qthread.h>
#include <qtimer.h>
#include <qmessagebox.h>
#include <qcheckbox.h>
#include <qcombobox.h>

#include "rcldb.h"
#include "rclconfig.h"
#include "pathut.h"
#include "recoll.h"
#include "smallut.h"
#include "rclinit.h"
#include "debuglog.h"
#ifdef WITH_KDE
#include "rclversion.h"
#endif
#include "rclmain_w.h"
#include "ssearch_w.h"
#include "guiutils.h"
#ifdef RCL_USE_ASPELL
#include "rclaspell.h"
#endif
#include "smallut.h"

#ifdef WITH_KDE
static const char description[] =
    I18N_NOOP("A KDE fulltext search application");

static KCmdLineOptions options[] =
{
//    { "+[URL]", I18N_NOOP( "Document to open" ), 0 },
    KCmdLineLastOption
};
#endif

const string recoll_datadir = RECOLL_DATADIR;

RclConfig *rclconfig;
Rcl::Db *rcldb;
#ifdef RCL_USE_ASPELL
Aspell *aspell;
#endif

RclConfig* RclConfig::getMainConfig()
{
    return rclconfig;
}

RclHistory *g_dynconf;
int recollNeedsExit;
static string dbdir;
static RclMain *mainWindow;
static string recollsharedir;

bool maybeOpenDb(string &reason, bool force)
{
    if (!rcldb) {
	reason = "Internal error: db not created";
	return false;
    }

    int qopts = Rcl::Db::QO_NONE;
    if (prefs.queryStemLang.length() > 0)
	qopts |= Rcl::Db::QO_STEM;
    if (force)
	rcldb->close();
    rcldb->rmQueryDb("");
    for (list<string>::const_iterator it = prefs.activeExtraDbs.begin();
	 it != prefs.activeExtraDbs.end(); it++) {
	LOGDEB(("main: adding [%s]\n", it->c_str()));
	rcldb->addQueryDb(*it);
    }
    if (!rcldb->isopen() && !rcldb->open(dbdir, rclconfig->getStopfile(),
					 Rcl::Db::DbRO, qopts)) {
	reason = "Could not open database in " + 
	    dbdir + " wait for indexing to complete?";
	return false;
    }
    rcldb->setAbstractParams(-1, prefs.syntAbsLen, prefs.syntAbsCtx);
    return true;
}

static void recollCleanup()
{
    LOGDEB(("recollCleanup: writing settings\n"));
    rwSettings(true);
    LOGDEB2(("recollCleanup: stopping idx thread\n"));
    stop_idxthread();
    LOGDEB2(("recollCleanup: closing database\n"));
    deleteZ(rcldb);
    deleteZ(rclconfig);
#ifdef RCL_USE_ASPELL
    deleteZ(aspell);
#endif
    LOGDEB2(("recollCleanup: done\n"));
}

static void sigcleanup(int)
{
    fprintf(stderr, "sigcleanup called\n");
    // Cant call exit from here, because the atexit cleanup does some
    // thread stuff that we can't do from signal context.
    // Just set a flag and let the watchdog timer do the work
    recollNeedsExit = 1;
}

extern void qInitImages_recoll();

static const char *thisprog;
static int    op_flags;
#define OPT_MOINS 0x1
#define OPT_h     0x4 
#define OPT_c     0x20
#define OPT_q     0x40
#define OPT_O     0x80
#define OPT_L     0x100
#define OPT_F     0x200

static const char usage [] =
"\n"
"recoll [-h] [-c <configdir>]\n"
"  -h : Print help and exit\n"
"  -c <configdir> : specify config directory, overriding $RECOLL_CONFDIR\n"
"  -q 'query' [-o|l|f]: search query to be executed on startup as if entered\n"
"      into simple search. The default mode is AND (but see modifier options)\n"
"      In most cases, the query string should be quoted with single-quotes to\n"
"      avoid shell interpretation\n"
"     -L : the query will be interpreted as a query language string\n"
"     -O : the query will be interpreted as an OR query\n"
"     -F : the query will be interpreted as a filename search\n"
;
static void
Usage(void)
{
    FILE *fp = (op_flags & OPT_h) ? stdout : stderr;
    fprintf(fp, "%s: Usage: %s", thisprog, usage);
    exit((op_flags & OPT_h)==0);
}

int main(int argc, char **argv)
{
#ifdef WITH_KDE
    KAboutData about("recoll", I18N_NOOP("Recoll"), rclversion, description,
                     KAboutData::License_GPL, "(C) 2006 Jean-Francois Dockes", 0, 0, "jean-francois.dockes@wanadoo.fr");
    about.addAuthor( "Jean-Francois Dockes", 0, 
		     "jean-francois.dockes@wanadoo.fr" );
    KCmdLineArgs::init(argc, argv, &about);
    KCmdLineArgs::addCmdLineOptions( options );
    KApplication app;
#else
    QApplication app(argc, argv);
#endif

    //    fprintf(stderr, "Application created\n");
    string a_config;
    string qstring;

    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
	(*argv)++;
	if (!(**argv))
	    Usage();
	while (**argv)
	    switch (*(*argv)++) {
	    case 'c':	op_flags |= OPT_c; if (argc < 2)  Usage();
		a_config = *(++argv);
		argc--; goto b1;
	    case 'F': op_flags |= OPT_F; break;
	    case 'h': op_flags |= OPT_h; Usage();break;
	    case 'L': op_flags |= OPT_L; break;
	    case 'O': op_flags |= OPT_O; break;
	    case 'q':	op_flags |= OPT_q; if (argc < 2)  Usage();
		qstring = *(++argv);
		argc--; goto b1;
	    }
    b1: argc--; argv++;
    }

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

    string reason;
    rclconfig = recollinit(recollCleanup, sigcleanup, reason, &a_config);
    if (!rclconfig || !rclconfig->ok()) {
	QString msg = app.translate("Main", "Configuration problem: ");
	msg += QString::fromUtf8(reason.c_str());
	QMessageBox::critical(0, "Recoll",  msg);
	exit(1);
    }
    //    fprintf(stderr, "recollinit done\n");

#ifdef RCL_USE_ASPELL
    aspell = new Aspell(rclconfig);
    aspell->init(reason);
    if (!aspell || !aspell->ok()) {
	LOGDEB(("Aspell speller creation failed %s\n", reason.c_str()));
	aspell = 0;
    }
#endif

    string historyfile = path_cat(rclconfig->getConfDir(), "history");
    g_dynconf = new RclHistory(historyfile);
    if (!g_dynconf || !g_dynconf->ok()) {
	QString msg = app.translate("Main", "Configuration problem (dynconf");
	QMessageBox::critical(0, "Recoll",  msg);
	exit(1);
    }

    //    fprintf(stderr, "History done\n");
    rwSettings(false);
    //    fprintf(stderr, "Settings done\n");


    // Create main window and set its size to previous session's
#ifdef WITH_KDE
#if 0
    if (app.isRestored()) {
	RESTORE(RclMain);
    } else {
#endif
        // no session.. just start up normally
        KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
        /// @todo do something with the command line args here
	qInitImages_recoll();
        mainWindow = new RclMain;
        app.setMainWidget( mainWindow );
        mainWindow->show();

        args->clear();

	//    }
#else
    RclMain w;
    mainWindow = &w;
#endif

    if (prefs.mainwidth > 100) {
	QSize s(prefs.mainwidth, prefs.mainheight);
	mainWindow->resize(s);
    }

    mainWindow->sSearch->searchTypCMB->setCurrentItem(prefs.ssearchTyp);
    dbdir = rclconfig->getDbDir();
    if (dbdir.empty()) {
	// Note: this will have to be replaced by a call to a
	// configuration buildin dialog for initial configuration
	QMessageBox::critical(0, "Recoll",
			      app.translate("Main", 
					  "No db directory in configuration"));
	exit(1);
    }

    rcldb = new Rcl::Db;

    if (!maybeOpenDb(reason)) {
	startindexing = 1;

	switch (QMessageBox::
#if (QT_VERSION >= 0x030200)
		question
#else
		information
#endif
		(0, "Recoll",
		 app.translate("Main", "Could not open database in ") +
		 QString::fromLocal8Bit(dbdir.c_str()) +
		 app.translate("Main", 
			       ".\n"
			       "Click Cancel if you want to edit the configuration file before indexation starts, or Ok to let it proceed."),
		 "Ok", "Cancel", 0,   0)) {

	case 0: // Ok
	    break;

	case 1: // Cancel
	    exit(0);
	}
    }

    mainWindow->show();

    if (prefs.startWithAdvSearchOpen)
	mainWindow->showAdvSearchDialog();
    if (prefs.startWithSortToolOpen)
	mainWindow->showSortDialog();

    // Connect exit handlers etc.. Beware, apparently this must come
    // after mainWindow->show() , else the QMessageBox above never
    // returns.
    app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
    QTimer *timer = new QTimer(&app);
    mainWindow->connect(timer, SIGNAL(timeout()), 
			mainWindow, SLOT(periodic100()));
    timer->start(100);
    app.connect(&app, SIGNAL(aboutToQuit()), mainWindow, SLOT(close()));

    start_idxthread(*rclconfig);
    if (op_flags & OPT_q) {
	SSearch::SSearchType stype;
	if (op_flags & OPT_O) {
	    stype = SSearch::SST_ANY;
	} else if (op_flags & OPT_F) {
	    stype = SSearch::SST_FNM;
	} else if (op_flags & OPT_L) {
	    stype = SSearch::SST_LANG;
	} else {
	    stype = SSearch::SST_ALL;
	}
	mainWindow->sSearch->searchTypCMB->setCurrentItem(int(stype));
	mainWindow->
	    sSearch->setSearchString(QString::fromUtf8(qstring.c_str()));
	QTimer::singleShot(0, mainWindow->sSearch, SLOT(startSimpleSearch()));
    }
    //    fprintf(stderr, "Go\n");
    // Let's go
    return app.exec();
}

