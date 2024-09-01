// rclmonrcv_fsevents.cpp

/* Andrea Alberti, 2024 */


/* Description of events listed at https://developer.apple.com/documentation/coreservices/1455361-fseventstreameventflags

1. **`kFSEventStreamEventFlagNone`**:
   - Indicates that there is no special event or condition associated with this event. It represents a standard file system change event.

2. **`kFSEventStreamEventFlagMustScanSubDirs`**:
   - This flag indicates that the directory contents changed, and the system cannot determine which files or subdirectories changed. As a result, the entire directory and its contents must be scanned.

3. **`kFSEventStreamEventFlagUserDropped`**:
   - This flag indicates that the event queue overflowed, and the user-space process dropped events. You should scan the directory again to determine what has changed.

4. **`kFSEventStreamEventFlagKernelDropped`**:
   - This flag indicates that the kernel event queue overflowed, and the kernel dropped events. Similar to `kFSEventStreamEventFlagUserDropped`, you should scan the directory to determine changes.

5. **`kFSEventStreamEventFlagEventIdsWrapped`**:
   - Indicates that the event ID counter has wrapped around. This typically occurs if the event queue has been running for a very long time or if there is an issue with the event ID generation.

6. **`kFSEventStreamEventFlagHistoryDone`**:
   - Indicates that the historical event replay is complete. This flag is sent when you request historical events (e.g., from a specific time) and the system has finished sending all past events.

7. **`kFSEventStreamEventFlagRootChanged`**:
   - Indicates that the monitored volume or directory’s root path has changed, such as due to a volume being renamed or unmounted and remounted.

8. **`kFSEventStreamEventFlagMount`**:
   - Indicates that a volume has been mounted on the system. This event is useful for detecting when a new volume becomes available.

9. **`kFSEventStreamEventFlagUnmount`**:
   - Indicates that a volume has been unmounted from the system. It is the counterpart to `kFSEventStreamEventFlagMount`.

10. **`kFSEventStreamEventFlagItemChangeOwner`**:
    - Indicates that the ownership of an item has changed. This could happen if the file or directory’s owner is modified.

11. **`kFSEventStreamEventFlagItemCreated`**:
    - Indicates that an item (file or directory) was created.

12. **`kFSEventStreamEventFlagItemFinderInfoMod`**:
    - Indicates that the Finder information (metadata such as labels) of an item has been modified.

13. **`kFSEventStreamEventFlagItemInodeMetaMod`**:
    - Indicates that the inode metadata of an item has been modified. This could include changes to permissions, timestamps, or other inode-level information. This flag is triggered when there are changes to the inode metadata of a file or directory. Inode metadata includes attributes such as file permissions, ownership, timestamps (e.g., creation, modification, and access times), and link counts. It does not indicate changes to the actual content of the file. Instead, it focuses on changes at the file system level, such as when you change the permissions (chmod), ownership (chown), or other metadata of a file or directory.

14. **`kFSEventStreamEventFlagItemIsDir`**:
    - Indicates that the event is associated with a directory.

15. **`kFSEventStreamEventFlagItemIsFile`**:
    - Indicates that the event is associated with a file.

16. **`kFSEventStreamEventFlagItemIsHardlink`**:
    - Indicates that the item is a hard link.

17. **`kFSEventStreamEventFlagItemIsLastHardlink`**:
    - Indicates that this event is associated with the last hard link to a file being removed.

18. **`kFSEventStreamEventFlagItemIsSymlink`**:
    - Indicates that the item is a symbolic link.

19. **`kFSEventStreamEventFlagItemModified`**:
    - Indicates that an item has been modified. This includes content changes in a file. This flag is triggered when the actual content of the file is modified. It indicates that the data within the file has changed, which typically happens when the file is opened for writing and then closed. It focuses on the contents of the file rather than its metadata.

20. **`kFSEventStreamEventFlagItemRemoved`**:
    - Indicates that an item (file or directory) was removed.

21. **`kFSEventStreamEventFlagItemRenamed`**:
    - Indicates that an item was renamed.

22. **`kFSEventStreamEventFlagItemXattrMod`**:
    - Indicates that the extended attributes of an item were modified.

23. **`kFSEventStreamEventFlagOwnEvent`**:
    - Indicates that the event was triggered by the current process. This is useful for ignoring changes your own application made.

24. **`kFSEventStreamEventFlagItemCloned`**:
    - Indicates that the item was cloned, often associated with features like APFS's copy-on-write cloning.

*/

#include "autoconfig.h"
#include "rclmonrcv.h"

#ifdef FSWATCH_FSEVENTS

#include "rclmonrcv_fsevents.h"

// Custom context for the run loop source
typedef struct {
    RclFSEvents *monitor;
} RunLoopSourceContext;

#ifdef MANAGE_SEPARATE_QUEUE
// Function to process individual events
void processEvent(const RclMonEvent& event) {
    switch (event.m_etyp) {
        case RclMonEvent::RCLEVT_MODIFY:
            std::cout << "Processing MODIFY event for: " << event.m_path << std::endl;
            // Handle modify logic here
            break;
        case RclMonEvent::RCLEVT_DELETE:
            std::cout << "Processing DELETE event for: " << event.m_path << std::endl;
            // Handle delete logic here
            break;
        case RclMonEvent::RCLEVT_DIRCREATE:
            std::cout << "Processing DIRECTORY CREATE event for: " << event.m_path << std::endl;
            // Handle directory create logic here
            break;
        case RclMonEvent::RCLEVT_ISDIR:
            std::cout << "Processing DIRECTORY event for: " << event.m_path << std::endl;
            // Handle directory-specific logic here
            break;
        default:
            std::cout << "Unknown event type for: " << event.m_path << std::endl;
            break;
    }
}

void RunLoopSourcePerformRoutine(void *info) {
    // std::cout << "RunLoopSourcePerformRoutine triggered." << std::endl; // std::endl forces a flush

    RunLoopSourceContext *context = static_cast<RunLoopSourceContext *>(info);
    RclFSEvents *monitor = context->monitor;
    RclMonEvent event;

    // Process all available events in the queue
    while (monitor->getEvent(event)) {
        // std::cout << "Processing event in RunLoopSourcePerformRoutine." << std::endl; 
        processEvent(event);
    }

    std::cout << "Exiting RunLoopSourcePerformRoutine." << std::endl;
}

// Function to create the custom run loop source
CFRunLoopSourceRef CreateEventQueueRunLoopSource(RclFSEvents *monitor) {
    RunLoopSourceContext *context = new RunLoopSourceContext();
    context->monitor = monitor;

    CFRunLoopSourceContext sourceContext = {
        0,                           // Version (unused)
        context,                     // Info pointer (our custom context)
        NULL,                        // Retain (unused)
        NULL,                        // Release (unused)
        NULL,                        // CopyDescription (unused)
        NULL,                        // Equal (unused)
        NULL,                        // Hash (unused)
        NULL,                        // Schedule (unused)
        NULL,                        // Cancel (unused)
        RunLoopSourcePerformRoutine  // Perform routine
    };

    return CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &sourceContext);
}
#endif // MANAGE_SEPARATE_QUEUE

void RclFSEvents::startMonitoring(
        RclMonEventQueue *queue,
        RclConfig& lconfig,
        FsTreeWalker& walker) {

    this->queue = queue;
    this->lconfigPtr = &lconfig;
    this->walkerPtr = &walker;

    setupAndStartStream();  // Initial setup and start

    // Each thread in macOS has its own run loop, and CFRunLoopGetCurrent()
    // returns the run loop for the calling thread. We need to store a reference
    // to be used in the callback function, which runs in a separate thread.
    m_runLoop = CFRunLoopGetCurrent();

#ifdef MANAGE_SEPARATE_QUEUE
    // Create and add the custom run loop source
    m_runLoopSource = CreateEventQueueRunLoopSource(this);
    
    // std::cout << "Run Loop Reference at startMonitoring: " << runLoop << std::endl;

    CFRunLoopAddSource(m_runLoop, m_runLoopSource, kCFRunLoopDefaultMode);
#endif // MANAGE_SEPARATE_QUEUE

    std::cout << "Starting CFRunLoop..." << std::endl;
    CFRunLoopRun(); // Start the run loop

    // Clean up
    if (m_runLoopSource) {
        CFRunLoopSourceInvalidate(m_runLoopSource);
        CFRelease(m_runLoopSource);
        m_runLoopSource = nullptr;
    }
    std::cout << "CFRunLoop has exited." << std::endl;
}

RclFSEvents::RclFSEvents() : lconfigPtr(nullptr), walkerPtr(nullptr), m_ok(true) {
    // Initialize FSEvents stream
}

RclFSEvents::~RclFSEvents() {
    removeFSEventStream();
#ifdef MANAGE_SEPARATE_QUEUE
    removeSeparateQueueLoop();    
#endif // MANAGE_SEPARATE_QUEUE
}

void RclFSEvents::fsevents_callback(
    ConstFSEventStreamRef streamRef,
    void *clientCallBackInfo,
    size_t numEvents,
    void *eventPaths,
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId eventIds[]) {
    
    RclFSEvents *self = static_cast<RclFSEvents *>(clientCallBackInfo);
    
    char **paths = (char **)eventPaths;
    
    for (size_t i = 0; i < numEvents; ++i) {
        string path = paths[i];
        // std::cout << "Path of modified file/directory: " << path << std::endl;

        if (rclMonShouldSkip(path, *self->lconfigPtr, *self->walkerPtr))
            continue;

        RclMonEvent event;
        event.m_path = path;
        event.m_etyp = RclMonEvent::RCLEVT_NONE;

        bool isDir = eventFlags[i] & kFSEventStreamEventFlagItemIsDir;

        // Handle file creation
        if (eventFlags[i] & kFSEventStreamEventFlagItemCreated) {
            if (isDir) {
                event.m_etyp |= RclMonEvent::RCLEVT_DIRCREATE;
                std::cout << "    - Event type: DIRECTORY CREATED" << std::endl;
            } else {
                event.m_etyp |= RclMonEvent::RCLEVT_MODIFY;
                std::cout << "    - Event type: FILE CREATED" << std::endl;
            }
        }

        // Handle file removal
        if (eventFlags[i] & kFSEventStreamEventFlagItemRemoved) {
            event.m_etyp |= RclMonEvent::RCLEVT_DELETE;
            if (isDir) {
                event.m_etyp |= RclMonEvent::RCLEVT_ISDIR;
                std::cout << "    - Event type: DIRECTORY REMOVED" << std::endl;
            } else {
                std::cout << "    - Event type: FILE REMOVED" << std::endl;
            }
        }

        // Handle inode metadata modification
        if (eventFlags[i] & kFSEventStreamEventFlagItemInodeMetaMod) {
            event.m_etyp |= RclMonEvent::RCLEVT_MODIFY;
            std::cout << "    - Event type: METADATA MODIFIED" << std::endl;
        }

        // Handle content modification
        if (eventFlags[i] & kFSEventStreamEventFlagItemModified) {
            event.m_etyp |= RclMonEvent::RCLEVT_MODIFY;
            std::cout << "    - Event type: FILE MODIFIED" << std::endl;
        }

        // Handle file renaming
        if (eventFlags[i] & kFSEventStreamEventFlagItemRenamed) {
            event.m_etyp |= RclMonEvent::RCLEVT_MODIFY; // Handle renames as modify
            std::cout << "    - Event type: FILE RENAMED" << std::endl;
        }

        if (event.m_etyp != RclMonEvent::RCLEVT_NONE) {
            
#ifdef MANAGE_SEPARATE_QUEUE
            std::unique_lock<std::mutex> lockInstance(self->m_queueMutex); // Mutex is locked here
            self->m_eventQueue.push_back(event); // Store the event
            // Manually unlock the mutex before signaling the run loop
            lockInstance.unlock();  // Unlock the mutex
            
            std::cout << "Signaling the run loop source..." << std::endl;
            CFRunLoopSourceSignal(self->m_runLoopSource); // Signal the stored run loop source
            std::cout << "Waking up the run loop..." << std::endl;
            CFRunLoopWakeUp(self->m_runLoop); // Wake up the stored run loop
            std::cout << "Run loop signaled and woken up." << std::endl;
#endif // MANAGE_SEPARATE_QUEUE
        }
    }
}

#ifdef MANAGE_SEPARATE_QUEUE
bool RclFSEvents::getEvent(RclMonEvent& ev, int msecs) {
    std::unique_lock<std::mutex> lockInstance(m_queueMutex); // Mutex is locked here
    if (m_eventQueue.empty()) {
        return false; // No events available
    }

    ev = m_eventQueue.front();
    m_eventQueue.erase(m_eventQueue.begin());    
    lockInstance.unlock();  // Unlock the mutex
    return true;
}

void RclFSEvents::removeSeparateQueueLoop() {
    if (m_runLoopSource) {
        CFRunLoopSourceInvalidate(m_runLoopSource);
        CFRelease(m_runLoopSource);
        m_runLoopSource = nullptr;
    }
}
#endif // MANAGE_SEPARATE_QUEUE

bool RclFSEvents::ok() const {
    return m_ok;
}

void RclFSEvents::removeFSEventStream() {
    if (m_stream) {
        // Stop and release the existing stream if it's already running
        FSEventStreamStop(m_stream);
        FSEventStreamInvalidate(m_stream);
        FSEventStreamRelease(m_stream);
        m_stream = nullptr;
    }
}

void removeSeparateQueueLoop() {

}

void RclFSEvents::setupAndStartStream() {
    removeFSEventStream();

    // Ensure there are paths to monitor
    if (m_pathsToWatch.empty()) {
        LOGSYSERR("RclFSEvents::setupAndStartStream", "m_pathsToWatch.empty()", "No paths to watch!")
        return;
    }

    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)m_pathsToWatch.data(), m_pathsToWatch.size(), &kCFTypeArrayCallBacks);

    FSEventStreamContext context = { 0, this, NULL, NULL, NULL };
    m_stream = FSEventStreamCreate(NULL,
                                   &fsevents_callback,
                                   &context,
                                   pathsToWatch,
                                   kFSEventStreamEventIdSinceNow,
                                   1.0,
                                   kFSEventStreamCreateFlagFileEvents);

    CFRelease(pathsToWatch);

    if (!m_stream) {
        LOGSYSERR("RclFSEvents::setupAndStartStream", "FSEventStreamCreate", "Failed to create FSEventStream");
        return;
    }

    dispatch_queue_t event_queue = dispatch_queue_create("org.recoll.fswatch", NULL);
    FSEventStreamSetDispatchQueue(m_stream, event_queue);
    FSEventStreamStart(m_stream);
}

bool RclFSEvents::addWatch(const string& path, bool /*isDir*/, bool /*follow*/) {
    CFStringRef cfPath = CFStringCreateWithCString(NULL, path.c_str(), kCFStringEncodingUTF8);
    if (cfPath) {
        m_pathsToWatch.push_back(cfPath);
        setupAndStartStream();  // Restart stream with updated paths
        return true;
    } else {
        std::cerr << "Failed to convert path to CFStringRef: " << path << std::endl;
        return false;
    }
}

void RclFSEvents::removeWatch(const string &path) {
    CFStringRef cfPath = CFStringCreateWithCString(NULL, path.c_str(), kCFStringEncodingUTF8);
    if (!cfPath) {
        std::cerr << "Failed to convert path to CFStringRef: " << path << std::endl;
        return;
    }

    auto it = std::remove_if(m_pathsToWatch.begin(), m_pathsToWatch.end(),
                             [cfPath](CFStringRef existingPath) {
                                 bool match = CFStringCompare(existingPath, cfPath, 0) == kCFCompareEqualTo;
                                 if (match) {
                                     CFRelease(existingPath);  // Release the CFStringRef if removing
                                 }
                                 return match;
                             });
    
    m_pathsToWatch.erase(it, m_pathsToWatch.end());
    CFRelease(cfPath);

    setupAndStartStream();  // Restart stream with updated paths
}


#endif // FSWATCH_FSEVENTS
