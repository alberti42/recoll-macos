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
#ifndef _WORKQUEUE_H_INCLUDED_
#define _WORKQUEUE_H_INCLUDED_

#include "pthread.h"
#include <string>
#include <queue>
using std::queue;
using std::string;

/**
 * A WorkQueue manages the synchronisation around a queue of work items,
 * where a single client thread queues tasks and a single worker takes
 * and executes them. The goal is to introduce some level of
 * parallelism between the successive steps of a previously single
 * threaded pipe-line (data extraction / data preparation / index
 * update).
 *
 * There is no individual task status return. In case of fatal error,
 * the client or worker sets an end condition on the queue. A second
 * queue could conceivably be used for returning individual task
 * status.
 */
template <class T> class WorkQueue {
public:
    WorkQueue(int hi = 0, int lo = 1)
	: m_high(hi), m_low(lo), m_size(0), m_worker_up(false),
	  m_worker_waiting(false), m_jobcnt(0), m_lenacc(0)
    {
	m_ok = (pthread_cond_init(&m_cond, 0) == 0) && 
	    (pthread_mutex_init(&m_mutex, 0) == 0);
    }

    ~WorkQueue() 
    {
	if (m_worker_up)
	    setTerminateAndWait();
    }

    /** Start the worker thread. The start_routine will loop
     *  taking and executing tasks. */
    bool start(void *(*start_routine)(void *), void *arg)
    {
	bool status = pthread_create(&m_worker_thread, 0, 
				     start_routine, arg) == 0;
	if (status)
	    m_worker_up = true;
	return status;
    }

    /**
     * Add item to work queue. Sleep if there are already too many.
     * Called from client.
     */
    bool put(T t)
    {
	if (!ok() || pthread_mutex_lock(&m_mutex) != 0) 
	    return false;

	while (ok() && m_high > 0 && m_queue.size() >= m_high) {
	    // Keep the order: we test ok() AFTER the sleep...
	    if (pthread_cond_wait(&m_cond, &m_mutex) || !ok()) {
		pthread_mutex_unlock(&m_mutex);
		return false;
	    }
	}

	m_queue.push(t);
	++m_size;
	pthread_cond_broadcast(&m_cond);
	pthread_mutex_unlock(&m_mutex);
	return true;
    }

    /** Wait until the queue is empty and the worker is
     *  back waiting for task. Called from the client when it needs to
     *  perform work that couldn't be done in parallel with the
     *  worker's tasks.
     */
    bool waitIdle()
    {
	if (!ok() || pthread_mutex_lock(&m_mutex) != 0) 
	    return false;

	// We're done when the queue is empty AND the worker is back
	// for a task (has finished the last)
	while (ok() && (m_queue.size() > 0 || !m_worker_waiting)) {
	    if (pthread_cond_wait(&m_cond, &m_mutex)) {
		pthread_mutex_unlock(&m_mutex);
		return false;
	    }
	}
	pthread_mutex_unlock(&m_mutex);
	return ok();
    }

    /** Tell the worker to exit, and wait for it. There may still
	be tasks on the queue. */
    void* setTerminateAndWait()
    {
	if (!m_worker_up)
	    return (void *)0;

	pthread_mutex_lock(&m_mutex);
	m_ok = false;
	pthread_cond_broadcast(&m_cond);
	pthread_mutex_unlock(&m_mutex);
	void *status;
	pthread_join(m_worker_thread, &status);
	m_worker_up = false;
	return status;
    }

    /** Remove task from queue. Sleep if there are not enough. Signal if we go
	to sleep on empty queue: client may be waiting for our going idle */
    bool take(T* tp)
    {
	if (!ok() || pthread_mutex_lock(&m_mutex) != 0)
	    return false;

	while (ok() && m_queue.size() < m_low) {
	    m_worker_waiting = true;
	    if (m_queue.empty())
		pthread_cond_broadcast(&m_cond);
	    if (pthread_cond_wait(&m_cond, &m_mutex) || !ok()) {
		pthread_mutex_unlock(&m_mutex);
		m_worker_waiting = false;
		return false;
	    }
	    m_worker_waiting = false;
	}

	++m_jobcnt;
	m_lenacc += m_size;

	*tp = m_queue.front();
	m_queue.pop();
	--m_size;

	pthread_cond_broadcast(&m_cond);
	pthread_mutex_unlock(&m_mutex);
	return true;
    }

    /** Take note of the worker exit. This would normally happen after an
	unrecoverable error */
    void workerExit()
    {
	if (!ok() || pthread_mutex_lock(&m_mutex) != 0)
	    return;
	m_ok = false;
	pthread_cond_broadcast(&m_cond);
	pthread_mutex_unlock(&m_mutex);
    }

    /** Debug only: as the size is returned while the queue is unlocked, there
     *  is no warranty on its consistency. Not that we use the member size, not 
     *  the container size() call which would need locking.
     */
    size_t size() {return m_size;}

private:
    bool ok() {return m_ok && m_worker_up;}

    size_t m_high;
    size_t m_low; 
    size_t m_size;
    bool m_worker_up;
    bool m_worker_waiting;
    int m_jobcnt;
    int m_lenacc;

    pthread_t m_worker_thread;
    queue<T> m_queue;
    pthread_cond_t m_cond;
    pthread_mutex_t m_mutex;
    bool m_ok;
};

#endif /* _WORKQUEUE_H_INCLUDED_ */
