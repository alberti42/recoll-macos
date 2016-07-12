/* Copyright (C) 2016 J.F.Dockes
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef TEST_RCLUTIL
#include "autoconfig.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "safefcntl.h"
#include "safeunistd.h"
#include "dirent.h"
#include "cstr.h"
#ifdef _WIN32
#include "safewindows.h"
#else
#include <sys/param.h>
#include <pwd.h>
#include <sys/file.h>
#endif
#include <math.h>
#include <errno.h>
#include <sys/types.h>
#include "safesysstat.h"

#include <mutex>

#include "rclutil.h"
#include "pathut.h"
#include "wipedir.h"
#include "transcode.h"
#include "md5ut.h"

using namespace std;


void map_ss_cp_noshr(const map<string, string> s, map<string, string> *d)
{
    for (map<string, string>::const_iterator it = s.begin();
            it != s.end(); it++) {
        d->insert(
            pair<string, string>(string(it->first.begin(), it->first.end()),
                                 string(it->second.begin(), it->second.end())));
    }
}

#ifdef _WIN32
static bool path_hasdrive(const string& s)
{
    if (s.size() >= 2 && isalpha(s[0]) && s[1] == ':') {
        return true;
    }
    return false;
}
static bool path_isdriveabs(const string& s)
{
    if (s.size() >= 3 && isalpha(s[0]) && s[1] == ':' && s[2] == '/') {
        return true;
    }
    return false;
}

#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

string path_tchartoutf8(TCHAR *text)
{
#ifdef UNICODE
    // Simple C
    // const size_t size = ( wcslen(text) + 1 ) * sizeof(wchar_t);
    // wcstombs(&buffer[0], text, size);
    // std::vector<char> buffer(size);
    // Or:
    // Windows API
    std::vector<char> buffer;
    int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
    if (size > 0) {
        buffer.resize(size);
        WideCharToMultiByte(CP_UTF8, 0, text, -1,
                            &buffer[0], int(buffer.size()), NULL, NULL);
    } else {
        return string();
    }
    return string(&buffer[0]);
#else
    return text;
#endif
}
string path_thisexecpath()
{
    TCHAR text[MAX_PATH];
    GetModuleFileName(NULL, text, MAX_PATH);
#ifdef NTDDI_WIN8_future
    PathCchRemoveFileSpec(text, MAX_PATH);
#else
    PathRemoveFileSpec(text);
#endif
    string path = path_tchartoutf8(text);
    if (path.empty()) {
        path = "c:/";
    }

    return path;
}
string path_wingettempfilename(TCHAR *pref)
{
    TCHAR buf[(MAX_PATH + 1)*sizeof(TCHAR)];
    TCHAR dbuf[(MAX_PATH + 1)*sizeof(TCHAR)];
    GetTempPath(MAX_PATH + 1, dbuf);
    GetTempFileName(dbuf, pref, 0, buf);
    // Windows will have created a temp file, we delete it.
    string filename = path_tchartoutf8(buf);
    unlink(filename.c_str());
    path_slashize(filename);
    return filename;
}

#endif // _WIN32

string path_defaultrecollconfsubdir()
{
#ifdef _WIN32
    return "Recoll";
#else
    return ".recoll";
#endif
}

// Location for sample config, filters, etc. (e.g. /usr/share/recoll/)
const string& path_pkgdatadir()
{
    static string datadir;
    if (datadir.empty()) {
#ifdef _WIN32
        datadir = path_cat(path_thisexecpath(), "Share");
#else
        const char *cdatadir = getenv("RECOLL_DATADIR");
        if (cdatadir == 0) {
            // If not in environment, use the compiled-in constant.
            datadir = RECOLL_DATADIR;
        } else {
            datadir = cdatadir;
        }
#endif
    }
    return datadir;
}

// Printable url: this is used to transcode from the system charset
// into either utf-8 if transcoding succeeds, or url-encoded
bool printableUrl(const string& fcharset, const string& in, string& out)
{
    int ecnt = 0;
    if (!transcode(in, out, fcharset, "UTF-8", &ecnt) || ecnt) {
        out = url_encode(in, 7);
    }
    return true;
}

string url_gpathS(const string& url)
{
#ifdef _WIN32
    string u = url_gpath(url);
    string nu;
    if (path_hasdrive(u)) {
        nu.append(1, '/');
        nu.append(1, u[0]);
        if (path_isdriveabs(u)) {
            nu.append(u.substr(2));
        } else {
            // This should be an error really
            nu.append(1, '/');
            nu.append(u.substr(2));
        }
    }
    return nu;
#else
    return url_gpath(url);
#endif
}

const string& tmplocation()
{
    static string stmpdir;
    if (stmpdir.empty()) {
        const char *tmpdir = getenv("RECOLL_TMPDIR");
        if (tmpdir == 0) {
            tmpdir = getenv("TMPDIR");
        }
        if (tmpdir == 0) {
            tmpdir = getenv("TMP");
        }
        if (tmpdir == 0) {
            tmpdir = getenv("TEMP");
        }
        if (tmpdir == 0) {
#ifdef _WIN32
            TCHAR bufw[(MAX_PATH + 1)*sizeof(TCHAR)];
            GetTempPath(MAX_PATH + 1, bufw);
            stmpdir = path_tchartoutf8(bufw);
#else
            stmpdir = "/tmp";
#endif
        } else {
            stmpdir = tmpdir;
        }
        stmpdir = path_canon(stmpdir);
    }

    return stmpdir;
}

bool maketmpdir(string& tdir, string& reason)
{
#ifndef _WIN32
    tdir = path_cat(tmplocation(), "rcltmpXXXXXX");

    char *cp = strdup(tdir.c_str());
    if (!cp) {
        reason = "maketmpdir: out of memory (for file name !)\n";
        tdir.erase();
        return false;
    }

    // There is a race condition between name computation and
    // mkdir. try to make sure that we at least don't shoot ourselves
    // in the foot
#if !defined(HAVE_MKDTEMP) || defined(_WIN32)
    static std::mutex mmutex;
    std::unique_lock lock(mmutex);
#endif

    if (!
#ifdef HAVE_MKDTEMP
            mkdtemp(cp)
#else
            mktemp(cp)
#endif // HAVE_MKDTEMP
       ) {
        free(cp);
        reason = "maketmpdir: mktemp failed for [" + tdir + "] : " +
                 strerror(errno);
        tdir.erase();
        return false;
    }
    tdir = cp;
    free(cp);
#else // _WIN32
    // There is a race condition between name computation and
    // mkdir. try to make sure that we at least don't shoot ourselves
    // in the foot
    static std::mutex mmutex;
    std::unique_lock lock(mmutex);
    tdir = path_wingettempfilename(TEXT("rcltmp"));
#endif

    // At this point the directory does not exist yet except if we used
    // mkdtemp

#if !defined(HAVE_MKDTEMP) || defined(_WIN32)
    if (mkdir(tdir.c_str(), 0700) < 0) {
        reason = string("maketmpdir: mkdir ") + tdir + " failed";
        tdir.erase();
        return false;
    }
#endif

    return true;
}

TempFileInternal::TempFileInternal(const string& suffix)
    : m_noremove(false)
{
    // Because we need a specific suffix, can't use mkstemp
    // well. There is a race condition between name computation and
    // file creation. try to make sure that we at least don't shoot
    // our own selves in the foot. maybe we'll use mkstemps one day.
    static std::mutex mmutex;
    std::unique_lock<std::mutex> lock(mmutex);

#ifndef _WIN32
    string filename = path_cat(tmplocation(), "rcltmpfXXXXXX");
    char *cp = strdup(filename.c_str());
    if (!cp) {
        m_reason = "Out of memory (for file name !)\n";
        return;
    }

    // Using mkstemp this way is awful (bot the suffix adding and
    // using mkstemp() instead of mktemp just to avoid the warnings)
    int fd;
    if ((fd = mkstemp(cp)) < 0) {
        free(cp);
        m_reason = "TempFileInternal: mkstemp failed\n";
        return;
    }
    close(fd);
    unlink(cp);
    filename = cp;
    free(cp);
#else
    string filename = path_wingettempfilename(TEXT("recoll"));
#endif

    m_filename = filename + suffix;
    if (close(open(m_filename.c_str(), O_CREAT | O_EXCL, 0600)) != 0) {
        m_reason = string("Could not open/create") + m_filename;
        m_filename.erase();
    }
}

TempFileInternal::~TempFileInternal()
{
    if (!m_filename.empty() && !m_noremove) {
        unlink(m_filename.c_str());
    }
}

TempDir::TempDir()
{
    if (!maketmpdir(m_dirname, m_reason)) {
        m_dirname.erase();
        return;
    }
}

TempDir::~TempDir()
{
    if (!m_dirname.empty()) {
        (void)wipedir(m_dirname, true, true);
        m_dirname.erase();
    }
}

bool TempDir::wipe()
{
    if (m_dirname.empty()) {
        m_reason = "TempDir::wipe: no directory !\n";
        return false;
    }
    if (wipedir(m_dirname, false, true)) {
        m_reason = "TempDir::wipe: wipedir failed\n";
        return false;
    }
    return true;
}

// Freedesktop standard paths for cache directory (thumbnails are now in there)
static const string& xdgcachedir()
{
    static string xdgcache;
    if (xdgcache.empty()) {
        const char *cp = getenv("XDG_CACHE_HOME");
        if (cp == 0) {
            xdgcache = path_cat(path_home(), ".cache");
        } else {
            xdgcache = string(cp);
        }
    }
    return xdgcache;
}
static const string& thumbnailsdir()
{
    static string thumbnailsd;
    if (thumbnailsd.empty()) {
        thumbnailsd = path_cat(xdgcachedir(), "thumbnails");
        if (access(thumbnailsd.c_str(), 0) != 0) {
            thumbnailsd = path_cat(path_home(), ".thumbnails");
        }
    }
    return thumbnailsd;
}

// Place for 256x256 files
static const string thmbdirlarge = "large";
// 128x128
static const string thmbdirnormal = "normal";

static void thumbname(const string& url, string& name)
{
    string digest;
    string l_url = url_encode(url);
    MD5String(l_url, digest);
    MD5HexPrint(digest, name);
    name += ".png";
}

bool thumbPathForUrl(const string& url, int size, string& path)
{
    string name;
    thumbname(url, name);
    if (size <= 128) {
        path = path_cat(thumbnailsdir(), thmbdirnormal);
        path = path_cat(path, name);
        if (access(path.c_str(), R_OK) == 0) {
            return true;
        }
    }
    path = path_cat(thumbnailsdir(), thmbdirlarge);
    path = path_cat(path, name);
    if (access(path.c_str(), R_OK) == 0) {
        return true;
    }

    // File does not exist. Path corresponds to the large version at this point,
    // fix it if needed.
    if (size <= 128) {
        path = path_cat(path_home(), thmbdirnormal);
        path = path_cat(path, name);
    }
    return false;
}

void rclutil_init_mt()
{
    path_pkgdatadir();
    tmplocation();
    thumbnailsdir();
}

#else // TEST_RCLUTIL

void path_to_thumb(const string& _input)
{
    string input(_input);
    // Make absolute path if needed
    if (input[0] != '/') {
        input = path_absolute(input);
    }

    input = string("file://") + path_canon(input);

    string path;
    //path = url_encode(input, 7);
    thumbPathForUrl(input, 7, path);
    cout << path << endl;
}

const char *thisprog;

int main(int argc, const char **argv)
{
    thisprog = *argv++;
    argc--;

    string s;
    vector<string>::const_iterator it;

#if 0
    if (argc > 1) {
        cerr <<  "Usage: thumbpath <filepath>" << endl;
        exit(1);
    }
    string input;
    if (argc == 1) {
        input = *argv++;
        if (input.empty())  {
            cerr << "Usage: thumbpath <filepath>" << endl;
            exit(1);
        }
        path_to_thumb(input);
    } else {
        while (getline(cin, input)) {
            path_to_thumb(input);
        }
    }
    exit(0);
#endif
}

#endif // TEST_RCLUTIL

