#include "autoconfig.h"

#ifdef RCL_MONITOR
#ifndef lint
static char rcsid[] = "@(#$Id: rclmonprc.cpp,v 1.3 2006-10-22 14:47:13 dockes Exp $ (C) 2006 J.F.Dockes";
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

/**
 * Recoll real time monitor processing. This file has the code to retrieve
 * event from the event queue and do the database-side processing, and the 
 * initialization function.
 */

#include <pthread.h>

#include "debuglog.h"
#include "rclmon.h"
#include "debuglog.h"
#include "indexer.h"

typedef map<string, RclMonEvent> queue_type;

class RclEQData {
public:
    queue_type m_queue;
    RclConfig *m_config;
    bool       m_ok;
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
    RclEQData() 
	: m_config(0), m_ok(false)
    {
	if (!pthread_mutex_init(&m_mutex, 0) && !pthread_cond_init(&m_cond, 0))
	    m_ok = true;
    }
};

RclMonEventQueue rclEQ;

RclMonEventQueue::RclMonEventQueue()
{
    m_data = new RclEQData;
}
RclMonEventQueue::~RclMonEventQueue()
{
    delete m_data;
}
bool RclMonEventQueue::empty()
{
    return m_data == 0 ? true : m_data->m_queue.empty();
}

RclMonEvent RclMonEventQueue::pop()
{
    RclMonEvent ev;
    if (!empty()) {
	ev = m_data->m_queue.begin()->second;
	m_data->m_queue.erase(m_data->m_queue.begin());
    }
    return ev;
}

/** Wait until there is something to process on the queue.
 *  Must be called with the queue locked 
 */
bool RclMonEventQueue::wait() 
{
    if (!empty())
	return true;
    if (pthread_cond_wait(&m_data->m_cond, &m_data->m_mutex)) {
	LOGERR(("RclMonEventQueue::wait: pthread_cond_wait failed\n"));
	return false;
    }
    return true;
}

bool RclMonEventQueue::lock()
{
    if (pthread_mutex_lock(&m_data->m_mutex)) {
	LOGERR(("RclMonEventQueue::lock: pthread_mutex_lock failed\n"));
	return false;
    }
    return true;
}
bool RclMonEventQueue::unlock()
{
    if (pthread_mutex_unlock(&m_data->m_mutex)) {
	LOGERR(("RclMonEventQueue::lock: pthread_mutex_unlock failed\n"));
	return false;
    }
    return true;
}

void RclMonEventQueue::setConfig(RclConfig *cnf)
{
    m_data->m_config = cnf;
}

RclConfig *RclMonEventQueue::getConfig()
{
    return m_data->m_config;
}

extern int stopindexing;

bool RclMonEventQueue::ok()
{
    if (m_data == 0)
	return false;
    return !stopindexing && m_data->m_ok;
}

void RclMonEventQueue::setTerminate()
{
    lock();
    m_data->m_ok = false;
    pthread_cond_broadcast(&m_data->m_cond);
    unlock();
}

bool RclMonEventQueue::pushEvent(const RclMonEvent &ev)
{
    LOGDEB2(("RclMonEventQueue::pushEvent for %s\n", ev.m_path.c_str()));
    lock();
    // It seems that a newer event always override any older. TBVerified ?
    m_data->m_queue[ev.m_path] = ev;
    pthread_cond_broadcast(&m_data->m_cond);
    unlock();
    return true;
}

pthread_t rcv_thrid;
void *rcv_result;
extern void *rclMonRcvRun(void *);

bool startMonitor(RclConfig *conf, bool nofork)
{
    rclEQ.setConfig(conf);
    if (pthread_create(&rcv_thrid, 0, &rclMonRcvRun, &rclEQ) != 0) {
	LOGERR(("start_monitoring: cant create event-receiving thread\n"));
	return false;
    }

    if (!rclEQ.lock()) {
	return false;
    }
    LOGDEB(("start_monitoring: entering main loop\n"));
    while (rclEQ.wait()) {
	LOGDEB2(("startMonitor: wait returned\n"));
	if (!rclEQ.ok())
	    break;
	list<string> modified;
	list<string> deleted;

	// Process event queue
	while (!rclEQ.empty()) {
	    // Retrieve event
	    RclMonEvent ev = rclEQ.pop();
	    switch (ev.m_etyp) {
	    case RclMonEvent::RCLEVT_MODIFY:
		LOGDEB(("Monitor: Modify/Check on %s\n", ev.m_path.c_str()));
		modified.push_back(ev.m_path);
		break;
	    case RclMonEvent::RCLEVT_DELETE:
		LOGDEB(("Monitor: Delete on %s\n", ev.m_path.c_str()));
		deleted.push_back(ev.m_path);
		break;
	    case RclMonEvent::RCLEVT_RENAME:
		LOGDEB(("Monitor: Rename on %s\n", ev.m_path.c_str()));
		break;
	    default:
		LOGDEB(("Monitor: got Other on %s\n", ev.m_path.c_str()));
	    }
	}
	// Unlock queue before processing lists
	rclEQ.unlock();
	// Process
	if (!indexfiles(conf, modified))
	    break;
	if (!purgefiles(conf, deleted))
	    break;
	// Lock queue before waiting again
	rclEQ.lock();
    }
    return true;
}
#endif // RCL_MONITOR
