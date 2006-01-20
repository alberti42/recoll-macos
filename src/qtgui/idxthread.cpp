#include <stdio.h>
#include <unistd.h>


#include <qthread.h>

#include "indexer.h"
#include "debuglog.h"
#include "idxthread.h"

class IdxThread : public QThread {
    virtual void run();
 public:
    ConfIndexer *indexer;
};

int startindexing;
int indexingdone = 1;
bool indexingstatus = false;

static int stopidxthread;

void IdxThread::run()
{
    DebugLog::getdbl()->setloglevel(DEBDEB1);
    for (;;) {
	if (stopidxthread) {
	    delete indexer;
	    return;
	}
	if (startindexing) {
	    indexingdone = indexingstatus = 0;
	    fprintf(stderr, "Index thread :start index\n");
	    indexingstatus = indexer->index();
	    startindexing = 0;
	    indexingdone = 1;
	} 
	msleep(100);
    }
}

static IdxThread idxthread;

void start_idxthread(const RclConfig& cnf)
{
    // We have to make a copy of the config (setKeydir changes it during 
    // indexation)
    RclConfig *myconf = new RclConfig(cnf);
    ConfIndexer *ix = new ConfIndexer(myconf);
    idxthread.indexer = ix;
    idxthread.start();
}

void stop_idxthread()
{
    stopidxthread = 1;
    idxthread.wait();
}
