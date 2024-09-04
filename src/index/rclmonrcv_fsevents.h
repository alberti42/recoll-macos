// rclmonrcv_fsevents.h

#ifndef RCLMONRCV_FSEVENTS_H
#define RCLMONRCV_FSEVENTS_H

#include "rclmonrcv.h"

#ifdef FSWATCH_FSEVENTS

#include <CoreServices/CoreServices.h>
#include <iostream>
#include <map>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

class RclFSEvents : public RclMonitor {
public:
    RclFSEvents();
    virtual ~RclFSEvents() override;

    static void fsevents_callback(
        ConstFSEventStreamRef streamRef,
        void *clientCallBackInfo,
        size_t numEvents,
        void *eventPaths,
        const FSEventStreamEventFlags eventFlags[],
        const FSEventStreamEventId eventIds[]);

    bool virtual getEvent(RclMonEvent& ev, int msecs = -1) override {
        // we do not need this functio for RclFSEvents; it should return false
        return false;
    };
    bool virtual ok() const override;
#ifdef FSWATCH_FSEVENTS
    void virtual startMonitoring(RclMonEventQueue *queue, RclConfig& lconfig, FsTreeWalker& walker) override;
#endif
    void virtual setupAndStartStream();
    bool virtual addWatch(const std::string& path, bool isDir, bool follow = false) override;

    bool virtual isRecursive() override;

public:
    RclMonEventQueue *m_queue{NULL};

private:
    int eraseWatchSubTree(CFStringRef topDirectory);
    static void signalHandler(int signum);
    void releaseFSEventStream();
    RclConfig* m_lconfigPtr;
    FsTreeWalker* m_walkerPtr;
    bool m_ok;
    FSEventStreamRef m_stream;
    string m_rootPath;    
};

// Custom context for the idle loop
typedef struct  {
    bool shouldExit;
    RclFSEvents *monitor;
} IdleLoopContext;

#endif // FSWATCH_FSEVENTS

#endif // RCLMONRCV_FSEVENTS_H