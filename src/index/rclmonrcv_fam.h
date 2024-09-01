#ifndef RCLMONRCV_FAM_CPP
#define RCLMONRCV_FAM_CPP

#include "rclmonrcv.h"

#ifdef FSWATCH_FAM

/** FAM based monitor class. We have to keep a record of FAM watch
    request numbers to directory names as the event only contain the
    request number and file name, not the full path */
class RclFAM : public RclMonitor {
public:
    RclFAM();
    virtual ~RclFAM();
    virtual bool addWatch(const string& path, bool isdir, bool follow) override;
    virtual bool getEvent(RclMonEvent& ev, int msecs = -1) override;
    bool ok() override const {return m_ok;}
    virtual bool generatesExist() override const {return true;}

private:
    bool m_ok;
    FAMConnection m_conn;
    void close() {
        FAMClose(&m_conn);
        m_ok = false;
    }
    map<int,string> m_idtopath;
    const char *event_name(int code);
};

#endif // FSWATCH_FAM

#endif