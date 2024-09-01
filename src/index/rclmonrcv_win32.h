#ifndef RCLMONRCV_WIN32_H
#define RCLMONRCV_WIN32_H

#include "rclmonrcv.h"

#ifdef FSWATCH_WIN32

// Adapter for the rclmon interface
class RclMonitorWin32 : public RclMonitor, public FileWatchListener {
public:
    virtual ~RclMonitorWin32() {}
    virtual bool addWatch(const string& path, bool /*isDir*/, bool /*follow*/) override;

    virtual bool getEvent(RclMonEvent& ev, int msecs = -1) override;

    virtual bool ok() const override;

    // Does this monitor generate 'exist' events at startup?
    virtual bool generatesExist() const override;

    // Can the caller avoid setting watches on subdirs ?
    virtual bool isRecursive() const override;

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