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
