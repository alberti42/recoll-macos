#include <stdio.h>
#include <qthread.h>

#include "indexer.h"
#include "debuglog.h"

class IdxThread : public QThread {
    virtual void run();
 public:
    ConfIndexer *indexer;
};

int startindexing;
int indexingdone;
bool indexingstatus;
int stopidxthread;

void IdxThread::run()
{
    DebugLog::getdbl()->setloglevel(DEBDEB1);
    for (;;) {
	if (stopidxthread) {
	    delete indexer;
	    return;
	}
	if (startindexing) {
	    indexingdone = indexingstatus = startindexing = 0;
	    fprintf(stderr, "Index thread :start index\n");
	    indexingstatus = indexer->index();
	    indexingdone = 1;
	} 
	msleep(100);
    }
}

static IdxThread idxthread;

void start_idxthread(RclConfig *cnf)
{
    ConfIndexer *ix = new ConfIndexer(cnf);
    idxthread.indexer = ix;
    idxthread.start();
}

void stop_idxthread()
{
    stopidxthread = 1;
    while (idxthread.running())
	sleep(1);
}
