#ifndef RCLMONRCV_H
#define RCLMONRCV_H

#ifdef RCL_MONITOR

// Set the FSWATCH to the chosen library.
#ifdef _WIN32
    #define FSWATCH_WIN32
#else // ! _WIN32
    #ifdef RCL_USE_FSEVENTS // darwin (i.e., MACOS)
        #define FSWATCH_FSEVENTS
    #else // ! RCL_USE_FSEVENTS
        // We dont compile both the inotify and the fam interface and inotify has preference
        #ifdef RCL_USE_INOTIFY
            #define FSWATCH_INOTIFY
        #else // ! RCL_USE_INOTIFY
            #ifdef RCL_USE_FAM
                #define FSWATCH_FAM
            #endif // RCL_USE_FAM
        #endif // INOTIFY
    #endif // RCL_USE_FSEVENTS

    #ifndef FSWATCH_WIN32
        #ifndef FSWATCH_FSEVENTS
            #ifndef FSWATCH_INOTIFY
                #ifndef FSWATCH_FAM
                    #error "Something went wrong. RCL_MONITOR is true, but all _WIN32, RCL_USE_INOTIFY, RCL_USE_FAM, and RCL_USE_FSEVENTS are undefined."
                #endif // FSWATCH_FAM
            #endif // FSWATCH_INOTIFY
        #endif // FSWATCH_FSEVENTS
    #endif // FSWATCH_WIN32
#endif // _WIN32

#include <string>
#include <vector>
#include <map>

#include <errno.h>
#include <cstdio>
#include <cstring>
#include "safeunistd.h"

#include "log.h"
#include "rclmon.h"
#include "rclinit.h"
#include "fstreewalk.h"
#include "pathut.h"
#include "smallut.h"

using std::string;
using std::vector;
using std::map;

/** Virtual interface for the actual filesystem monitoring module. */

class RclMonitor {
public:
    RclMonitor();
    virtual ~RclMonitor();

    virtual bool addWatch(const std::string& path, bool isDir, bool follow = false) = 0;

    #ifdef FSWATCH_FSEVENTS
        virtual void startMonitoring(RclMonEventQueue *queue, RclConfig& lconfig, FsTreeWalker& walker) = 0;
    #else
        virtual bool getEvent(RclMonEvent& ev, int msecs = -1) = 0;
    #endif

    virtual bool ok() const = 0;
    virtual bool generatesExist();
    virtual bool isRecursive();

    // Save significant errno after monitor calls
    int saved_errno{0};
};

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
};

#endif // FSWATCH_FSEVENTS

#endif // RCL_MONITOR

#endif // RCLMONRCV_H