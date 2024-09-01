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
    virtual ~RclFSEvents();

    static void fsevents_callback(
        ConstFSEventStreamRef streamRef,
        void *clientCallBackInfo,
        size_t numEvents,
        void *eventPaths,
        const FSEventStreamEventFlags eventFlags[],
        const FSEventStreamEventId eventIds[]);

    bool getEvent(RclMonEvent& ev, int msecs = -1);

    virtual bool ok() const override;

    void startMonitoring(RclMonEventQueue *queue, RclConfig& lconfig, FsTreeWalker& walker) override;
    void setupAndStartStream();

    virtual bool addWatch(const std::string& path, bool /*isDir*/, bool /*follow*/) override;

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

#endif // FSWATCH_FSEVENTS

#endif // RCLMONRCV_FSEVENTS_H