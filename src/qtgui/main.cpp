#ifndef lint
static char rcsid[] = "@(#$Id: main.cpp,v 1.11 2005-10-22 05:35:16 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include <unistd.h>

#include <qapplication.h>

#include <qtranslator.h>
#include <qtextcodec.h> 

#include <qthread.h>
#include <qtimer.h>

#include <qmessagebox.h>

#include "rcldb.h"
using Rcl::AdvSearchData;

#include "rclconfig.h"
#include "pathut.h"
#include "recoll.h"
#include "smallut.h"
#include "wipedir.h"
#include "rclinit.h"

#include "recollmain.h"

RclConfig *rclconfig;
Rcl::Db *rcldb;
int recollNeedsExit;
string tmpdir;

void getQueryStemming(bool &dostem, std::string &stemlang)
{
    string param;
    if (rclconfig->getConfParam("querystemming", param))
	dostem = ConfTree::stringToBool(param);
    else
	dostem = false;
    if (!rclconfig->getConfParam("querystemminglanguage", stemlang))
	stemlang = "english";
}

bool maybeOpenDb(string &reason)
{
    if (!rcldb)
	return false;
    if (!rcldb->isopen()) {
	string dbdir;
	if (rclconfig->getConfParam(string("dbdir"), dbdir) == 0) {
	    reason = "No db directory in configuration";
	    return false;
	}
	dbdir = path_tildexpand(dbdir);
	if (!rcldb->open(dbdir, Rcl::Db::DbRO)) {
	    reason = "Could not open database in " + 
		dbdir + " wait for indexing to complete?";
	    return false;
	}
    }
    return true;
}

void recollCleanup()
{
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

    QTranslator translator( 0 );
    // QTextCodec::locale() return $LANG
    translator.load( QString("recoll_") + QTextCodec::locale(), "." );
    a.installTranslator( &translator );
    
    RecollMain w;
    w.show();
    a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
    QTimer *timer = new QTimer(&a);
    w.connect(timer, SIGNAL(timeout()), &w, SLOT(checkExit()));
    timer->start(100);

    rclconfig = recollinit(recollCleanup, sigcleanup);

    if (!rclconfig || !rclconfig->ok()) {
	QMessageBox::critical(0, "Recoll",
			      QString("Could not find configuration"));
	exit(1);
    }

    string dbdir;
    if (rclconfig->getConfParam(string("dbdir"), dbdir) == 0) {
	// Note: this will have to be replaced by a call to a
	// configuration buildin dialog for initial configuration
	QMessageBox::critical(0, "Recoll",
			      QString("No db directory in configuration"));
	exit(1);
    }

    if (!maketmpdir(tmpdir)) {
	QMessageBox::critical(0, "Recoll",
			      QString("Cannot create temporary directory"));
	exit(1);
    }
	
    dbdir = path_tildexpand(dbdir);

    rcldb = new Rcl::Db;

    if (!rcldb || !rcldb->open(dbdir, Rcl::Db::DbRO)) {
	startindexing = 1;
	QMessageBox::information(0, "Recoll",
				 QString("Could not open database in ") + 
				 QString(dbdir) + ". Starting indexation");
	startindexing = 1;
    }

    start_idxthread(rclconfig);

    return a.exec();
}
