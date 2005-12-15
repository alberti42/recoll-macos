#ifndef lint
static char rcsid[] = "@(#$Id: main.cpp,v 1.26 2005-12-15 14:39:57 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include <unistd.h>

#include <qapplication.h>

#include <qtranslator.h>
#include <qtextcodec.h> 
#include <qthread.h>
#include <qtimer.h>
#include <qmessagebox.h>
#include <qsettings.h>
#include <qcheckbox.h>


#include "rcldb.h"
#ifndef NO_NAMESPACES
using Rcl::AdvSearchData;
#endif /* NO_NAMESPACES */
#include "rclconfig.h"
#include "pathut.h"
#include "recoll.h"
#include "smallut.h"
#include "wipedir.h"
#include "rclinit.h"
#include "history.h"

#include "recollmain.h"

static const char *recollsharedir = "/usr/local/share/recoll";

RclConfig *rclconfig;
Rcl::Db *rcldb;
int recollNeedsExit;
string tmpdir;
string iconsdir;
RclDHistory *history;
static string dbdir;
static RecollMain *mainWindow;

///////////////////////// 
// Global variables for user preferences. These are set in the user preference
// dialog and saved restored to the appropriate place in the qt settings
bool prefs_showicons;
int prefs_respagesize = 8;
QString prefs_reslistfontfamily;
int prefs_reslistfontsize;
QString prefs_queryStemLang;
int prefs_mainwidth;
int prefs_mainheight;
bool prefs_ssall;

#define SETTING_RW(var, nm, tp, def)			\
    if (writing) {					\
	settings.writeEntry(nm , var);			\
    } else {						\
	var = settings.read##tp##Entry(nm, def);	\
    }						

static void rwSettings(bool writing)
{
    if (writing) {
	if (!mainWindow) 
	    return;
	prefs_mainwidth = mainWindow->width();
	prefs_mainheight = mainWindow->height();
	prefs_ssall = mainWindow->allTermsCB->isChecked();
    }

    QSettings settings;
    settings.setPath("Recoll.org", "Recoll");

    SETTING_RW(prefs_mainwidth, "/Recoll/geometry/width", Num, 590);
    SETTING_RW(prefs_mainheight, "/Recoll/geometry/height", Num, 810);
    SETTING_RW(prefs_ssall, "/Recoll/prefs/simpleSearchAll", Bool, false);
    SETTING_RW(prefs_showicons, "/Recoll/prefs/reslist/showicons", Bool, true);
    SETTING_RW(prefs_respagesize, "/Recoll/prefs/reslist/pagelen", Num, 8);
    SETTING_RW(prefs_reslistfontfamily, "/Recoll/prefs/reslist/fontFamily", ,
	       "");
    SETTING_RW(prefs_reslistfontsize, "/Recoll/prefs/reslist/fontSize", Num, 
	       0);
    SETTING_RW(prefs_queryStemLang, "/Recoll/prefs/query/stemLang", ,
	       "english");
}

bool maybeOpenDb(string &reason)
{
    if (!rcldb) {
	reason = "Internal error: db not created";
	return false;
    }
    if (!rcldb->isopen() && !rcldb->open(dbdir, Rcl::Db::DbRO)) {
	reason = "Could not open database in " + 
	    dbdir + " wait for indexing to complete?";
	return false;
    }
    return true;
}

void recollCleanup()
{
    rwSettings(true);
    stop_idxthread();
    delete rcldb;
    rcldb = 0;
    delete rclconfig;
    rclconfig = 0;
    if (tmpdir.length()) {
	wipedir(tmpdir);
	rmdir(tmpdir.c_str());
	tmpdir.erase();
    }
}

static void sigcleanup(int)
{
    // fprintf(stderr, "sigcleanup\n");
    // Cant call exit from here, because the atexit cleanup does some
    // thread stuff that we can't do from signal context.
    // Just set a flag and let the watchdog timer do the work
    recollNeedsExit = 1;
}


int main( int argc, char ** argv )
{
    QApplication a(argc, argv);

    // translation file for Qt
    QTranslator qt( 0 );
    qt.load( QString( "qt_" ) + QTextCodec::locale(), "." );
    a.installTranslator( &qt );

    rwSettings(false);

    // Create main window and set its size to previous session's
    RecollMain w;
    mainWindow = &w;

    QSize s(prefs_mainwidth, prefs_mainheight);
    w.resize(s);
    w.allTermsCB->setChecked(prefs_ssall);
    string reason;
    rclconfig = recollinit(recollCleanup, sigcleanup, reason);
    if (!rclconfig || !rclconfig->ok()) {
	QString msg = a.translate("Main", "Configuration problem: ");
	msg += reason;
	QMessageBox::critical(0, "Recoll",  msg);
	exit(1);
    }

    if (rclconfig->getConfParam(string("dbdir"), dbdir) == 0) {
	// Note: this will have to be replaced by a call to a
	// configuration buildin dialog for initial configuration
	QMessageBox::critical(0, "Recoll",
			      a.translate("Main", 
					  "No db directory in configuration"));
	exit(1);
    }
    dbdir = path_tildexpand(dbdir);

    // Translations for Recoll
    string translatdir = path_cat(recollsharedir, "translations");
    QTranslator translator( 0 );
    // QTextCodec::locale() returns $LANG
    translator.load( QString("recoll_") + QTextCodec::locale(), 
		     translatdir.c_str() );
    a.installTranslator( &translator );

    rclconfig->getConfParam("iconsdir", iconsdir);
    if (iconsdir.empty()) {
	iconsdir = path_cat(recollsharedir, "images");
    } else {
	iconsdir = path_tildexpand(iconsdir);
    }

    if (!maketmpdir(tmpdir)) {
	QMessageBox::critical(0, "Recoll",
			      a.translate("Main", 
					 "Cannot create temporary directory"));
	exit(1);
    }

    string historyfile = path_cat(rclconfig->getConfDir(), "history");
    history = new RclDHistory(historyfile);


    rcldb = new Rcl::Db;

    // Connect exit handlers etc..
    a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
    QTimer *timer = new QTimer(&a);
    w.connect(timer, SIGNAL(timeout()), &w, SLOT(periodic100()));
    timer->start(100);

    if (!rcldb || !rcldb->open(dbdir, Rcl::Db::DbRO)) {
	startindexing = 1;
	switch (QMessageBox::
		question(0, "Recoll",
			 a.translate("Main", "Could not open database in ")+
			 QString(dbdir) +
			 a.translate("Main", 
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

    start_idxthread(rclconfig);

    // Let's go
    w.show();
    return a.exec();
}
