// rclmonrcv.h

#ifndef RCLMONRCV_H
#define RCLMONRCV_H

#ifdef RCL_MONITOR

// Set the FSWATCH to the chosen library.
#ifdef _WIN32
    #define FSWATCH_WIN32
    class RclMonitorWin32;
    using RclMonitor = RclMonitorWin32; // Create an alias for RclMonitorWin32
#else // ! _WIN32
    #ifdef RCL_USE_FSEVENTS // darwin (i.e., MACOS)
        #define FSWATCH_FSEVENTS
        class RclFSEvents;
        using RclMonitor = RclFSEvents; // Create an alias for RclFSEvents
    #else // ! RCL_USE_FSEVENTS
        // We dont compile both the inotify and the fam interface and inotify has preference
        #ifdef RCL_USE_INOTIFY
            #define FSWATCH_INOTIFY
            class RclIntf;
            using RclMonitor = RclIntf; // Create an alias for RclIntf
        #else // ! RCL_USE_INOTIFY
            #ifdef RCL_USE_FAM
                #define FSWATCH_FAM
                class RclFAM;
                using RclMonitor = RclFAM; // Create an alias for RclFAM
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

/** Template for the actual filesystem monitoring module.**/
template <typename T>
class RclMonitorFactory {
public:
    // The functions below must be defined in the class constructed from the template
    // or else an error at compile time will be generated. Unfortunately, C++ does not
    // provide a keyword as `abstract` to state that a function is pure and hsa to be provided
    // in the derived class. Pure functions in C++ exist but always come with dynamical dispatching,
    // which bring a slight overhead. The current implementation is safe, uses less code, and
    // could be more efficient as it does not rely on dynamical dispatching. The only downside is the
    // need of this clarificatory message here.

    // bool addWatch(const std::string& path, bool isDir, bool follow = false);
    // bool ok() const;
    // #ifdef FSWATCH_FSEVENTS
    //     void startMonitoring(RclMonEventQueue *queue, RclConfig& lconfig, FsTreeWalker& walker);
    // #else
    //     bool getEvent(RclMonEvent& ev, int msecs = -1);
    // #endif
    
    // Does this monitor generate 'exist' events at startup?
    bool generatesExist() { return false; }
    // Is fs watch mechanism recursive and only the root directory needs to be provided?
    bool isRecursive() { return false; }

    // Save significant errno after monitor calls
    int saved_errno{0};
};

bool rclMonShouldSkip(const std::string& path, RclConfig& lconfig, FsTreeWalker& walker);
bool rclMonAddSubWatches(const std::string& path, FsTreeWalker& walker, RclConfig& lconfig,
            RclMonitor *mon, RclMonEventQueue *queue);
bool eraseWatchSubTree(std::map<int, std::string>& idtopath, const std::string& top);

#endif // RCL_MONITOR

#endif // RCLMONRCV_H
