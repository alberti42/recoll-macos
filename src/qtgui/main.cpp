#include <signal.h>
#include <qapplication.h>
#include <qmessagebox.h>

#include "recollmain.h"
#include "rcldb.h"
#include "rclconfig.h"

RclConfig *rclconfig;
Rcl::Db *rcldb;

static void cleanup()
{
    delete rcldb;
    rcldb = 0;
    delete rclconfig;
    rclconfig = 0;
}
static void sigcleanup(int sig)
{
    fprintf(stderr, "sigcleanup\n");
    cleanup();
    exit(1);
}
int main( int argc, char ** argv )
{
    QApplication a( argc, argv );
    RecollMain w;
    w.show();
    a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );

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
    
    rcldb = new Rcl::Db;

    if (!rcldb->open(dbdir, Rcl::Db::DbRO)) {
	QMessageBox::critical(0, "Recoll",
			      QString("Could not open database in ") + 
			      QString(dbdir));
	exit(1);
    }
    atexit(cleanup);
    if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
	signal(SIGHUP, sigcleanup);
    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	signal(SIGINT, sigcleanup);
    if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
	signal(SIGQUIT, sigcleanup);
    if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
	signal(SIGTERM, sigcleanup);



    return a.exec();
}
