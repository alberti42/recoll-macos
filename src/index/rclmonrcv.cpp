#ifndef lint
static char rcsid[] = "@(#$Id: rclmonrcv.cpp,v 1.1 2006-10-16 15:33:08 dockes Exp $ (C) 2006 J.F.Dockes";
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


/** A small virtual interface for monitors. Suitable to let either of
    fam/gamin/ or raw imonitor hide behind */
class RclMonitor {
public:
    RclMonitor(){}
    virtual ~RclMonitor() {}
    virtual bool addWatch(const string& path, const struct stat&) = 0;
    virtual bool getEvent(RclMonEvent& ev) = 0;
    virtual bool ok() = 0;
};
// Monitor factory
static RclMonitor *makeMonitor();

/** Class used to create the directory watches */
class WalkCB : public FsTreeWalkerCB {
public:
    WalkCB(RclConfig *conf, RclMonitor *mon)
	: m_conf(conf), m_mon(mon) 
    {}
    virtual ~WalkCB() 
    {}

    virtual FsTreeWalker::Status 
    processone(const string &fn, const struct stat *st, 
	       FsTreeWalker::CbFlag flg)
    {
	LOGDEB2(("rclMonRcvRun: processone %s m_mon %p m_mon->ok %d\n", 
		 fn.c_str(), m_mon, m_mon?m_mon->ok():0));
	if (flg == FsTreeWalker::FtwDirEnter) {
	    if (!m_mon || !m_mon->ok() || !m_mon->addWatch(fn, *st))
		return FsTreeWalker::FtwError;
	}
	return FsTreeWalker::FtwOk;
    }
private:
    RclConfig  *m_conf;
    RclMonitor *m_mon;
};

/** Main thread routine: create watches, then wait for events an queue them */
void *rclMonRcvRun(void *q)
{
    RclMonEventQueue *queue = (RclMonEventQueue *)q;
    RclMonitor *mon;

    LOGDEB(("rclMonRcvRun: running\n"));

    if ((mon = makeMonitor()) == 0) {
	LOGERR(("rclMonRcvRun: makeMonitor failed\n"));
	rclEQ.setTerminate();
	return 0;
    }

    // Get top directories from config and walk trees to add watches
    FsTreeWalker walker;
    WalkCB walkcb(queue->getConfig(), mon);
    list<string> tdl = queue->getConfig()->getTopdirs();
    if (tdl.empty()) {
	LOGERR(("rclMonRcvRun:: top directory list (topdirs param.) not"
		"found in config or Directory list parse error"));
	rclEQ.setTerminate();
	return 0;
    }
    for (list<string>::iterator it = tdl.begin(); it != tdl.end(); it++) {
	queue->getConfig()->setKeyDir(*it);
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
    LOGDEB2(("rclMonRcvRun: waiting for events. rclEQ.ok() %d\n", rclEQ.ok()));
    while (rclEQ.ok()) {
	if (!mon->ok())
	    break;
	RclMonEvent ev;
	if (mon->getEvent(ev)) {
	    rclEQ.pushEvent(ev);
	}
	if (!mon->ok())
	    break;
    }
    LOGDEB(("rclMonRcvRun: exiting\n"));
    rclEQ.setTerminate();
    return 0;
}

//////////////////////////////////////////////////////////////////////////
/** Fam/gamin -based monitor class */
#include <fam.h>
#include <sys/select.h>

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
 
    if (code < FAMChanged || code > FAMEndExist)
    {
        sprintf(unknown_event, "unknown (%d)", code);
        return unknown_event;
    }
    return famevent[code];
}

// FAM based monitor class
class RclFAM : public RclMonitor {
public:
    RclFAM();
    virtual ~RclFAM();
    virtual bool addWatch(const string& path, const struct stat& st);
    virtual bool getEvent(RclMonEvent& ev);
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

bool RclFAM::getEvent(RclMonEvent& ev)
{
    if (!ok())
	return false;
    LOGDEB2(("RclFAM::getEvent:\n"));

    fd_set readfds;
    int fam_fd = FAMCONNECTION_GETFD(&m_conn);
    FD_ZERO(&readfds);
    FD_SET(fam_fd, &readfds);

    // Note: can't see a reason to set a timeout. Only reason we might
    // want out is signal which will break the select call anyway (I
    // don't think that there is any system still using the old bsd-type
    // syscall re-entrance after signal).
    LOGDEB(("RclFAM::getEvent: select\n"));
    if (select(fam_fd + 1, &readfds, 0, 0, 0) < 0) {
	LOGERR(("RclFAM::getEvent: select failed, errno %d\n", errno));
	close();
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
    if ((it = m_reqtodir.find(fe.fr.reqnum)) != m_reqtodir.end()) {
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

// The monitor factory
static RclMonitor *makeMonitor()
{
    return new RclFAM;
}
