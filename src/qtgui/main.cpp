#include <signal.h>
#include <qapplication.h>
#include <qthread.h>
#include <qtimer.h>

#include <qmessagebox.h>

#include "recollmain.h"
#include "rcldb.h"
#include "rclconfig.h"
#include "pathut.h"

RclConfig *rclconfig;
Rcl::Db *rcldb;

extern void start_idxthread(RclConfig *cnf);
extern void stop_idxthread();
extern int startindexing;

void recollCleanup()
{
    stop_idxthread();
    delete rcldb;
    rcldb = 0;
    delete rclconfig;
    rclconfig = 0;
}

int recollNeedsExit;

static void sigcleanup(int sig)
{
    fprintf(stderr, "sigcleanup\n");
    // Cant call exit from here, because the atexit cleanup does some
    // thread stuff that we can't do from signal context.
    // Just set a flag and let the watchdog timer do the work
    recollNeedsExit = 1;
}


int main( int argc, char ** argv )
{
    QApplication a(argc, argv);
    RecollMain w;
    w.show();
    a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
    QTimer *timer = new QTimer(&a);
    w.connect(timer, SIGNAL(timeout()), &w, SLOT(checkExit()));
    timer->start(100);

    atexit(recollCleanup);
    if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
	signal(SIGHUP, sigcleanup);
    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	signal(SIGINT, sigcleanup);
    if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
	signal(SIGQUIT, sigcleanup);
    if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
	signal(SIGTERM, sigcleanup);


    rclconfig = new RclConfig;
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
    dbdir = path_tildexpand(dbdir);

    rcldb = new Rcl::Db;

    if (!rcldb->open(dbdir, Rcl::Db::DbRO)) {
	startindexing = 1;
	QMessageBox::information(0, "Recoll",
				 QString("Could not open database in ") + 
				 QString(dbdir) + ". Starting indexation");
	startindexing = 1;
    }

    start_idxthread(rclconfig);

    return a.exec();
}
