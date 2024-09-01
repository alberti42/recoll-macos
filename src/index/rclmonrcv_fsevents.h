// rclmonrcv_fsevents.h

#ifndef RCLMONRCV_FSEVENTS_H
#define RCLMONRCV_FSEVENTS_H

#include "rclmonrcv.h"

#ifdef FSWATCH_FSEVENTS

#define MANAGE_SEPARATE_QUEUE

#include <CoreServices/CoreServices.h>
#include <iostream>
#include <map>
#include <vector>

#ifdef MANAGE_SEPARATE_QUEUE
#include <mutex>
#endif // MANAGE_SEPARATE_QUEUE

class RclFSEvents : public RclMonitor {
public:
    RclFSEvents();
    ~RclFSEvents();

    static void fsevents_callback(
        ConstFSEventStreamRef streamRef,
        void *clientCallBackInfo,
        size_t numEvents,
        void *eventPaths,
        const FSEventStreamEventFlags eventFlags[],
        const FSEventStreamEventId eventIds[]);

    bool getEvent(RclMonEvent& ev, int msecs = -1);

    bool ok() const;

    void startMonitoring(RclMonEventQueue *queue, RclConfig& lconfig, FsTreeWalker& walker);
    void setupAndStartStream();

    bool addWatch(const std::string& path, bool isDir, bool follow = false);
    void removeWatch(const std::string &path);

private:
    void removeFSEventStream();

    RclConfig* lconfigPtr;
    FsTreeWalker* walkerPtr;
    bool m_ok;
    RclMonEventQueue *queue;
    FSEventStreamRef m_stream;
    std::vector<CFStringRef> m_pathsToWatch;
    CFRunLoopSourceRef m_runLoopSource;
    CFRunLoopRef m_runLoop;
    
#ifdef MANAGE_SEPARATE_QUEUE
    void removeSeparateQueueLoop();
    std::vector<RclMonEvent> m_eventQueue;
    std::mutex m_queueMutex; // Mutex to protect the event queue
#endif // MANAGE_SEPARATE_QUEUE
};

typedef RclFSEvents RclMonitorDerived;

#endif // FSWATCH_FSEVENTS

#endif // RCLMONRCV_FSEVENTS_H