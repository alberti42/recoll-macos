#ifndef _RCLMON_H_INCLUDED_
#define _RCLMON_H_INCLUDED_
#include "autoconfig.h"
#ifdef RCL_MONITOR
/* @(#$Id: rclmon.h,v 1.5 2006-10-24 14:28:38 dockes Exp $  (C) 2006 J.F.Dockes */
/**
 * Definitions for the real-time monitoring recoll. 
 * We're interested in file modifications, deletions and renaming.
 * We use two threads, one to receive events from the source, the
 * other to perform adequate processing.
 *
 * The two threads communicate through an event buffer which is
 * actually a hash map indexed by file path for easy coalescing of
 * multiple events to the same file.
 */

#include <string>
#include <map>

#include "rclconfig.h"

#ifndef NO_NAMESPACES
using std::string;
using std::multimap;
#endif

/**
 * Monitoring event: something changed in the filesystem
 */
class RclMonEvent {
 public: 
    enum EvType {RCLEVT_NONE, RCLEVT_MODIFY, RCLEVT_DELETE, 
		 RCLEVT_DIRCREATE, RCLEVT_RENAME};
    string m_path;
    string m_opath;
    EvType m_etyp;
    RclMonEvent() : m_etyp(RCLEVT_NONE) {}
};

/**
 * Monitoring event queue. This is the shared object between the main thread 
 * (which does the actual indexing work), and the monitoring thread which 
 * receives events from FAM / inotify / etc.
 */
class RclEQData;
class RclMonEventQueue {
 public:
    RclMonEventQueue();
    ~RclMonEventQueue();
    /** Unlock queue and wait until there are new events. 
     *  Returns with the queue locked */
    bool wait(int secs = -1, bool *timedout = 0);
    /** Unlock queue */
    bool unlock();
    /** Lock queue. */
    bool lock();
    /** Lock queue and add event. */
    bool pushEvent(const RclMonEvent &ev);
    void setTerminate(); /* To all threads: end processing */
    bool ok();
    bool empty();
    RclMonEvent pop();

    // Convenience function for initially communicating config to mon thr
    void setConfig(RclConfig *conf);
    RclConfig *getConfig();

 private:
    RclEQData *m_data;
};

/** Start monitoring on the topdirs described in conf */
extern bool startMonitor(RclConfig *conf, bool nofork);

/** Main routine for the event receiving thread */
extern void *rclMonRcvRun(void *);

/** There can only be one of us. Name of the lock file inside the config
 * directory. We write our pid in there */
#define RCL_MONITOR_PIDFILENAME "rclmonpid"

#endif // RCL_MONITOR
#endif /* _RCLMON_H_INCLUDED_ */
