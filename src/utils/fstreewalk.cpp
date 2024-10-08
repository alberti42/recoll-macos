/* Copyright (C) 2004-2024 J.F.Dockes 
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "autoconfig.h"

#include <stdio.h>

#include <errno.h>
#include <fnmatch.h>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <vector>
#include <deque>
#include <set>

#include "cstr.h"
#include "log.h"
#include "pathut.h"
#include "fstreewalk.h"

#ifdef _WIN32
// We do need this for detecting stuff about network shares
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include <windows.h>
#endif // _WIN32

using namespace std;

bool FsTreeWalker::o_useFnmPathname = true;
string FsTreeWalker::o_nowalkfn;

const int FsTreeWalker::FtwTravMask = FtwTravNatural|
    FtwTravBreadth|FtwTravFilesThenDirs|FtwTravBreadthThenDepth;

#ifndef _WIN32
// dev/ino means nothing on Windows. It seems that FileId could replace it
// but we only use this for cycle detection which we just disable.
#include <sys/stat.h>
class DirId {
public:
    dev_t dev;
    ino_t ino;
    DirId(dev_t d, ino_t i) : dev(d), ino(i) {}
    bool operator<(const DirId& r) const {
        return dev < r.dev || (dev == r.dev && ino < r.ino);
    }
};
#endif

class FsTreeWalker::Internal {
public:
    Internal(int opts)
        : options(opts), depthswitch(4), maxdepth(-1), errors(0)
        {}
    int options;
    int depthswitch;
    int maxdepth;
    int basedepth;
    stringstream reason;
    vector<string> skippedNames;
    vector<string> onlyNames;
    vector<string> skippedPaths;
    // When doing Breadth or FilesThenDirs traversal, we keep a list
    // of directory paths to be processed, and we do not recurse.
    deque<string> dirs;
    int errors;
#ifndef _WIN32
    set<DirId> donedirs;
#endif
    void logsyserr(const char *call, const string &param) {
        errors++;
        reason << call << "(" << param << ") : " << errno << " : " << strerror(errno) << "\n";
    }
};

FsTreeWalker::FsTreeWalker(int opts)
{
    data = new Internal(opts);
}

FsTreeWalker::~FsTreeWalker()
{
    delete data;
}

void FsTreeWalker::setOpts(int opts)
{
    if (data) {
        data->options = opts;
    }
}
int FsTreeWalker::getOpts()
{
    if (data) {
        return data->options;
    } else {
        return 0;
    }
}
void FsTreeWalker::setDepthSwitch(int ds)
{
    if (data) {
        data->depthswitch = ds;
    }
}
void FsTreeWalker::setMaxDepth(int md)
{
    if (data) {
        data->maxdepth = md;
    }
}

string FsTreeWalker::getReason()
{
    string reason = data->reason.str();
    data->reason.str(string());
    data->errors = 0;
    return reason;
}

int FsTreeWalker::getErrCnt()
{
    return data->errors;
}

bool FsTreeWalker::addSkippedName(const string& pattern)
{
    if (find(data->skippedNames.begin(), 
             data->skippedNames.end(), pattern) == data->skippedNames.end())
        data->skippedNames.push_back(pattern);
    return true;
}
bool FsTreeWalker::setSkippedNames(const vector<string> &patterns)
{
    data->skippedNames = patterns;
    return true;
}
bool FsTreeWalker::inSkippedNames(const string& name)
{
    for (const auto& pattern : data->skippedNames) {
        if (fnmatch(pattern.c_str(), name.c_str(), 0) == 0) {
            return true;
        }
    }
    return false;
}
bool FsTreeWalker::setOnlyNames(const vector<string> &patterns)
{
    data->onlyNames = patterns;
    return true;
}
bool FsTreeWalker::inOnlyNames(const string& name)
{
    if (data->onlyNames.empty()) {
        // Not set: all match
        return true;
    }
    for (const auto& pattern : data->onlyNames) {
        if (fnmatch(pattern.c_str(), name.c_str(), 0) == 0) {
            return true;
        }
    }
    return false;
}

bool FsTreeWalker::addSkippedPath(const string& ipath)
{
    string path = (data->options & FtwNoCanon) ? ipath : path_canon(ipath);
    if (find(data->skippedPaths.begin(), 
             data->skippedPaths.end(), path) == data->skippedPaths.end())
        data->skippedPaths.push_back(path);
    return true;
}
bool FsTreeWalker::setSkippedPaths(const vector<string> &paths)
{
    data->skippedPaths = paths;
    if (!(data->options & FtwNoCanon))
        for (auto& path : data->skippedPaths)
            path = path_canon(path);
    return true;
}
bool FsTreeWalker::inSkippedPaths(const string& path, bool ckparents)
{
    int fnmflags = o_useFnmPathname ? FNM_PATHNAME : 0;
#ifdef FNM_LEADING_DIR
    if (ckparents)
        fnmflags |= FNM_LEADING_DIR;
#endif

    for (const auto& skpath : data->skippedPaths) {
#ifndef FNM_LEADING_DIR
        if (ckparents) {
            string mpath = path;
            while (mpath.length() > 2) {
                if (fnmatch(skpath.c_str(), mpath.c_str(), fnmflags) == 0) 
                    return true;
                mpath = path_getfather(mpath);
            }
        } else 
#endif /* FNM_LEADING_DIR */
            if (fnmatch(skpath.c_str(), path.c_str(), fnmflags) == 0) {
                return true;
            }
    }
    return false;
}

static inline int slashcount(const string& p)
{
    return static_cast<int>(std::count(p.begin(), p.end(), '/'));
}

FsTreeWalker::Status FsTreeWalker::walk(const string& _top, FsTreeWalkerCB& cb)
{
    string top = (data->options & FtwNoCanon) ? _top : path_canon(_top);

    if ((data->options & FtwTravMask) == 0) {
        data->options |= FtwTravNatural;
    }

    data->basedepth = slashcount(top); // Only used for breadthxx
    struct PathStat st;
    // We always follow symlinks at this point. Makes more sense.
    if (path_fileprops(top, &st) == -1) {
        // Note that we do not return an error if the stat call
        // fails. A temp file may have gone away.
        data->logsyserr("stat", top);
        return errno == ENOENT ? FtwOk : FtwError;
    }

    // Recursive version, using the call stack to store state. iwalk
    // will process files and recursively descend into subdirs in
    // physical order of the current directory.
    if ((data->options & FtwTravMask) == FtwTravNatural) {
        return iwalk(top, st, cb);
    }

    // Breadth first of filesThenDirs semi-depth first order
    // Managing queues of directories to be visited later, in breadth or
    // depth order. Null marker are inserted in the queue to indicate
    // father directory changes (avoids computing parents all the time).
    data->dirs.push_back(top);
    Status status;
    while (!data->dirs.empty()) {
        string dir, nfather;
        if (data->options & (FtwTravBreadth|FtwTravBreadthThenDepth)) {
            // Breadth first, pop and process an older dir at the
            // front of the queue. This will add any child dirs at the
            // back
            dir = data->dirs.front();
            data->dirs.pop_front();
            if (dir.empty()) {
                // Father change marker. 
                if (data->dirs.empty())
                    break;
                dir = data->dirs.front();
                data->dirs.pop_front();
                nfather = path_getfather(dir);
                if (data->options & FtwTravBreadthThenDepth) {
                    // Check if new depth warrants switch to depth first
                    // traversal (will happen on next loop iteration).
                    int curdepth = slashcount(dir) - data->basedepth;
                    if (curdepth >= data->depthswitch) {
                        //fprintf(stderr, "SWITCHING TO DEPTH FIRST\n");
                        data->options &= ~FtwTravMask;
                        data->options |= FtwTravFilesThenDirs;
                    }
                }
            }
        } else {
            // Depth first, pop and process latest dir
            dir = data->dirs.back();
            data->dirs.pop_back();
            if (dir.empty()) {
                // Father change marker. 
                if (data->dirs.empty())
                    break;
                dir = data->dirs.back();
                data->dirs.pop_back();
                nfather = path_getfather(dir);
            }
        }

        // If changing parent directory, advise our user.
        if (!nfather.empty()) {
            if (path_fileprops(nfather, &st) == -1) {
                data->logsyserr("stat", nfather);
                return errno == ENOENT ? FtwOk : FtwError;
            }
            if (!(data->options & FtwOnlySkipped)) {
                status = cb.processone(nfather, FtwDirReturn, st);
                if (status & (FtwStop|FtwError)) {
                    return status;
                }
            }
        }

        if (path_fileprops(dir, &st) == -1) {
            data->logsyserr("stat", dir);
            return errno == ENOENT ? FtwOk : FtwError;
        }
        // iwalk will not recurse in this case, just process file entries
        // and append subdir entries to the queue.
        status = iwalk(dir, st, cb);
        if (status != FtwOk) {
            return status;
        }
    }
    return FtwOk;
}

// Note that the 'norecurse' flag is handled as part of the directory read. 
// This means that we always go into the top 'walk()' parameter if it is a 
// directory, even if norecurse is set. Bug or Feature ?
FsTreeWalker::Status FsTreeWalker::iwalk(
    const string &top, const struct PathStat& stp, FsTreeWalkerCB& cb)
{
    Status status = FtwOk;
    bool nullpush = false;

    // Tell user to process the top entry itself
    if (stp.pst_type == PathStat::PST_DIR) {
        if (!(data->options & FtwOnlySkipped)) {
            status = cb.processone(top, FtwDirEnter,  stp);
            if (status &  (FtwStop|FtwError)) {
                return status;
            }
        }
    } else if (stp.pst_type == PathStat::PST_REGULAR) {
        if (!(data->options & FtwOnlySkipped)) {
            return cb.processone(top, FtwRegular, stp);
        } else {
            return status;
        }
    } else {
        return status;
    }

    int curdepth = slashcount(top) - data->basedepth;
    if (data->maxdepth >= 0 && curdepth >= data->maxdepth) {
        LOGDEB1("FsTreeWalker::iwalk: Maxdepth reached: [" << top << "]\n");
        return status;
    }

    // This is a directory, read it and process entries:

#ifndef _WIN32
    // Detect if directory already seen. This could just be several
    // symlinks pointing to the same place (if FtwFollow is set), it
    // could also be some other kind of cycle. In any case, there is
    // no point in entering again.
    // For now, we'll ignore the "other kind of cycle" part and only monitor
    // this is FtwFollow is set
    if (data->options & FtwFollow) {
        DirId dirid(stp.pst_dev, stp.pst_ino);
        if (data->donedirs.find(dirid) != data->donedirs.end()) {
            LOGINF("Not processing [" << top << "] (already seen as other path)\n");
            return status;
        }
        data->donedirs.insert(dirid);
    }
#endif

    PathDirContents dc(top);
    if (!dc.opendir()) {
        data->logsyserr("opendir", top);
        switch (errno) {
        case EPERM:
        case EACCES:
        case ENOENT:
#ifdef _WIN32
            // We get this quite a lot, don't know why. To be checked.
        case EINVAL:
#endif
            // No error set: indexing will continue in other directories
            goto out;
        default:
            status = FtwError;
            goto out;
        }
    }

    const struct PathDirContents::Entry *ent;
    while (errno = 0, ((ent = dc.readdir()) != nullptr)) {
        string fn;
        struct PathStat st;
        const string& dname{ent->d_name};
        if (dname.empty()) {
            // ???
            continue;
        }
        // Maybe skip dotfiles
        if ((data->options & FtwSkipDotFiles) && dname[0] == '.')
            continue;
        // Skip . and ..
        if (dname == "." || dname == "..") 
            continue;

        // Skipped file names match ?
        if (!data->skippedNames.empty() && inSkippedNames(dname)) {
            cb.processone(path_cat(top, dname), FtwSkipped);
            continue;
        }

        fn = path_cat(top, dname);

        // Skipped file paths match ?
        // We do not check the ancestors. This means that you can have a topdirs member under a
        // skippedPath, to index a portion of an ignored area. This is the way it had always worked,
        // but this was broken by 1.13.00 and the systematic use of FNM_LEADING_DIR
        if (!data->skippedPaths.empty() && inSkippedPaths(fn, false)) {
            cb.processone(fn, FtwSkipped);
            continue;
        }

        int statret =  path_fileprops(fn.c_str(), &st, data->options&FtwFollow);
        if (statret == -1) {
            data->logsyserr("stat", fn);
#ifdef _WIN32
            int rc = GetLastError();
            LOGERR("stat(" << fn << ") failed. LastError: " << rc << "\n");
            if (rc == ERROR_NETNAME_DELETED) {
                status = FtwError;
                goto out;
            }
#endif
            continue;
        }


        if (st.pst_type == PathStat::PST_DIR) {
            if (!o_nowalkfn.empty() && path_exists(path_cat(fn, o_nowalkfn))) {
                continue;
            }
            if (data->options & FtwNoRecurse) {
                if (!(data->options & FtwOnlySkipped)) {
                    status = cb.processone(fn, FtwDirEnter, st);
                } else {
                    status = FtwOk;
                }
            } else {
                if (data->options & FtwTravNatural) {
                    status = iwalk(fn, st, cb);
                } else {
                    // If first subdir, push marker to separate
                    // from entries for other dir. This is to help
                    // with generating DirReturn callbacks
                    if (!nullpush) {
                        if (!data->dirs.empty() && !data->dirs.back().empty()) {
                            data->dirs.push_back(cstr_null);
                        }
                        nullpush = true;
                    }
                    data->dirs.push_back(fn);
                    continue;
                }
            }
            // Note: only recursive case gets here.
            if (status & (FtwStop|FtwError))
                goto out;
            if (!(data->options & FtwNoRecurse)) 
                if (!(data->options & FtwOnlySkipped)) {
                    status = cb.processone(top, FtwDirReturn, st);
                    if (status & (FtwStop|FtwError)) {
                        goto out;
                    }
                }
        } else if (st.pst_type == PathStat::PST_REGULAR || st.pst_type == PathStat::PST_SYMLINK) {
            // Filtering patterns match ?
            if (!data->onlyNames.empty()) {
                if (!inOnlyNames(dname))
                    continue;
            }
            if (!(data->options & FtwOnlySkipped)) {
                status = cb.processone(fn, FtwRegular, st);
                if (status & (FtwStop|FtwError)) {
                    goto out;
                }
            }
        }
        // We ignore other file types (devices etc...)
    } // readdir loop
    if (errno) {
        // Actual readdir error, not eof.
        data->logsyserr("readdir", top);
#ifdef _WIN32
        int rc = GetLastError();
        LOGERR("Readdir(" << top << ") failed: LastError " << rc << "\n");
        if (rc == ERROR_NETNAME_DELETED) {
            status = FtwError;
            goto out;
        }
#endif
    }

out:
    return status;
}


int64_t fsTreeBytes(const string& topdir)
{
    class bytesCB : public FsTreeWalkerCB {
    public:
        FsTreeWalker::Status processone(
            const string &, FsTreeWalker::CbFlag flg, const struct PathStat& st) override {
            if (flg == FsTreeWalker::FtwDirEnter || flg == FsTreeWalker::FtwRegular) {
#ifdef _WIN32
                totalbytes += st.pst_size;
#else
                totalbytes += st.pst_blocks * 512;
#endif
            }
            return FsTreeWalker::FtwOk;
        }
        int64_t totalbytes{0};
    };
    FsTreeWalker walker;
    bytesCB cb;
    FsTreeWalker::Status status = walker.walk(topdir, cb);
    if (status != FsTreeWalker::FtwOk) {
        LOGERR("fsTreeBytes: walker failed: " << walker.getReason() << "\n");
        return -1;
    }
    return cb.totalbytes;
}
