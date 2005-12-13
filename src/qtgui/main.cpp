#ifndef lint
static char rcsid[] = "@(#$Id: main.cpp,v 1.23 2005-12-13 17:20:46 dockes Exp $ (C) 2005 J.F.Dockes";
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
bool showicons;
string iconsdir;
RclDHistory *history;

static string dbdir;

void getQueryStemming(bool &dostem, std::string &stemlang)
{
    string param;
    if (rclconfig->getConfParam("querystemming", param))
	dostem = stringToBool(param);
    else
	dostem = false;
    if (!rclconfig->getConfParam("querystemminglanguage", stemlang))
	stemlang = "english";
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

RecollMain *mainWindow;

void recollCleanup()
{
    QSettings settings;
    if (mainWindow) {
	int width = mainWindow->width();
	int height = mainWindow->height();
	settings.setPath("Recoll.org", "Recoll");
	settings.writeEntry( "/Recoll/geometry/width", width);
	settings.writeEntry("/Recoll/geometry/height", height);
	settings.writeEntry("/Recoll/prefs/simpleSearchAll", 
			    mainWindow->allTermsCB->isChecked());
    }

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

    // Restore some settings from previous session
    QSettings settings;
    settings.setPath("Recoll.org", "Recoll");
    int width = settings.readNumEntry( "/Recoll/geometry/width", 590);
    int height = settings.readNumEntry( "/Recoll/geometry/height", 810);
    bool ssall = settings.readBoolEntry( "/Recoll/prefs/simpleSearchAll", 0); 
    QSize s(width, height);
    
    // Create main window and set its size to previous session's
    RecollMain w;
    mainWindow = &w;
    w.resize(s);
    w.allTermsCB->setDown(ssall);
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

    showicons = false;
    rclconfig->getConfParam("showicons", &showicons);
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
