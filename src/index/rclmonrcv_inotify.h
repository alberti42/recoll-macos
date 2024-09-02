// rclmonrcv_inotify.h

#ifndef RCLMONRCV_INOTIFY_H
#define RCLMONRCV_INOTIFY_H

#include "rclmonrcv.h"

#ifdef FSWATCH_INOTIFY

#define EVBUFSIZE (32*1024)

class RclIntf : public RclMonitor {
public:
    RclIntf();
    ~RclIntf();

    bool addWatch(const string& path, bool isdir, bool follow);
    bool getEvent(RclMonEvent& ev, int msecs = -1);
    bool ok() const;

private:
    bool m_ok;
    int m_fd;
    map<int,string> m_idtopath; // Watch descriptor to name
    char m_evbuf[EVBUFSIZE]; // Event buffer
    char *m_evp; // Pointer to next event or 0
    char *m_ep;  // Pointer to end of events
    const char *event_name(int code);
    void close();
};

#endif // FSWATCH_INOTIFY

#endif // RCLMONRCV_INOTIFY_H