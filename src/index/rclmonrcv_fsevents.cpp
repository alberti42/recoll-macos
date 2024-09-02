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
#include "log.h"
#include "rclmonrcv.h"

#ifdef FSWATCH_FSEVENTS

#include "rclmonrcv_fsevents.h"

#ifdef MANAGE_SEPARATE_QUEUE

CFRunLoopSourceRef runLoopSource;
RunLoopSourceContext contextLoop = {.shouldExit = false, .monitor = NULL};

void stopMonitoring() {
    if (runLoopSource) {
        CFRunLoopSourceInvalidate(runLoopSource);
        CFRelease(runLoopSource);
        runLoopSource = nullptr;
    }

    // Signal to exit gracefully (optional: can be done within the cancel routine)
    if (contextLoop.monitor->m_runLoop) {
        contextLoop.shouldExit = true;
        CFRunLoopWakeUp(contextLoop.monitor->m_runLoop); // Wake up the run loop if needed to process the exit
    }
}

// Function to process individual events
void processEvent(const RclMonEvent& event) {
    switch (event.m_etyp) {
        case RclMonEvent::RCLEVT_MODIFY:
            LOGINFO("processEvent: Processing MODIFY event for: " + event.m_path + '\n');
            // Handle modify logic here
            break;
        case RclMonEvent::RCLEVT_DELETE:
            LOGINFO("processEvent: Processing DELETE event for: " + event.m_path + '\n');
            // Handle delete logic here
            break;
        case RclMonEvent::RCLEVT_DIRCREATE:
            LOGINFO("processEvent: Processing DIRECTORY CREATE event for: " + event.m_path + '\n');
            // Handle directory create logic here
            break;
        case RclMonEvent::RCLEVT_ISDIR:
            LOGINFO("processEvent: Processing DIRECTORY event for: " + event.m_path + '\n');
            // Handle directory-specific logic here
            break;
        default:
            LOGINFO("processEvent: Unknown event type for: " + event.m_path + '\n');
            break;
    }
}

// Function to cancel the run loop source
void RunLoopSourceCancelRoutine(void *info, CFRunLoopRef rl, CFStringRef mode) {
    RunLoopSourceContext *context = static_cast<RunLoopSourceContext *>(info);
    LOGINFO("RunLoopSourceCancelRoutine: RunLoopSourceCancelRoutine triggered.\n");
    context->shouldExit = true;  // Set the exit flag
}

void RunLoopSourcePerformRoutine(void *info) {
    RunLoopSourceContext *context = static_cast<RunLoopSourceContext *>(info);
    RclFSEvents *monitor = context->monitor;
    RclMonEvent event;

    // Process all available events in the queue
    while (monitor->getEvent(event)) {
        LOGINFO("RunLoopSourcePerformRoutine:  Processing event in RunLoopSourcePerformRoutine.\n");
        processEvent(event);
    }

    if (context->shouldExit) {
        LOGINFO("RunLoopSourcePerformRoutine: Gracefully exiting run loop based on shouldExit flag.\n");
        CFRunLoopStop(CFRunLoopGetCurrent());  // Stop the run loop
    }

    LOGINFO("RunLoopSourcePerformRoutine: Exiting RunLoopSourcePerformRoutine.\n");
}

// Function to create the custom run loop source
CFRunLoopSourceRef CreateEventQueueRunLoopSource(RclFSEvents *monitor) {
    contextLoop.monitor = monitor;
    
    CFRunLoopSourceContext sourceContext = {
        0,                           // Version (unused)
        &contextLoop,                    // Info pointer (our custom context)
        NULL,                        // Retain (unused)
        NULL,                        // Release (unused)
        NULL,                        // CopyDescription (unused)
        NULL,                        // Equal (unused)
        NULL,                        // Hash (unused)
        NULL,                        // Schedule (unused)
        RunLoopSourceCancelRoutine,  // Cancel
        RunLoopSourcePerformRoutine  // Perform routine
    };

    return CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &sourceContext);
}
#else
// Initialize the context with the exit condition
DummyTimerContext contextLoop = {.shouldExit = false};

// Timer callback function
void DummyTimerCallback(CFRunLoopTimerRef timer, void *info) {
    DummyTimerContext *context = static_cast<DummyTimerContext *>(info);

    // Check the exit condition
    if (context->shouldExit) {
        CFRunLoopStop(CFRunLoopGetCurrent());  // Stop the run loop
    } else {
        // Run loop is still running...
    }
}
#endif // MANAGE_SEPARATE_QUEUE

// Signal handler for SIGINT
void RclFSEvents::signalHandler(int signum) {
    if(signum == SIGINT) {
        LOGINFO("RclFSEvents::signalHandler: Interrupt signal (" << signum << ") received." << std::endl);
#ifdef MANAGE_SEPARATE_QUEUE
        stopMonitoring();
#else
        contextLoop.shouldExit = true; // Set the exit flag to true
#endif
    }
}

void RclFSEvents::startMonitoring(
        RclMonEventQueue *queue,
        RclConfig& lconfig,
        FsTreeWalker& walker) {

    this->m_queue = queue;
    this->m_lconfigPtr = &lconfig;
    this->m_walkerPtr = &walker;

    setupAndStartStream();  // Initial setup and start

    // Each thread in macOS has its own run loop, and CFRunLoopGetCurrent()
    // returns the run loop for the calling thread. We need to store a reference
    // to be used in the callback function, which runs in a separate thread.
    m_runLoop = CFRunLoopGetCurrent();

#ifdef MANAGE_SEPARATE_QUEUE
    // Create and add the custom run loop source
    runLoopSource = CreateEventQueueRunLoopSource(this);
    
    CFRunLoopAddSource(m_runLoop, runLoopSource, kCFRunLoopDefaultMode);

#else
    // Define the CFRunLoopTimerContext
    CFRunLoopTimerContext timerContext = {0, &contextLoop, NULL, NULL, NULL};

    // Add a dummy timer to keep the run loop active
    CFRunLoopTimerRef dummyTimer = CFRunLoopTimerCreate(
        kCFAllocatorDefault,  // 1. Allocator
        CFAbsoluteTimeGetCurrent() + 1.0, // 2. First fire time
        1.0, // 3. Interval
        0, // 4. Flags
        0, // 5. Order
        DummyTimerCallback, // 6. Callback function
        &timerContext // 7. Context
    );

    CFRunLoopAddTimer(m_runLoop, dummyTimer, kCFRunLoopDefaultMode);
#endif // MANAGE_SEPARATE_QUEUE

    // Register signal handler for SIGINT (CTRL-C)
    signal(SIGINT, signalHandler);

    CFRunLoopRun(); // Start the run loop

#ifdef MANAGE_SEPARATE_QUEUE
    stopMonitoring();    
#endif // MANAGE_SEPARATE_QUEUE

}

RclFSEvents::RclFSEvents() : m_lconfigPtr(nullptr), m_walkerPtr(nullptr), m_ok(true) {
    // Initialize FSEvents stream
}

RclFSEvents::~RclFSEvents() {
    removeFSEventStream();
#ifdef MANAGE_SEPARATE_QUEUE
    stopMonitoring();    
#endif // MANAGE_SEPARATE_QUEUE
}
void RclFSEvents::fsevents_callback(
    ConstFSEventStreamRef streamRef,
    void *clientCallBackInfo,
    size_t numEvents,
    void *eventPaths,  // now a CFArrayRef of CFDictionaryRef
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId eventIds[]) {

    RclFSEvents *self = static_cast<RclFSEvents *>(clientCallBackInfo);

    CFArrayRef pathsArray = (CFArrayRef)eventPaths;

    // std::cout << "Number of events: " << numEvents << std::endl;

    for (size_t i = 0; i < numEvents; ++i) {
        // Get the dictionary for this event
        CFDictionaryRef eventDict = (CFDictionaryRef)CFArrayGetValueAtIndex(pathsArray, i);

        // Extract the path from the dictionary
        CFStringRef pathRef = (CFStringRef)CFDictionaryGetValue(eventDict, kFSEventStreamEventExtendedDataPathKey);
        char path[PATH_MAX];
        CFStringGetFileSystemRepresentation(pathRef, path, sizeof(path));

        /*
        std::cout << "Path         : " << path << std::endl;
        std::cout << "Event ID     : " << eventIds[i] << std::endl;
        // Extract the inode from the dictionary
        CFNumberRef inodeRef = (CFNumberRef)CFDictionaryGetValue(eventDict, kFSEventStreamEventExtendedFileIDKey);
        ino_t inode;
        if (inodeRef) {
            CFNumberGetValue(inodeRef, kCFNumberLongLongType, &inode);
            std::cout << "Inode        : " << inode << std::endl;
        } else {
            std::cout << "Inode        : not available" << std::endl;
        }
        std::cout << "Event flags  : " << eventFlags[i] << std::endl;
        */

        if (rclMonShouldSkip(path, *self->m_lconfigPtr, *self->m_walkerPtr))
            continue;

        RclMonEvent event;
        event.m_path = path;
        event.m_etyp = RclMonEvent::RCLEVT_NONE;

        bool isDir = eventFlags[i] & kFSEventStreamEventFlagItemIsDir;

        // Handle file creation
        if (eventFlags[i] & kFSEventStreamEventFlagItemCreated) {
            if (isDir) {
                event.m_etyp = RclMonEvent::RCLEVT_DIRCREATE;
                LOGINFO("RclFSEvents::fsevents_callback: Event type: DIRECTORY CREATED:  " << path << std::endl);
            } else {
                event.m_etyp = RclMonEvent::RCLEVT_MODIFY;
                LOGINFO("RclFSEvents::fsevents_callback: Event type: FILE CREATED:       " << path << std::endl);
            }
        }
        // Handle file removal
        else if (eventFlags[i] & kFSEventStreamEventFlagItemRemoved) {
            event.m_etyp = RclMonEvent::RCLEVT_DELETE;
            if (isDir) {
                event.m_etyp |= RclMonEvent::RCLEVT_ISDIR;
                LOGINFO("RclFSEvents::fsevents_callback: Event type: DIRECTORY REMOVED:  " << path << std::endl);
            } else {
                LOGINFO("RclFSEvents::fsevents_callback: Event type: FILE REMOVED:       " << path << std::endl);
            }
        }
        // Handle inode metadata modification
        else if (eventFlags[i] & kFSEventStreamEventFlagItemInodeMetaMod) {
            event.m_etyp = RclMonEvent::RCLEVT_MODIFY;
            LOGINFO("RclFSEvents::fsevents_callback: Event type: METADATA MODIFIED:  " << path << std::endl);
        }
        // Handle content modification
        else if (eventFlags[i] & kFSEventStreamEventFlagItemModified) {
            event.m_etyp = RclMonEvent::RCLEVT_MODIFY;
            LOGINFO("RclFSEvents::fsevents_callback: Event type: FILE MODIFIED:      " << path << std::endl);
        }
        // Handle file renaming
        else if (eventFlags[i] & kFSEventStreamEventFlagItemRenamed) {
            struct stat buffer;
            if (stat(path, &buffer) != 0) {
                // File does not exist, indicating it was renamed or moved
                event.m_etyp = RclMonEvent::RCLEVT_DELETE; // Handle renames as modify    
                if(isDir) {
                    event.m_etyp |= RclMonEvent::RCLEVT_ISDIR;
                    LOGINFO("RclFSEvents::fsevents_callback: Event type: DIRECTORY MOVED FROM:  " << path << std::endl);
                } else {
                    LOGINFO("RclFSEvents::fsevents_callback: Event type: FILE MOVED FROM:       " << path << std::endl);
                }
            } else {
                if (isDir) {
                    event.m_etyp = RclMonEvent::RCLEVT_DIRCREATE;
                    LOGINFO("RclFSEvents::fsevents_callback: Event type: DIRECTORY MOVE TO:     " << path << std::endl);
                } else {
                    event.m_etyp = RclMonEvent::RCLEVT_MODIFY;
                    LOGINFO("RclFSEvents::fsevents_callback: Event type: FILE MOVE TO:          " << path << std::endl);
                }
            }
        }

        if (event.m_etyp != RclMonEvent::RCLEVT_NONE) {
#ifdef MANAGE_SEPARATE_QUEUE
            std::unique_lock<std::mutex> lockInstance(self->m_queueMutex); // Mutex is locked here
            self->m_eventQueue.push_back(event); // Store the event

            CFRunLoopSourceSignal(runLoopSource); // Signal the stored run loop source
            CFRunLoopWakeUp(self->m_runLoop); // Wake up the stored run loop

            // Manually unlock the mutex before signaling the run loop
            lockInstance.unlock();  // Unlock the mutex
#else

            if(event.m_etyp == RclMonEvent::RCLEVT_DIRCREATE) {
                // Recursive addwatch: there may already be stuff inside this directory. E.g.: files
                // were quickly created, or this is actually the target of a directory move. This is
                // necessary for inotify, but it seems that fam/gamin is doing the job for us so
                // that we are generating double events here (no big deal as prc will sort/merge).
                LOGINFO("RclFSEvents::fsevents_callback: Event type: WALKING NEW DIRECTORY: " << event.m_path << "\n");
                if (!rclMonAddSubWatches(event.m_path, *self->m_walkerPtr, *self->m_lconfigPtr, self, self->m_queue)) {
                    LOGERR("RclFSEvents::fsevents_callback: error in walking dir" << std::endl);
#ifdef MANAGE_SEPARATE_QUEUE
                            stopMonitoring();
#else
                            contextLoop.shouldExit = true; // Set the exit flag to true
#endif
                    return;
                }
            }

            // We push the event on the queue; pushEvent handles the operation in a thread-safe manner mutex
            self->m_queue->pushEvent(event);
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
                                   kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagUseExtendedData | kFSEventStreamCreateFlagUseCFTypes);

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
