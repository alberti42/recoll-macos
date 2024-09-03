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

class RclFSEvents : public RclMonitorFactory<RclFSEvents> {
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

    CFRunLoopRef m_runLoop;

    bool isRecursive();

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
    pid_t parentPid;
    RclFSEvents *monitor;
} IdleLoopContext;

#endif // FSWATCH_FSEVENTS

#endif // RCLMONRCV_FSEVENTS_H