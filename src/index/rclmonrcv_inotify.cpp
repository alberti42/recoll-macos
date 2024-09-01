#include "autoconfig.h"
#include "rclmonrcv_inotify.h"

#ifdef FSWATCH_INOTIFY
//////////////////////////////////////////////////////////////////////////
/** Inotify-based monitor class */
#include <sys/inotify.h>
#include <sys/select.h>

RclIntf::RclIntf(): m_ok(false), m_fd(-1), m_evp(nullptr), m_ep(nullptr) {
    if ((m_fd = inotify_init()) < 0) {
        LOGERR("RclIntf:: inotify_init failed, errno " << errno << "\n");
        return;
    }
    m_ok = true;
}

RclIntf::~RclIntf() {
    close();
}

bool RclIntf::ok() const {return m_ok;}

void RclIntf::close() {
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    m_ok = false;
}

const char *RclIntf::event_name(int code) 
{
    code &= ~(IN_ISDIR|IN_ONESHOT);
    switch (code) {
    case IN_ACCESS: return "IN_ACCESS";  // File was accessed
    case IN_MODIFY: return "IN_MODIFY";  // File was modified
    case IN_ATTRIB: return "IN_ATTRIB";  // Metadata changed (e.g., permissions, timestamps)
    case IN_CLOSE_WRITE: return "IN_CLOSE_WRITE";  // Writable file was closed
    case IN_CLOSE_NOWRITE: return "IN_CLOSE_NOWRITE";  // Unwritable file was closed
    case IN_CLOSE: return "IN_CLOSE";  // File was closed
    case IN_OPEN: return "IN_OPEN";  // File was opened
    case IN_MOVED_FROM: return "IN_MOVED_FROM";  // File or directory was moved from this location
    case IN_MOVED_TO: return "IN_MOVED_TO";  // File or directory was moved to this location
    case IN_MOVE: return "IN_MOVE";  // File was moved
    case IN_CREATE: return "IN_CREATE";  // File or directory was created
    case IN_DELETE: return "IN_DELETE";  // File or directory was deleted
    case IN_DELETE_SELF: return "IN_DELETE_SELF";  // The monitored item itself was deleted
    case IN_MOVE_SELF: return "IN_MOVE_SELF";  // The monitored item itself was moved
    case IN_UNMOUNT: return "IN_UNMOUNT";  // Filesystem was unmounted
    case IN_Q_OVERFLOW: return "IN_Q_OVERFLOW";  // Event queue overflowed
    case IN_IGNORED: return "IN_IGNORED";  // Watch was removed
    default: {
        static char msg[50];
        sprintf(msg, "Unknown event 0x%x", code);
        return msg;
    }
    };
}

bool RclIntf::addWatch(const string& path, bool, bool follow)
{
    if (!ok()) {
        return false;
    }
    MONDEB("RclIntf::addWatch: adding " << path << " follow " << follow << "\n");
    
    // Define the set of events we are interested in.
    // CLOSE_WRITE is covered through MODIFY. CREATE is needed for mkdirs
    uint32_t mask = IN_MODIFY | IN_CREATE
        | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE
        // IN_ATTRIB used to be not needed to receive extattr
        // modification events, which was a bit weird because only ctime is
        // set, and now it is...
        | IN_ATTRIB
#ifdef IN_DONT_FOLLOW
        | (follow ? 0 : IN_DONT_FOLLOW)  // Optionally don't follow symlinks
#endif
#ifdef IN_EXCL_UNLINK
        | IN_EXCL_UNLINK   // Watch should be removed when the file is unlinked
#endif
        ;

    // Add the path to the inotify watch list
    int wd;
    if ((wd = inotify_add_watch(m_fd, path.c_str(), mask)) < 0) {
        saved_errno = errno;
        LOGSYSERR("RclIntf::addWatch", "inotify_add_watch", path);
        if (errno == ENOSPC) {
            LOGERR("RclIntf::addWatch: ENOSPC error may mean that you should "
                   "increase the inotify kernel constants. See inotify(7)\n");
        }
        return false;
    }

    // Map the watch descriptor (wd) to the path
    m_idtopath[wd] = path;
    return true;
}

// Retrieves and processes an event from the inotify queue
// Note: return false only for queue empty or error 
// Return EVT_NONE for bad event to keep queue processing going
bool RclIntf::getEvent(RclMonEvent& ev, int msecs)
{
    if (!ok())
        return false;

    ev.m_etyp = RclMonEvent::RCLEVT_NONE;
    MONDEB("RclIntf::getEvent:\n");

    if (nullptr == m_evp) {
        // If no event is currently being processed, wait for an event
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(m_fd, &readfds);
        struct timeval timeout;
        if (msecs >= 0) {
            timeout.tv_sec = msecs / 1000;
            timeout.tv_usec = (msecs % 1000) * 1000;
        }

        int ret;
        MONDEB("RclIntf::getEvent: select\n");

        // Use select to wait for an event to be available on the inotify file descriptor
        if ((ret = select(m_fd + 1, &readfds, nullptr, nullptr,
                          msecs >= 0 ? &timeout : nullptr)) < 0) {
            LOGSYSERR("RclIntf::getEvent", "select", "");
            close();
            return false;
        } else if (ret == 0) {
            MONDEB("RclIntf::getEvent: select timeout\n");
            // No event received within the timeout
            return false;
        }
        MONDEB("RclIntf::getEvent: select returned\n");

        if (!FD_ISSET(m_fd, &readfds))
            return false;
        
        // Read the event from the inotify file descriptor
        int rret;
        if ((rret=read(m_fd, m_evbuf, sizeof(m_evbuf))) <= 0) {
            LOGSYSERR("RclIntf::getEvent", "read", sizeof(m_evbuf));
            close();
            return false;
        }

        // Set up pointers for processing the event
        m_evp = m_evbuf;
        m_ep = m_evbuf + rret;
    }

    // Cast the buffer to an inotify_event structure
    struct inotify_event *evp = (struct inotify_event *)m_evp;
    m_evp += sizeof(struct inotify_event);
    
    // If the event has additional data, move the pointer forward
    if (evp->len > 0)
        m_evp += evp->len;
    
    // If all events have been processed, reset the pointers
    if (m_evp >= m_ep)
        m_evp = m_ep = nullptr;
    
    // Find the path corresponding to the watch descriptor
    map<int,string>::const_iterator it;
    if ((it = m_idtopath.find(evp->wd)) == m_idtopath.end()) {
        LOGERR("RclIntf::getEvent: unknown wd " << evp->wd << "\n");
        return true;
    }
    ev.m_path = it->second;

    // If the event includes a file name, append it to the path
    if (evp->len > 0) {
        ev.m_path = path_cat(ev.m_path, evp->name);
    }

    // Handle the event type and set the appropriate event type for the monitor
    MONDEB("RclIntf::getEvent: " << event_name(evp->mask) << " " << ev.m_path << "\n");

    if ((evp->mask & IN_MOVED_FROM) && (evp->mask & IN_ISDIR)) {
        // Directory was moved; remove the subtree entries in the map
        // 
        // We get this when a directory is renamed. Erase the subtree
        // entries in the map. The subsequent MOVED_TO will recreate
        // them. This is probably not needed because the watches
        // actually still exist in the kernel, so that the wds
        // returned by future addwatches will be the old ones, and the
        // map will be updated in place. But still, this feels safer
        eraseWatchSubTree(m_idtopath, ev.m_path);
    }

    // Set the event type for the monitor based on the inotify event
    // Note that IN_ATTRIB used to be not needed, but now it is
    if (evp->mask & (IN_MODIFY|IN_ATTRIB)) {
        ev.m_etyp = RclMonEvent::RCLEVT_MODIFY;
    } else if (evp->mask & (IN_DELETE | IN_MOVED_FROM)) {
        ev.m_etyp = RclMonEvent::RCLEVT_DELETE;
        if (evp->mask & IN_ISDIR)
            ev.m_etyp |= RclMonEvent::RCLEVT_ISDIR;
    } else if (evp->mask & (IN_CREATE | IN_MOVED_TO)) {
        if (evp->mask & IN_ISDIR) {
            ev.m_etyp = RclMonEvent::RCLEVT_DIRCREATE;
        } else {
            // Treat file creation or move as a modify event
            // We used to return null event because we would get a
            // modify event later, but it seems not to be the case any
            // more (10-2011). So generate MODIFY event
            ev.m_etyp = RclMonEvent::RCLEVT_MODIFY;
        }
    } else if (evp->mask & (IN_IGNORED)) {
        // The watch was removed; handle cleanup
        if (!m_idtopath.erase(evp->wd)) {
            LOGDEB0("Got IGNORE event for unknown watch\n");
        } else {
            eraseWatchSubTree(m_idtopath, ev.m_path);
        }
    } else {
        // Unhandled event type
        LOGDEB("RclIntf::getEvent: unhandled event " << event_name(evp->mask) <<
               " " << evp->mask << " " << ev.m_path << "\n");
        return true;
    }
    return true;
}

#endif // FSWATCH_INOTIFY