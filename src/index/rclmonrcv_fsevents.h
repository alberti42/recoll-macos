// rclmonrcv_fsevents.h

#ifndef RCLMONRCV_FSEVENTS_H
#define RCLMONRCV_FSEVENTS_H

#include "rclmonrcv.h"

#ifdef FSWATCH_FSEVENTS

// This macro is supposed to be commented out
// This disable a whole part of code that was written but is at the moment
// not useful. It is unclear whether it will be necessary in the future.
// #define MANAGE_SEPARATE_QUEUE

#include <CoreServices/CoreServices.h>
#include <iostream>
#include <map>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

#ifdef MANAGE_SEPARATE_QUEUE
#include <mutex>
#endif // MANAGE_SEPARATE_QUEUE

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
    
#ifdef MANAGE_SEPARATE_QUEUE
    std::vector<RclMonEvent> m_eventQueue;
    std::mutex m_queueMutex; // Mutex to protect the event queue
#endif // MANAGE_SEPARATE_QUEUE
};

// Custom context for the idle loop
typedef struct  {
    bool shouldExit;
    pid_t parentPid;
    RclFSEvents *monitor;
} IdleLoopContext;

#endif // FSWATCH_FSEVENTS

#endif // RCLMONRCV_FSEVENTS_H