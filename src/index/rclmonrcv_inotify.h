#ifndef RCLMONRCV_INOTIFY_CPP
#define RCLMONRCV_INOTIFY_CPP

#include "rclmonrcv.h"

#ifdef FSWATCH_INOTIFY

#define EVBUFSIZE (32*1024)

class RclIntf : public RclMonitor {
public:
    RclIntf();
    virtual ~RclIntf();

    virtual bool addWatch(const string& path, bool isdir, bool follow) override;
    virtual bool getEvent(RclMonEvent& ev, int msecs = -1) override;
    bool ok() const override;

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

#endif