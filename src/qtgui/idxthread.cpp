#include <stdio.h>
#include <unistd.h>
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


#include <qthread.h>
#include <qmutex.h>

#include "indexer.h"
#include "debuglog.h"
#include "idxthread.h"
#include "smallut.h"
#include "rclinit.h"

static QMutex curfile_mutex;

class IdxThread : public QThread , public DbIxStatusUpdater {
    virtual void run();
 public:
    virtual bool update() {
	QMutexLocker locker(&curfile_mutex);
	m_statusSnap = status;
	LOGDEB1(("IdxThread::update: indexing %s\n", m_statusSnap.fn.c_str()));
	if (stopindexing) {
	    stopindexing = 0;
	    return false;
	}
	return true;
    }
    ConfIndexer *indexer;
    // Maintain a copy/snapshot of idx status
    DbIxStatus m_statusSnap;
    int loglevel;
};

int stopindexing;
int startindexing;
int indexingdone = 1;
IdxThreadStatus indexingstatus = IDXTS_NULL;
string indexingReason;
static int stopidxthread;

void IdxThread::run()
{
    DebugLog::getdbl()->setloglevel(loglevel);
    recoll_threadinit();
    for (;;) {
	if (stopidxthread) {
	    deleteZ(indexer);
	    return;
	}
	if (startindexing) {
	    startindexing = 0;
	    indexingdone = 0;
	    indexingstatus = IDXTS_NULL;
	    if (indexer->index()) {
		indexingstatus = IDXTS_OK;
		indexingReason = "";
	    } else {
		indexingstatus = IDXTS_ERROR;
		indexingReason = "Indexation failed: " + indexer->getReason();
	    }
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
    idxthread.indexer = new ConfIndexer(myconf, &idxthread);
    idxthread.loglevel = DebugLog::getdbl()->getlevel();
    idxthread.start();
}

void stop_idxthread()
{
    stopindexing = 1;
    stopidxthread = 1;
    idxthread.wait();
}

DbIxStatus idxthread_idxStatus()
{
    QMutexLocker locker(&curfile_mutex);
    return(idxthread.m_statusSnap);
}
