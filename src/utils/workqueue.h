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

#include <pthread.h>
#include <time.h>

#include <string>
#include <queue>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
using std::tr1::unordered_map;
using std::tr1::unordered_set;
using std::queue;
using std::string;

#include "debuglog.h"

#define WORKQUEUE_TIMING

class WQTData {
    public:
    WQTData() {wstart.tv_sec = 0; wstart.tv_nsec = 0;}
    struct timespec wstart;
};

/**
 * A WorkQueue manages the synchronisation around a queue of work items,
 * where a number of client threads queue tasks and a number of worker
 * threads takes and executes them. The goal is to introduce some level
 * of parallelism between the successive steps of a previously single
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

    /** Create a WorkQueue
     * @param name for message printing
     * @param hi number of tasks on queue before clients blocks. Default 0 
     *    meaning no limit.
     * @param lo minimum count of tasks before worker starts. Default 1.
     */
    WorkQueue(const string& name, int hi = 0, int lo = 1)
        : m_name(name), m_high(hi), m_low(lo), m_size(0), 
          m_workers_waiting(0), m_workers_exited(0), m_jobcnt(0), 
          m_clientwait(0), m_workerwait(0), m_workerwork(0)
    {
        m_ok = (pthread_cond_init(&m_cond, 0) == 0) && 
            (pthread_mutex_init(&m_mutex, 0) == 0);
    }

    ~WorkQueue() 
    {
        LOGDEB2(("WorkQueue::~WorkQueue: name %s\n", m_name.c_str()));
        if (!m_worker_threads.empty())
            setTerminateAndWait();
    }

    /** Start the worker threads. 
     *
     * @param nworkers number of threads copies to start.
     * @param start_routine thread function. It should loop
     *      taking (QueueWorker::take() and executing tasks. 
     * @param arg initial parameter to thread function.
     * @return true if ok.
     */
    bool start(int nworkers, void *(*start_routine)(void *), void *arg)
    {
        for  (int i = 0; i < nworkers; i++) {
            int err;
            pthread_t thr;
            if ((err = pthread_create(&thr, 0, start_routine, arg))) {
                LOGERR(("WorkQueue:%s: pthread_create failed, err %d\n",
                        m_name.c_str(), err));
                return false;
            }
            m_worker_threads.insert(pair<pthread_t, WQTData>(thr, WQTData()));
        }
        return true;
    }

    /** Add item to work queue, called from client.
     *
     * Sleeps if there are already too many.
     */
    bool put(T t)
    {
        if (!ok() || pthread_mutex_lock(&m_mutex) != 0) 
            return false;

#ifdef WORKQUEUE_TIMING
        struct timespec before;
        clock_gettime(CLOCK_MONOTONIC, &before);
#endif

        while (ok() && m_high > 0 && m_queue.size() >= m_high) {
            // Keep the order: we test ok() AFTER the sleep...
            if (pthread_cond_wait(&m_cond, &m_mutex) || !ok()) {
                pthread_mutex_unlock(&m_mutex);
                return false;
            }
        }

#ifdef WORKQUEUE_TIMING
        struct timespec after;
        clock_gettime(CLOCK_MONOTONIC, &after);
        m_clientwait += nanodiff(before, after);
#endif

        m_queue.push(t);
        ++m_size;
	// Just wake one worker, there is only one new task.
        pthread_cond_signal(&m_cond);
        pthread_mutex_unlock(&m_mutex);
        return true;
    }

    /** Wait until the queue is inactive. Called from client.
     *
     * Waits until the task queue is empty and the workers are all
     * back sleeping. Used by the client to wait for all current work
     * to be completed, when it needs to perform work that couldn't be
     * done in parallel with the worker's tasks, or before shutting
     * down. Work can be resumed after calling this.
     */
    bool waitIdle()
    {
        if (!ok() || pthread_mutex_lock(&m_mutex) != 0) {
            LOGERR(("WorkQueue::waitIdle: %s not ok or can't lock\n",
                    m_name.c_str()));
            return false;
        }

        // We're done when the queue is empty AND all workers are back
        // waiting for a task.
        while (ok() && (m_queue.size() > 0 || 
                        m_workers_waiting != m_worker_threads.size())) {
            if (pthread_cond_wait(&m_cond, &m_mutex)) {
                pthread_mutex_unlock(&m_mutex);
                m_ok = false;
                LOGERR(("WorkQueue::waitIdle: cond_wait failed\n"));
                return false;
            }
        }

#ifdef WORKQUEUE_TIMING
        long long M = 1000000LL;
        long long wscl = m_worker_threads.size() * M;
        LOGERR(("WorkQueue:%s: clients wait (all) %lld mS, "
                 "worker wait (avg) %lld mS, worker work (avg) %lld mS\n", 
                 m_name.c_str(), m_clientwait / M, m_workerwait / wscl,  
                 m_workerwork / wscl));
#endif // WORKQUEUE_TIMING

        pthread_mutex_unlock(&m_mutex);
        return ok();
    }


    /** Tell the workers to exit, and wait for them. Does not bother about
     * tasks possibly remaining on the queue, so should be called
     * after waitIdle() for an orderly shutdown.
     */
    void* setTerminateAndWait()
    {
        LOGDEB(("setTerminateAndWait:%s\n", m_name.c_str()));
        pthread_mutex_lock(&m_mutex);

	if (m_worker_threads.empty()) {
	    // Already called ?
	    return (void*)0;
	}

	// Wait for all worker threads to have called workerExit()
        m_ok = false;
        while (m_workers_exited < m_worker_threads.size()) {
            pthread_cond_broadcast(&m_cond);
            if (pthread_cond_wait(&m_cond, &m_mutex)) {
                pthread_mutex_unlock(&m_mutex);
                LOGERR(("WorkQueue::setTerminate: cond_wait failed\n"));
                return false;
            }
        }

	// Perform the thread joins and compute overall status
        // Workers return (void*)1 if ok
        void *statusall = (void*)1;
        unordered_map<pthread_t,  WQTData>::iterator it;
        while (!m_worker_threads.empty()) {
            void *status;
            it = m_worker_threads.begin();
            pthread_join(it->first, &status);
            if (status == (void *)0)
                statusall = status;
            m_worker_threads.erase(it);
        }
        pthread_mutex_unlock(&m_mutex);
        LOGDEB(("setTerminateAndWait:%s done\n", m_name.c_str()));
        return statusall;
    }

    /** Take task from queue. Called from worker.
     * 
     * Sleeps if there are not enough. Signal if we go
     * to sleep on empty queue: client may be waiting for our going idle.
     */
    bool take(T* tp)
    {
        if (!ok() || pthread_mutex_lock(&m_mutex) != 0)
            return false;

#ifdef WORKQUEUE_TIMING
        struct timespec beforesleep;
        clock_gettime(CLOCK_MONOTONIC, &beforesleep);

        pthread_t me = pthread_self();
        unordered_map<pthread_t, WQTData>::iterator it = 
            m_worker_threads.find(me);
        if (it != m_worker_threads.end() && 
            it->second.wstart.tv_sec != 0 && it->second.wstart.tv_nsec != 0) {
            long long nanos = nanodiff(it->second.wstart, beforesleep);
            m_workerwork += nanos;
        }
#endif

        while (ok() && m_queue.size() < m_low) {
            m_workers_waiting++;
            if (m_queue.empty())
                pthread_cond_broadcast(&m_cond);
            if (pthread_cond_wait(&m_cond, &m_mutex) || !ok()) {
		// !ok is a normal condition when shutting down
		if (ok())
		    LOGERR(("WorkQueue::take:%s: cond_wait failed or !ok\n",
			    m_name.c_str()));
                pthread_mutex_unlock(&m_mutex);
                m_workers_waiting--;
                return false;
            }
            m_workers_waiting--;
        }

#ifdef WORKQUEUE_TIMING
        struct timespec aftersleep;
        clock_gettime(CLOCK_MONOTONIC, &aftersleep);
        m_workerwait += nanodiff(beforesleep, aftersleep);
        it = m_worker_threads.find(me);
        if (it != m_worker_threads.end())
            it->second.wstart = aftersleep;
#endif

        ++m_jobcnt;
        *tp = m_queue.front();
        m_queue.pop();
        --m_size;
	// No reason to wake up more than one client thread
        pthread_cond_signal(&m_cond);
        pthread_mutex_unlock(&m_mutex);
        return true;
    }

    /** Advertise exit and abort queue. Called from worker
     * This would normally happen after an unrecoverable error, or when 
     * the queue is terminated by the client. Workers never exit normally,
     * except when the queue is shut down (at which point m_ok is set to false
     * by the shutdown code anyway). The thread must return/exit immediately 
     * after calling this
     */
    void workerExit()
    {
        if (pthread_mutex_lock(&m_mutex) != 0)
            return;
        m_workers_exited++;
        m_ok = false;
        pthread_cond_broadcast(&m_cond);
        pthread_mutex_unlock(&m_mutex);
    }

    /** Return current queue size. Debug only.
     *
     *  As the size is returned while the queue is unlocked, there
     *  is no warranty on its consistency. Not that we use the member
     *  size, not the container size() call which would need locking.
     */
    size_t size() 
    {
        return m_size;
    }

private:
    bool ok() 
    {
        return m_ok && m_workers_exited == 0 && !m_worker_threads.empty();
    }

    long long nanodiff(const struct timespec& older, 
                       const struct timespec& newer)
    {
        return (newer.tv_sec - older.tv_sec) * 1000000000LL
            + newer.tv_nsec - older.tv_nsec;
    }

    string m_name;
    size_t m_high;
    size_t m_low; 
    size_t m_size;
    /* Worker threads currently waiting for a job */
    unsigned int m_workers_waiting;
    unsigned int m_workers_exited;
    /* Stats */
    int m_jobcnt;
    long long m_clientwait;
    long long m_workerwait;
    long long m_workerwork;

    unordered_map<pthread_t, WQTData> m_worker_threads;
    queue<T> m_queue;
    pthread_cond_t m_cond;
    pthread_mutex_t m_mutex;
    bool m_ok;
};

#endif /* _WORKQUEUE_H_INCLUDED_ */
