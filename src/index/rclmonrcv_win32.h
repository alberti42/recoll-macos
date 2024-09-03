// rclmonrcv_win32.h

#ifndef RCLMONRCV_WIN32_H
#define RCLMONRCV_WIN32_H

#include "rclmonrcv.h"

#ifdef FSWATCH_WIN32

// Adapter for the rclmon interface
class RclMonitorWin32 : public RclMonitorFactory<RclMonitorWin32>, public FileWatchListener {
public:
    ~RclMonitorWin32() {}
    bool addWatch(const string& path, bool /*isDir*/, bool /*follow*/);

    bool getEvent(RclMonEvent& ev, int msecs = -1);

    bool ok() const;

    // Does this monitor generate 'exist' events at startup?
    virtual bool generatesExist() const;

    // Can the caller avoid setting watches on subdirs ?
    virtual bool isRecursive() const;

    virtual void handleFileAction(WatchID /*watchid*/, const std::string& dir, const std::string& fn,
                                  Action action, bool isdir, std::string oldfn = "");

    // Save significant errno after monitor calls
    int saved_errno{0};
private:
    std::queue<RclMonEvent> m_events;
    RclFSWatchWin32 m_fswatcher;
};

#endif // FSWATCH_WIN32

#endif // RCLMONRCV_WIN32_H