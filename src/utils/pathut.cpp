/* Copyright (C) 2004 J.F.Dockes
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

#ifndef TEST_PATHUT
#include "autoconfig.h"

#include <stdio.h>
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
#include "ptmutex.h"

// Let's include all files where statfs can be defined and hope for no
// conflict...
#ifdef HAVE_SYS_MOUNT_H 
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_STATFS_H 
#include <sys/statfs.h>
#endif
#ifdef HAVE_SYS_STATVFS_H 
#include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_VFS_H 
#include <sys/vfs.h>
#endif

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stack>
#include <set>
#include <vector>

#include "pathut.h"
#include "transcode.h"
#include "wipedir.h"
#include "md5ut.h"

using namespace std;

#ifdef _WIN32
/// Convert \ separators to /
void path_slashize(string& s)
{
    for (string::size_type i = 0; i < s.size(); i++) {
        if (s[i] == '\\')
            s[i] = '/';
    }
}
static bool path_strlookslikedrive(const string& s)
{
    return s.size() == 2 && isalpha(s[0]) && s[1] == ':';
}

static bool path_hasdrive(const string& s)
{
    if (s.size() >= 2 && isalpha(s[0]) && s[1] == ':')
        return true;
    return false;
}
static bool path_isdriveabs(const string& s)
{
    if (s.size() >= 3 && isalpha(s[0]) && s[1] == ':' && s[2] == '/')
        return true;
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
    DWORD length = GetModuleFileName(NULL, text, MAX_PATH);
#ifdef NTDDI_WIN8_future
    PathCchRemoveFileSpec(text, MAX_PATH);
#else
    PathRemoveFileSpec(text);
#endif
    string path = path_tchartoutf8(text);
    if (path.empty())
        path = "c:/";

    return path;
}

string path_wingettempfilename(TCHAR *pref)
{
    TCHAR buf[(MAX_PATH +1)*sizeof(TCHAR)];
    TCHAR dbuf[(MAX_PATH +1)*sizeof(TCHAR)];
    GetTempPath(MAX_PATH+1, dbuf);
    GetTempFileName(dbuf, pref, 0, buf);
    // Windows will have created a temp file, we delete it.
    string filename = path_tchartoutf8(buf);
    unlink(filename.c_str());
    return filename;
}
#endif


bool fsocc(const string &path, int *pc, long long *blocks)
{
#ifdef _WIN32
	return false;
#else
#ifdef sun
    struct statvfs buf;
    if (statvfs(path.c_str(), &buf) != 0) {
	return false;
    }
#else
    struct statfs buf;
    if (statfs(path.c_str(), &buf) != 0) {
	return false;
    }
#endif

    // used blocks
    double fpc = 0.0;
#define FSOCC_USED (double(buf.f_blocks - buf.f_bfree))
#define FSOCC_TOTAVAIL (FSOCC_USED + double(buf.f_bavail))
    if (FSOCC_TOTAVAIL > 0) {
	fpc = 100.0 * FSOCC_USED / FSOCC_TOTAVAIL;
    }
    *pc = int(fpc);
    if (blocks) {
	*blocks = 0;
#define FSOCC_MB (1024*1024)
	if (buf.f_bsize > 0) {
	    int ratio = buf.f_bsize > FSOCC_MB ? buf.f_bsize / FSOCC_MB :
		FSOCC_MB / buf.f_bsize;

	    *blocks = buf.f_bsize > FSOCC_MB ? 
                ((long long)buf.f_bavail) * ratio :
		((long long)buf.f_bavail) / ratio;
	}
    }
    return true;
#endif
}

const string& tmplocation()
{
    static string stmpdir;
    if (stmpdir.empty()) {
        const char *tmpdir = getenv("RECOLL_TMPDIR");
        if (tmpdir == 0) 
            tmpdir = getenv("TMPDIR");
        if (tmpdir == 0) 
            tmpdir = getenv("TMP");
        if (tmpdir == 0) 
            tmpdir = getenv("TEMP");
        if (tmpdir == 0) {
#ifdef _WIN32
            TCHAR bufw[(MAX_PATH+1)*sizeof(TCHAR)];
            GetTempPath(MAX_PATH+1, bufw);
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

// Location for sample config, filters, etc. (e.g. /usr/share/recoll/)
const string& path_sharedatadir()
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
    static PTMutexInit mlock;
    PTMutexLocker lock(mlock);
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
    static PTMutexInit mlock;
    PTMutexLocker lock(mlock);
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
    static PTMutexInit mlock;
    PTMutexLocker lock(mlock);

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
    if (close(open(m_filename.c_str(), O_CREAT|O_EXCL, 0600)) != 0) {
	m_reason = string("Could not open/create") + m_filename;
	m_filename.erase();
    }
}

TempFileInternal::~TempFileInternal()
{
    if (!m_filename.empty() && !m_noremove)
	unlink(m_filename.c_str());
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

void path_catslash(string &s) {
    if (s.empty() || s[s.length() - 1] != '/')
	s += '/';
}

string path_cat(const string &s1, const string &s2) {
    string res = s1;
    path_catslash(res);
    res +=  s2;
    return res;
}

string path_getfather(const string &s) {
    string father = s;

    // ??
    if (father.empty())
	return "./";

    if (path_isroot(father))
        return father;
    
    if (father[father.length() - 1] == '/') {
	// Input ends with /. Strip it, root special case was tested above
	father.erase(father.length()-1);
    }

    string::size_type slp = father.rfind('/');
    if (slp == string::npos)
	return "./";

    father.erase(slp);
    path_catslash(father);
    return father;
}

string path_getsimple(const string &s) {
    string simple = s;

    if (simple.empty())
	return simple;

    string::size_type slp = simple.rfind('/');
    if (slp == string::npos)
	return simple;

    simple.erase(0, slp+1);
    return simple;
}

string path_basename(const string &s, const string &suff)
{
    string simple = path_getsimple(s);
    string::size_type pos = string::npos;
    if (suff.length() && simple.length() > suff.length()) {
	pos = simple.rfind(suff);
	if (pos != string::npos && pos + suff.length() == simple.length())
	    return simple.substr(0, pos);
    } 
    return simple;
}

string path_suffix(const string& s)
{
    string::size_type dotp = s.rfind('.');
    if (dotp == string::npos)
	return string();
    return s.substr(dotp+1);
}

string path_home()
{
#ifdef _WIN32
    string dir;
    const char *cp = getenv("USERPROFILE");
    if (cp != 0) {
        dir = cp;
    }
    if (dir.empty()) {
        cp = getenv("HOMEDRIVE");
        if (cp != 0) {
            const char *cp1 = getenv("HOMEPATH");
            if (cp1 != 0) {
                dir = string(cp) + string(cp1);
            }
        }
    }
    if (dir.empty()) {
        dir = "C:\\";
    }
    dir = path_canon(dir);
    path_catslash(dir);
    return dir;
#else
    uid_t uid = getuid();

    struct passwd *entry = getpwuid(uid);
    if (entry == 0) {
	const char *cp = getenv("HOME");
	if (cp)
	    return cp;
	else 
	return "/";
    }

    string homedir = entry->pw_dir;
    path_catslash(homedir);
    return homedir;
#endif
}

// The default place to store the default config and other stuff (e.g webqueue)
string path_homedata()
{
#ifdef _WIN32
    const char *cp = getenv("LOCALAPPDATA");
    string dir;
    if (cp != 0) {
        dir = path_canon(cp);
    }
    if (dir.empty()) {
        dir = path_cat(path_home(), "AppData/Local/");
    }
    return dir;
#else
    // We should use an xdg-conforming location, but, history...
    return path_home();
#endif
}

string path_tildexpand(const string &s) 
{
    if (s.empty() || s[0] != '~')
	return s;
    string o = s;
    if (s.length() == 1) {
	o.replace(0, 1, path_home());
    } else if  (s[1] == '/') {
	o.replace(0, 2, path_home());
    } else {
	string::size_type pos = s.find('/');
        string::size_type l = (pos == string::npos) ? s.length() - 1 : pos - 1;
#ifdef _WIN32
        // Dont know what this means. Just replace with HOME
        o.replace(0, l+1, path_home());
#else
	struct passwd *entry = getpwnam(s.substr(1, l).c_str());
	if (entry)
	    o.replace(0, l+1, entry->pw_dir);
#endif
    }
    return o;
}

bool path_isroot(const string& path)
{
    if (path.size() == 1 && path[0] == '/')
        return true;
#ifdef _WIN32
    if (path.size() == 3 && isalpha(path[0]) && path[1] == ':' && path[2] == '/')
        return true;
#endif
    return false;
}

bool path_isabsolute(const string &path)
{
    if (!path.empty() && (path[0] == '/'
#ifdef _WIN32
                          || path_isdriveabs(path)
#endif
            )) {
        return true;
    } 
    return false;
}
    
string path_absolute(const string &is)
{
    if (is.length() == 0)
	return is;
    string s = is;
    if (s[0] != '/') {
	char buf[MAXPATHLEN];
	if (!getcwd(buf, MAXPATHLEN)) {
	    return string();
	}
	s = path_cat(string(buf), s); 
    }
    return s;
}

#include <smallut.h>
string path_canon(const string &is, const string* cwd)
{
    if (is.length() == 0)
	return is;
    string s = is;
#ifdef _WIN32
    path_slashize(s);
    // fix possible path from file: absolute url
    if (s.size() && s[0] == '/' && path_hasdrive(s.substr(1))) {
        s = s.substr(1);
    }
#endif

    if (!path_isabsolute(s)) {
	char buf[MAXPATHLEN];
	const char *cwdp = buf;
	if (cwd) {
	    cwdp = cwd->c_str();
	} else {
	    if (!getcwd(buf, MAXPATHLEN)) {
		return string();
	    }
	}
	s = path_cat(string(cwdp), s); 
    }
    vector<string> elems;
    stringToTokens(s, elems, "/");
    vector<string> cleaned;
    for (vector<string>::const_iterator it = elems.begin(); 
	 it != elems.end(); it++){
	if (*it == "..") {
	    if (!cleaned.empty())
		cleaned.pop_back();
	} else if (it->empty() || *it == ".") {
	} else {
	    cleaned.push_back(*it);
	}
    }
    string ret;
    if (!cleaned.empty()) {
	for (vector<string>::const_iterator it = cleaned.begin(); 
	     it != cleaned.end(); it++) {
            ret += "/";
#ifdef _WIN32
            if (it == cleaned.begin() && path_strlookslikedrive(*it)) {
                // Get rid of just added initial "/"
                ret.clear();
            }
#endif
	    ret += *it;
	}
    } else {
	ret = "/";
    }
    return ret;
}

bool makepath(const string& ipath)
{
    string path = path_canon(ipath);
    vector<string> elems;
    stringToTokens(path, elems, "/");
    path = "/";
    for (vector<string>::const_iterator it = elems.begin(); 
	 it != elems.end(); it++){
#ifdef _WIN32
        if (it == elems.begin() && path_strlookslikedrive(*it))
            path = "";
#endif
	path += *it;
	// Not using path_isdir() here, because this cant grok symlinks
	// If we hit an existing file, no worry, mkdir will just fail.
	if (access(path.c_str(), 0) != 0) {
	    if (mkdir(path.c_str(), 0700) != 0)  {
		return false;
	    }
	}
	path += "/";
    }
    return true;
}

bool path_isdir(const string& path)
{
    struct stat st;
    if (lstat(path.c_str(), &st) < 0) 
	return false;
    if (S_ISDIR(st.st_mode))
	return true;
    return false;
}

long long path_filesize(const string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) < 0) 
	return -1;
    return (long long)st.st_size;
}

bool path_exists(const string& path)
{
    return access(path.c_str(), 0) == 0;
}

// Allowed punctuation in the path part of an URI according to RFC2396
// -_.!~*'():@&=+$,
/*
21 !
    22 "
    23 #
24 $
    25 %
26 &
27 '
28 (
29 )
2A *
2B +
2C , 
2D -
2E .
2F /
30 0
...
39 9
3A :
    3B ;
    3C <
3D =
    3E >
    3F ?
40 @
41 A
...
5A Z
    5B [
    5C \
    5D ]
    5E ^
5F _
    60 `
61 a
...
7A z
    7B {
    7C |
    7D }
7E ~
    7F DEL
*/
string url_encode(const string& url, string::size_type offs)
{
    string out = url.substr(0, offs);
    const char *cp = url.c_str();
    for (string::size_type i = offs; i < url.size(); i++) {
	unsigned int c;
	const char *h = "0123456789ABCDEF";
	c = cp[i];
	if (c <= 0x20 || 
	   c >= 0x7f || 
	   c == '"' ||
	   c == '#' ||
	   c == '%' ||
	   c == ';' ||
	   c == '<' ||
	   c == '>' ||
	   c == '?' ||
	   c == '[' ||
	   c == '\\' ||
	   c == ']' ||
	   c == '^' ||
	   c == '`' ||
	   c == '{' ||
	   c == '|' ||
	   c == '}' ) {
	    out += '%';
	    out += h[(c >> 4) & 0xf];
	    out += h[c & 0xf];
	} else {
	    out += char(c);
	}
    }
    return out;
}

string url_gpath(const string& url)
{
    // Remove the access schema part (or whatever it's called)
    string::size_type colon = url.find_first_of(":");
    if (colon == string::npos || colon == url.size() - 1)
        return url;
    // If there are non-alphanum chars before the ':', then there
    // probably is no scheme. Whatever...
    for (string::size_type i = 0; i < colon; i++) {
        if (!isalnum(url.at(i)))
            return url;
    }

    // In addition we canonize the path to remove empty host parts
    // (for compatibility with older versions of recoll where file://
    // was hardcoded, but the local path was used for doc
    // identification.
    return path_canon(url.substr(colon+1));
}

string url_parentfolder(const string& url)
{
    // In general, the parent is the directory above the full path
    string parenturl = path_getfather(url_gpath(url));
    // But if this is http, make sure to keep the host part. Recoll
    // only has file or http urls for now.
    bool isfileurl = urlisfileurl(url);
    if (!isfileurl && parenturl == "/") {
        parenturl = url_gpath(url);
    }
    return isfileurl ? string("file://") + parenturl :
        string("http://") + parenturl;
}


string path_defaultrecollconfsubdir()
{
#ifdef _WIN32
    return "Recoll";
#else
    return ".recoll";
#endif
}

// Convert to file path if url is like file:
// Note: this only works with our internal pseudo-urls which are not
// encoded/escaped
string fileurltolocalpath(string url)
{
    if (url.find("file://") == 0)
        url = url.substr(7, string::npos);
    else
        return string();

#ifdef _WIN32
    // Absolute file urls are like: file:///c:/mydir/...
    // Get rid of the initial '/'
    if (url.size() >= 3 && url[0] == '/' && isalpha(url[1]) && url[2] == ':') {
        url = url.substr(1);
    }
#endif

    // Removing the fragment part. This is exclusively used when
    // executing a viewer for the recoll manual, and we only strip the
    // part after # if it is preceded by .html
    string::size_type pos;
    if ((pos = url.rfind(".html#")) != string::npos) {
        url.erase(pos+5);
    } else if ((pos = url.rfind(".htm#")) != string::npos) {
        url.erase(pos+4);
    }

    return url;
}

string path_pathtofileurl(const string& path)
{
	// We're supposed to receive a canonic absolute path, but on windows we
	// may need to add a '/' in front of the drive spec
	string url(cstr_fileu);
	if (path.empty() || path[0] != '/')
		url.push_back('/');
	url += path;
	return url;
}

bool urlisfileurl(const string& url)
{
    return url.find("file://") == 0;
}

// Printable url: this is used to transcode from the system charset
// into either utf-8 if transcoding succeeds, or url-encoded
bool printableUrl(const string &fcharset, const string &in, string &out)
{
    int ecnt = 0;
    if (!transcode(in, out, fcharset, "UTF-8", &ecnt) || ecnt) {
	out = url_encode(in, 7);
    }
    return true;
}

bool readdir(const string& dir, string& reason, set<string>& entries)
{
    struct stat st;
    int statret;
    ostringstream msg;
    DIR *d = 0;
    statret = lstat(dir.c_str(), &st);
    if (statret == -1) {
	msg << "readdir: cant stat " << dir << " errno " <<  errno;
	goto out;
    }
    if (!S_ISDIR(st.st_mode)) {
	msg << "readdir: " << dir <<  " not a directory";
	goto out;
    }
    if (access(dir.c_str(), R_OK) < 0) {
	msg << "readdir: no read access to " << dir;
	goto out;
    }

    d = opendir(dir.c_str());
    if (d == 0) {
	msg << "readdir: cant opendir " << dir << ", errno " << errno;
	goto out;
    }

    struct dirent *ent;
    while ((ent = readdir(d)) != 0) {
	if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) 
	    continue;
	entries.insert(ent->d_name);
    }

out:
    if (d)
	closedir(d);
    reason = msg.str();
    if (reason.empty())
	return true;
    return false;
}

// We do not want to mess with the pidfile content in the destructor:
// the lock might still be in use in a child process. In fact as much
// as we'd like to reset the pid inside the file when we're done, it
// would be very difficult to do it right and it's probably best left
// alone.
Pidfile::~Pidfile()
{
    if (m_fd >= 0)
	::close(m_fd);
    m_fd = -1;
}

pid_t Pidfile::read_pid()
{
    int fd = ::open(m_path.c_str(), O_RDONLY);
    if (fd == -1)
	return (pid_t)-1;

    char buf[16];
    int i = read(fd, buf, sizeof(buf) - 1);
    ::close(fd);
    if (i <= 0)
	return (pid_t)-1;
    buf[i] = '\0';
    char *endptr;
    pid_t pid = strtol(buf, &endptr, 10);
    if (endptr != &buf[i])
	return (pid_t)-1;
    return pid;
}

int Pidfile::flopen()
{
    const char *path = m_path.c_str();
    if ((m_fd = ::open(path, O_RDWR|O_CREAT, 0644)) == -1) {
	m_reason = "Open failed: [" + m_path + "]: " + strerror(errno);
	return -1;
    }

#ifdef sun
    struct flock lockdata;
    lockdata.l_start = 0;
    lockdata.l_len = 0;
    lockdata.l_type = F_WRLCK;
    lockdata.l_whence = SEEK_SET;
    if (fcntl(m_fd, F_SETLK,  &lockdata) != 0) {
	int serrno = errno;
	(void)::close(m_fd);
	errno = serrno;
	m_reason = "fcntl lock failed";
	return -1;
    }
#else
#ifdef _WIN32
	return 0;
#else
    int operation = LOCK_EX | LOCK_NB;
    if (flock(m_fd, operation) == -1) {
	int serrno = errno;
	(void)::close(m_fd);
	errno = serrno;
	m_reason = "flock failed";
	return -1;
    }
#endif // ! win32
#endif // ! sun

    if (ftruncate(m_fd, 0) != 0) {
	/* can't happen [tm] */
	int serrno = errno;
	(void)::close(m_fd);
	errno = serrno;
	m_reason = "ftruncate failed";
	return -1;
    }
    return 0;
}

pid_t Pidfile::open()
{
    if (flopen() < 0) {
	return read_pid();
    }
    return (pid_t)0;
}

int Pidfile::write_pid()
{
    /* truncate to allow multiple calls */
    if (ftruncate(m_fd, 0) == -1) {
	m_reason = "ftruncate failed";
	return -1;
    }
    char pidstr[20];
    sprintf(pidstr, "%u", int(getpid()));
    lseek(m_fd, 0, 0);
    if (::write(m_fd, pidstr, strlen(pidstr)) != (ssize_t)strlen(pidstr)) {
	m_reason = "write failed";
	return -1;
    }
    return 0;
}

int Pidfile::close()
{
    return ::close(m_fd);
}

int Pidfile::remove()
{
    return unlink(m_path.c_str());
}


// Freedesktop standard paths for cache directory (thumbnails are now in there)
static const string& xdgcachedir()
{
    static string xdgcache;
    if (xdgcache.empty()) {
	const char *cp = getenv("XDG_CACHE_HOME");
	if (cp == 0) 
	    xdgcache = path_cat(path_home(), ".cache");
	else
	    xdgcache = string(cp);
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

// Call funcs that need static init (not initially reentrant)
void pathut_init_mt()
{
    path_home();
    tmplocation();
    thumbnailsdir();
    path_sharedatadir();
}


#else // TEST_PATHUT
#include <stdlib.h>
#include <iostream>
using namespace std;

#include "pathut.h"

void path_to_thumb(const string& _input)
{
    string input(_input);
    // Make absolute path if needed
    if (input[0] != '/')
        input = path_absolute(input);

    input = string("file://") + path_canon(input);

    string path;
    //path = url_encode(input, 7);
    thumbPathForUrl(input, 7, path);
    cout << path << endl;
}

const char *tstvec[] = {"", "/", "/dir", "/dir/", "/dir1/dir2",
			 "/dir1/dir2",
			"./dir", "./dir1/", "dir", "../dir", "/dir/toto.c",
			"/dir/.c", "/dir/toto.txt", "toto.txt1"
};

const string ttvec[] = {"/dir", "", "~", "~/sub", "~root", "~root/sub",
		 "~nosuch", "~nosuch/sub"};
int nttvec = sizeof(ttvec) / sizeof(string);

const char *thisprog;

int main(int argc, const char **argv)
{
    thisprog = *argv++;argc--;

    string s;
    vector<string>::const_iterator it;
#if 0
    for (unsigned int i = 0;i < sizeof(tstvec) / sizeof(char *); i++) {
	cout << tstvec[i] << " Father " << path_getfather(tstvec[i]) << endl;
    }
    for (unsigned int i = 0;i < sizeof(tstvec) / sizeof(char *); i++) {
	cout << tstvec[i] << " Simple " << path_getsimple(tstvec[i]) << endl;
    }
    for (unsigned int i = 0;i < sizeof(tstvec) / sizeof(char *); i++) {
	cout << tstvec[i] << " Basename " << 
	    path_basename(tstvec[i], ".txt") << endl;
    }
#endif

#if 0
    for (int i = 0; i < nttvec; i++) {
	cout << "tildexp: '" << ttvec[i] << "' -> '" << 
	    path_tildexpand(ttvec[i]) << "'" << endl;
    }
#endif

#if 0
    const string canontst[] = {"/dir1/../../..", "/////", "", 
			       "/dir1/../../.././/////dir2///////",
			       "../../", 
			       "../../../../../../../../../../"
    };
    unsigned int nttvec = sizeof(canontst) / sizeof(string);
    for (unsigned int i = 0; i < nttvec; i++) {
	cout << "canon: '" << canontst[i] << "' -> '" << 
	    path_canon(canontst[i]) << "'" << endl;
    }
#endif    
#if 0
    if (argc != 2) {
	cerr << "Usage: trpathut <dir> <pattern>" << endl;
	exit(1);
    }
    string dir = *argv++;argc--;
    string pattern =  *argv++;argc--;
    vector<string> matched = path_dirglob(dir, pattern);
    for (it = matched.begin(); it != matched.end();it++) {
	cout << *it << endl;
    }
#endif

#if 0
    if (argc != 1) {
	fprintf(stderr, "Usage: fsocc: trpathut <path>\n");
	exit(1);
    }
  string path = *argv++;argc--;

  int pc;
  long long blocks;
  if (!fsocc(path, &pc, &blocks)) {
      fprintf(stderr, "fsocc failed\n");
      return 1;
  }
  printf("pc %d, megabytes %ld\n", pc, blocks);
#endif

#if 0
  Pidfile pidfile("/tmp/pathutpidfile");
  pid_t pid;
  if ((pid = pidfile.open()) != 0) {
      cerr << "open failed. reason: " << pidfile.getreason() << 
	  " return " << pid << endl;
      exit(1);
  }
  pidfile.write_pid();
  sleep(10);
  pidfile.close();
  pidfile.remove();
#endif

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
      while (getline(cin, input))
          path_to_thumb(input);
  }

  
  exit(0);
#endif

#if 0
    if (argc != 1) {
	cerr << "Usage: trpathut <filename>" << endl;
	exit(1);
    }
    string fn = *argv++;argc--;
    string ext = path_suffix(fn);
    cout << "Suffix: [" << ext << "]" << endl;
    return 0;
#endif

#if 1
    if (argc != 1) {
	cerr << "Usage: trpathut url" << endl;
	exit(1);
    }
    string url = *argv++;argc--;

    cout << "File: [" << fileurltolocalpath(url) << "]\n";
    return 0;
#endif


}

#endif // TEST_PATHUT

