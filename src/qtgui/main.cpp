#ifndef lint
static char rcsid[] = "@(#$Id: main.cpp,v 1.44 2006-04-28 07:54:38 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include "rclmain.h"
#include "guiutils.h"

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
int recollNeedsExit;
static string dbdir;
static RclMainBase *mainWindow;
static string recollsharedir;

bool maybeOpenDb(string &reason, bool force)
{
    if (!rcldb) {
	reason = "Internal error: db not created";
	return false;
    }

    int qopts = Rcl::Db::QO_NONE;
    if (prefs.queryBuildAbstract) {
	qopts |= Rcl::Db::QO_BUILD_ABSTRACT;
	if (prefs.queryReplaceAbstract)
	    qopts |= Rcl::Db::QO_REPLACE_ABSTRACT;
    }
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
    if (!rcldb->isopen() && !rcldb->open(dbdir, Rcl::Db::DbRO, qopts)) {
	reason = "Could not open database in " + 
	    dbdir + " wait for indexing to complete?";
	return false;
    }
    return true;
}

static void recollCleanup()
{
    if (mainWindow) {
	prefs.mainwidth = mainWindow->width();
	prefs.mainheight = mainWindow->height();
	prefs.ssearchTyp = mainWindow->sSearch->searchTypCMB->currentItem();
    }
    rwSettings(true);
    stop_idxthread();
    delete rcldb;
    rcldb = 0;
    delete rclconfig;
    rclconfig = 0;
}

static void sigcleanup(int)
{
    // fprintf(stderr, "sigcleanup\n");
    // Cant call exit from here, because the atexit cleanup does some
    // thread stuff that we can't do from signal context.
    // Just set a flag and let the watchdog timer do the work
    recollNeedsExit = 1;
}

extern void qInitImages_recoll();

int main( int argc, char ** argv )
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

    rwSettings(false);

    string reason;
    rclconfig = recollinit(recollCleanup, sigcleanup, reason);
    if (!rclconfig || !rclconfig->ok()) {
	QString msg = app.translate("Main", "Configuration problem: ");
	msg += QString::fromUtf8(reason.c_str());
	QMessageBox::critical(0, "Recoll",  msg);
	exit(1);
    }

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
    QSize s(prefs.mainwidth, prefs.mainheight);
    mainWindow->resize(s);


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

    // Connect exit handlers etc..
    app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
    QTimer *timer = new QTimer(&app);
    mainWindow->connect(timer, SIGNAL(timeout()), 
			mainWindow, SLOT(periodic100()));
    timer->start(100);

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
		 "Ok",
		 "Cancel", 0, 0, 1 )) {
	case 0: // Ok
	    break;
	case 1: // Cancel
	    exit(0);
	}
    }

    mainWindow->show();
    start_idxthread(*rclconfig);

    // Let's go
    return app.exec();
}

