#ifndef RCLMONRCV_FSEVENTS_H
#define RCLMONRCV_FSEVENTS_H

#include "rclmonrcv.h"

#ifdef FSWATCH_FSEVENTS

#include <CoreServices/CoreServices.h>
#include <iostream>
#include <map>
#include <vector>

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

    void removePathFromMonitor(const std::string &path);

private:
    RclConfig* lconfigPtr;
    FsTreeWalker* walkerPtr;
    bool m_ok;
    RclMonEventQueue *queue;
    FSEventStreamRef m_stream;
    std::vector<CFStringRef> m_pathsToWatch;
    std::vector<RclMonEvent> m_eventQueue;
    CFRunLoopSourceRef runLoopSource;
};

typedef RclFSEvents RclMonitorDerived;

#endif // FSWATCH_FSEVENTS

#endif // RCLMONRCV_FSEVENTS_H