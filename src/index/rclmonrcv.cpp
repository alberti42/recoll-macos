#include "autoconfig.h"
#ifdef RCL_MONITOR
#ifndef lint
static char rcsid[] = "@(#$Id: rclmonrcv.cpp,v 1.3 2006-10-22 14:47:13 dockes Exp $ (C) 2006 J.F.Dockes";
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
#include <sys/stat.h>
#include <errno.h>

#include "debuglog.h"
#include "rclmon.h"
#include "fstreewalk.h"
#include "indexer.h"
#include "pathut.h"

/**
 * Recoll real time monitor event receiver. This file has code to interface 
 * to FAM and place events on the event queue.
 */



/** A small virtual interface for monitors. Probably suitable to let
    either of fam/gamin or raw imonitor hide behind */
class RclMonitor {
public:
    RclMonitor(){}
    virtual ~RclMonitor() {}
    virtual bool addWatch(const string& path, const struct stat&) = 0;
    virtual bool getEvent(RclMonEvent& ev, int secs = -1) = 0;
    virtual bool ok() = 0;
};

// Monitor factory. We only have one compiled-in kind at a time, no
// need for a 'kind' parameter
static RclMonitor *makeMonitor();

/** This class is a callback for the file system tree walker
    class. The callback method alternatively creates the directory
    watches and flushes the event queue (to avoid a possible overflow
    while we create the watches)*/
class WalkCB : public FsTreeWalkerCB {
public:
    WalkCB(RclConfig *conf, RclMonitor *mon, RclMonEventQueue *queue)
	: m_conf(conf), m_mon(mon), m_queue(queue)
    {}
    virtual ~WalkCB() 
    {}

    virtual FsTreeWalker::Status 
    processone(const string &fn, const struct stat *st, FsTreeWalker::CbFlag flg)
    {
	LOGDEB2(("rclMonRcvRun: processone %s m_mon %p m_mon->ok %d\n", 
		 fn.c_str(), m_mon, m_mon?m_mon->ok():0));
	// Create watch when entering directory
	if (flg == FsTreeWalker::FtwDirEnter) {
	    // Empty whatever events we may already have on queue
	    while (m_queue->ok() && m_mon->ok()) {
		RclMonEvent ev;
		if (m_mon->getEvent(ev, 0)) {
		    m_queue->pushEvent(ev);
		} else {
		    break;
		}
	    }
	    if (!m_mon || !m_mon->ok() || !m_mon->addWatch(fn, *st))
		return FsTreeWalker::FtwError;
	}
	return FsTreeWalker::FtwOk;
    }

private:
    RclConfig         *m_conf;
    RclMonitor        *m_mon;
    RclMonEventQueue  *m_queue;
};

/** Main thread routine: create watches, then forever wait for and queue events */
void *rclMonRcvRun(void *q)
{
    RclMonEventQueue *queue = (RclMonEventQueue *)q;

    LOGDEB(("rclMonRcvRun: running\n"));

    // Create the fam/whatever interface object
    RclMonitor *mon;
    if ((mon = makeMonitor()) == 0) {
	LOGERR(("rclMonRcvRun: makeMonitor failed\n"));
	queue->setTerminate();
	return 0;
    }

    // Get top directories from config 
    list<string> tdl = queue->getConfig()->getTopdirs();
    if (tdl.empty()) {
	LOGERR(("rclMonRcvRun:: top directory list (topdirs param.) not"
		"found in config or Directory list parse error"));
	queue->setTerminate();
	return 0;
    }

    // Walk the directory trees to add watches
    FsTreeWalker walker;
    WalkCB walkcb(queue->getConfig(), mon, queue);
    for (list<string>::iterator it = tdl.begin(); it != tdl.end(); it++) {
	queue->getConfig()->setKeyDir(*it);
	// Adjust the skipped names according to config
	walker.clearSkippedNames();
	string skipped; 
	if (queue->getConfig()->getConfParam("skippedNames", skipped)) {
	    list<string> skpl;
	    stringToStrings(skipped, skpl);
	    walker.setSkippedNames(skpl);
	}
	LOGDEB(("rclMonRcvRun: walking %s\n", it->c_str()));
	walker.walk(*it, walkcb);
    }

    // Forever wait for monitoring events and add them to queue:
    LOGDEB2(("rclMonRcvRun: waiting for events. queue->ok() %d\n", queue->ok()));
    while (queue->ok() && mon->ok()) {
	RclMonEvent ev;
	if (mon->getEvent(ev)) {
	    queue->pushEvent(ev);
	}
    }

    LOGDEB(("rclMonRcvRun: exiting\n"));
    queue->setTerminate();
    return 0;
}

//////////////////////////////////////////////////////////////////////////
/** Fam/gamin -based monitor class */
#include <fam.h>
#include <sys/select.h>

// Translate event code to string (debug)
static const char *event_name(int code)
{
    static const char *famevent[] = {
        "",
        "FAMChanged",
        "FAMDeleted",
        "FAMStartExecuting",
        "FAMStopExecuting",
        "FAMCreated",
        "FAMMoved",
        "FAMAcknowledge",
        "FAMExists",
        "FAMEndExist"
    };
    static char unknown_event[20];
 
    if (code < FAMChanged || code > FAMEndExist) {
        sprintf(unknown_event, "unknown (%d)", code);
        return unknown_event;
    }
    return famevent[code];
}

/** FAM based monitor class. We have to keep a record of FAM watch
    request numbers to directory names as the event only contain the
    request number and file name, not the full path */
class RclFAM : public RclMonitor {
public:
    RclFAM();
    virtual ~RclFAM();
    virtual bool addWatch(const string& path, const struct stat& st);
    virtual bool getEvent(RclMonEvent& ev, int secs = -1);
    bool ok() {return m_ok;}

private:
    bool m_ok;
    FAMConnection m_conn;
    void close() {
	FAMClose(&m_conn);
	m_ok = false;
    }
    map<int,string> m_reqtodir;
};

RclFAM::RclFAM()
    : m_ok(false)
{
    if (FAMOpen2(&m_conn, "Recoll")) {
	LOGERR(("RclFAM::RclFAM: FAMOpen2 failed, errno %d\n", errno));
	return;
    }
    m_ok = true;
}

RclFAM::~RclFAM()
{
    if (ok())
	FAMClose(&m_conn);
}

bool RclFAM::addWatch(const string& path, const struct stat& st)
{
    if (!ok())
	return false;
    LOGDEB(("RclFAM::addWatch: adding %s\n", path.c_str()));
    FAMRequest req;
    if (S_ISDIR(st.st_mode)) {
	if (FAMMonitorDirectory(&m_conn, path.c_str(), &req, 0) != 0) {
	    LOGERR(("RclFAM::addWatch: FAMMonitorDirectory failed\n"));
	    return false;
	}
	m_reqtodir[req.reqnum] = path;
    } else if (S_ISREG(st.st_mode)) {
	if (FAMMonitorFile(&m_conn, path.c_str(), &req, 0) != 0) {
	    LOGERR(("RclFAM::addWatch: FAMMonitorFile failed\n"));
	    return false;
	}
    }
    return true;
}

bool RclFAM::getEvent(RclMonEvent& ev, int secs)
{
    if (!ok())
	return false;
    LOGDEB2(("RclFAM::getEvent:\n"));

    fd_set readfds;
    int fam_fd = FAMCONNECTION_GETFD(&m_conn);
    FD_ZERO(&readfds);
    FD_SET(fam_fd, &readfds);

    LOGDEB(("RclFAM::getEvent: select\n"));
    struct timeval timeout;
    if (secs >= 0) {
	memset(&timeout, 0, sizeof(timeout));
	timeout.tv_sec = secs;
    }
    int ret;
    if ((ret=select(fam_fd + 1, &readfds, 0, 0, secs >= 0 ? &timeout : 0)) < 0) {
	LOGERR(("RclFAM::getEvent: select failed, errno %d\n", errno));
	close();
	return false;
    } else if (ret == 0) {
	// timeout
	return false;
    }

    if (!FD_ISSET(fam_fd, &readfds))
	return false;

    FAMEvent fe;
    if (FAMNextEvent(&m_conn, &fe) < 0) {
	LOGERR(("RclFAM::getEvent: FAMNextEvent failed, errno %d\n", errno));
	close();
	return false;
    }
    
    map<int,string>::const_iterator it;
    if ((fe.filename[0] != '/') && 
	(it = m_reqtodir.find(fe.fr.reqnum)) != m_reqtodir.end()) {
	ev.m_path = path_cat(it->second, fe.filename);
    } else {
	ev.m_path = fe.filename;
    }
    LOGDEB(("RclFAM::getEvent: %-12s %s\n", 
	    event_name(fe.code), ev.m_path.c_str()));

    switch (fe.code) {
    case FAMChanged:
    case FAMCreated:
    case FAMExists:
	// Let the other side sort out the status of this file vs the db
	ev.m_etyp = RclMonEvent::RCLEVT_MODIFY;
	break;

    case FAMDeleted:
	ev.m_etyp = RclMonEvent::RCLEVT_DELETE;
	break;

    case FAMMoved: /* Never generated it seems */
	LOGDEB(("RclFAM::getEvent: got move event !\n"));
	ev.m_etyp = RclMonEvent::RCLEVT_MODIFY;
	break;

    case FAMStartExecuting:
    case FAMStopExecuting:
    case FAMAcknowledge:
    case FAMEndExist:
    default:
	return false;
    }
    return true;
}

// The monitor 'factory'
static RclMonitor *makeMonitor()
{
    return new RclFAM;
}
#endif // RCL_MONITOR
