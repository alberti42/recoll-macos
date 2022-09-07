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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "autoconfig.h"

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <fnmatch.h>
#ifndef _WIN32
#include <sys/time.h>
#include <sys/resource.h>
#else
#include <direct.h>
#endif
#include "safefcntl.h"
#include "safeunistd.h"
#include <getopt.h>

#include <iostream>
#include <list>
#include <string>
#include <cstdlib>

using namespace std;

#include "log.h"
#include "rclinit.h"
#include "rclconfig.h"
#include "smallut.h"
#include "readfile.h"
#include "pathut.h"
#include "rclutil.h"
#include "cancelcheck.h"
#include "execmd.h"
#include "internfile.h"
#include "rcldoc.h"
#include "fstreewalk.h"

// Command line options
static int     op_flags;
#define OPT_H 0x1
#define OPT_L 0x2
#define OPT_c 0x4
#define OPT_e 0x8
#define OPT_f 0x10    
#define OPT_h 0x20
#define OPT_i 0x40
#define OPT_l 0x80
#define OPT_p 0x100
#define OPT_v 0x200
#define OPT_x 0x400
#define OPT_n 0x800
#define OPT_A 0x1000
#define OPT_B 0x2000
#define OPT_C 0x4000

#define OPTVAL_RECOLL_CONFIG 1000
#define OPTVAL_HELP 1001

static struct option long_options[] = {
    {"regexp", required_argument, 0, 'e'},
    {"file", required_argument, 0, 'f'},
    {"invert-match", required_argument, 0, 'v'},
    {"word-regexp", 0, 0, 'w'}, // Unimplemented
    {"line-regexp", 0, 0, 'x'},
    {"config", required_argument, 0, OPTVAL_RECOLL_CONFIG},
    {"count", 0, 0, 'c'},
    {"files-without-match", 0, 0, 'L'},
    {"files-with-match", 0, 0, 'l'},
    {"with-filename", 0, 0, 'H'},
    {"no-filename", 0, 0, 'h'},
    {"line-number", 0, 0, 'n'},
    {"help", 0, 0, OPTVAL_HELP},
    {"after-context", required_argument, 0, 'A'},
    {"before-context", required_argument, 0, 'B'},
    {"context", required_argument, 0, 'C'},
    {0, 0, 0, 0}
};

std::vector<SimpleRegexp *> g_expressions;
int g_reflags = SimpleRegexp::SRE_NOSUB;

static RclConfig *config;

// Working directory before we change: it's simpler to change early
// but some options need the original for computing absolute paths.
static std::string orig_cwd;
static std::string current_topdir;
static int beforecontext;
static int aftercontext;

void grepit(const Rcl::Doc& doc)
{
    std::vector<std::string> lines;
    int matchcount = 0;
    stringToTokens(doc.text, lines, "\n");

    std::string ppath;
    if (op_flags & OPT_H) {
        ppath = fileurltolocalpath(doc.url);
        if (ppath.size() > current_topdir.size()) {
            ppath = ppath.substr(current_topdir.size());
        }
        ppath += ":";
        ppath += doc.ipath + "::";
    }
    int lnum = 0;
    std::string ln;
    bool inmatch{false};
    for (const auto& line: lines) {
        ++lnum;
        //std::cout << "LINE:[" << line << "]\n";
        for (const auto e_p : g_expressions) {
            auto match = e_p->simpleMatch(line);
            if (((op_flags & OPT_v) && match) || (!(op_flags&OPT_v) && !match)) {
                inmatch = false;
                goto nextline;
            }
        }
        if (op_flags & OPT_c) {
            matchcount++;
        } else {
            if (op_flags & OPT_n) {
                ln = ulltodecstr(lnum) + ":";
            }
            int idx = lnum -1;
            if (beforecontext) {
                for (int i = std::max(0, idx - beforecontext); i < idx; i++) {
                    std::cout << ppath << ln << lines[i] << "\n";
                }
            }
            std::cout << ppath << ln << line << "\n";
            if (aftercontext && idx < int(lines.size() - 1)) {
                for (int i = idx + 1; i < std::min(int(lines.size()), idx + aftercontext + 1); i++) {
                    std::cout << ppath << ln << lines[i] << "\n";
                }
                std::cout << "--\n";
            }
        }
    nextline:
        continue;
    }
    if (op_flags & OPT_L) {
        if (matchcount == 0) {
            std::cout << ppath << "\n";
        }
    } else if (op_flags & OPT_l) {
        if (matchcount) {
            std::cout << ppath << "\n";
        }
    } else if (op_flags & OPT_c) {
        std::cout << ppath << matchcount << "\n";
    }
}

bool processpath(RclConfig *config, const std::string& path)
{
    LOGINF("processpath: [" << path << "]\n");
    struct PathStat st;
    if (path_fileprops(path, &st, false) < 0) {
        std::cerr << path << " : ";
        perror("stat");
        return false;
    }
    
    config->setKeyDir(path_getfather(path));
    
    string mimetype;

    FileInterner interner(path, &st, config, FileInterner::FIF_none);
    if (!interner.ok()) {
        return false;
    }
    mimetype = interner.getMimetype();

    FileInterner::Status fis = FileInterner::FIAgain;
    bool hadNonNullIpath = false;
    Rcl::Doc doc;
    while (fis == FileInterner::FIAgain) {
        doc.erase();
        try {
            fis = interner.internfile(doc);
        } catch (CancelExcept) {
            LOGERR("fsIndexer::processone: interrupted\n");
            return false;
        }
        if (fis == FileInterner::FIError) {
            return false;
        }
            
        if (doc.url.empty())
            doc.url = path_pathtofileurl(path);

        grepit(doc);
    }

    return true;
}


class WalkerCB : public FsTreeWalkerCB {
public:
    WalkerCB(list<string>& files, const vector<string>& selpats, RclConfig *config)
        : m_files(files), m_pats(selpats), m_config(config) {}
    virtual FsTreeWalker::Status processone(
        const string& fn, const struct PathStat *, FsTreeWalker::CbFlag flg) {
        if (flg == FsTreeWalker::FtwRegular) {
            if (m_pats.empty()) {
                processpath(m_config, fn);
            } else {
                for (const auto& pat : m_pats) {
                    if (fnmatch(pat.c_str(), fn.c_str(), 0) == 0) {
                        processpath(m_config, fn);
                        break;
                    }
                }
            }
        }
        return FsTreeWalker::FtwOk;
    }
    list<string>& m_files;
    const vector<string>& m_pats;
    RclConfig *m_config{nullptr};
};

bool recursive_grep(RclConfig *config, const string& top, const vector<string>& selpats)
{
    list<string> files;
    WalkerCB cb(files, selpats, config);
    FsTreeWalker walker;
    current_topdir = top;
    if (path_isdir(top)) {
        path_catslash(current_topdir);
    }
    walker.walk(top, cb);
    return true;
}

bool processpaths(RclConfig *config, const std::vector<std::string> &_paths,
                  const std::vector<std::string>& selpats)
{
    if (_paths.empty())
        return true;
    std::vector<std::string> paths;
    std::string origcwd = config->getOrigCwd();
    for (const auto& path : _paths) {
        paths.push_back(path_canon(path, &origcwd));
    }
    std::sort(paths.begin(), paths.end());
    auto uit = std::unique(paths.begin(), paths.end());
    paths.resize(uit - paths.begin());

    for (const auto& path : paths) {
        LOGDEB("processpaths: " << path << "\n");
        if (path_isdir(path)) {
            recursive_grep(config, path, selpats);
        } else {
            if (!path_readable(path)) {
                std::cerr << "Can't read: " << path << "\n";
                continue;
            }
            processpath(config, path);
        }
    }
    return true;
}


std::string thisprog;

static const char usage [] =
"\n"
"rclgrep [--help] \n"
"    Print help\n"
"rclgrep [-f] [<path [path ...]>]\n"
"    Search files.\n"
"    -c <configdir> : specify config directory, overriding $RECOLL_CONFDIR\n"
"  -e PATTERNS, --regexp=PATTERNS patterns to search for. Can be given multiple times\n"
;

static void Usage(FILE* fp = stdout)
{
    fprintf(fp, "%s: Usage: %s", path_getsimple(thisprog).c_str(), usage);
    exit(1);
}

static void add_expressions(const std::string& exps)
{
    std::vector<std::string> vexps;
    stringToTokens(exps, vexps, "\n");
    for (const auto& pattern : vexps) {
        if (op_flags & OPT_x) {
            auto newpat = std::string("^(") + pattern + ")$";
            g_expressions.push_back(new SimpleRegexp(newpat, g_reflags));
        } else {            
            g_expressions.push_back(new SimpleRegexp(pattern, g_reflags));
        }
    }
}

std::vector<std::string> g_expstrings;
static void buildexps()
{
    for (const auto& s : g_expstrings)
        add_expressions(s);
}

static void exps_from_file(const std::string& fn)
{
    std::string data;
    std::string reason;
    if (!file_to_string(fn, data, -1, -1, &reason)) {
        std::cerr << "Could not read " << fn << " : " << reason << "\n";
        exit(1);
    }
    g_expstrings.push_back(data);
}

int main(int argc, char *argv[])
{
    int ret;
    std::string a_config;
    vector<string> selpatterns;
    
    while ((ret = getopt_long(argc, argv, "A:B:C:ce:f:hHiLlnp:vx", long_options, NULL)) != -1) {
        switch (ret) {
        case 'A': op_flags |= OPT_A; aftercontext = atoi(optarg); break;
        case 'B': op_flags |= OPT_B; beforecontext = atoi(optarg); break;
        case 'C': op_flags |= OPT_C; aftercontext = beforecontext = atoi(optarg); break;
        case 'c': op_flags |= OPT_c; break;
        case 'e': op_flags |= OPT_e; g_expstrings.push_back(optarg); break;
        case 'f': op_flags |= OPT_f; exps_from_file(optarg);break;
        case 'h': op_flags |= OPT_h; break;
        case 'H': op_flags |= OPT_H; break;
        case 'i': op_flags |= OPT_i; g_reflags |= SimpleRegexp::SRE_ICASE; break;
        case 'L': op_flags |= OPT_L|OPT_c; break;
        case 'l': op_flags |= OPT_l|OPT_c; break;
        case 'n': op_flags |= OPT_n; break;
        case 'p': op_flags |= OPT_p; selpatterns.push_back(optarg); break;
        case 'v': op_flags |= OPT_v; break;
        case 'x': op_flags |= OPT_x; break;
        case OPTVAL_RECOLL_CONFIG: a_config = optarg; break;
        case OPTVAL_HELP: Usage(stdout); break;
        default: Usage(); break;
        }
    }
    int aremain = argc - optind;

    if (!(op_flags & (OPT_e|OPT_f))) {
        if (aremain == 0)
            Usage();
        std::string patterns =  argv[optind++];
        aremain--;
        g_expstrings.push_back(patterns);
    }

    buildexps();

    // If there are more than 1 file args and -h was not used, we want to print file names.
    if ((aremain > 1 || (aremain == 1 && path_isdir(argv[optind]))) && !(op_flags & OPT_h)) {
        op_flags |= OPT_H;
    }
    
    string reason;
    int flags = 0;
    config = recollinit(flags, nullptr, nullptr, reason, &a_config);
    if (config == 0 || !config->ok()) {
        std::cerr << "Configuration problem: " << reason << "\n";
        exit(1);
    }

    // Get rid of log messages
    Logger::getTheLog()->setLogLevel(Logger::LLFAT);

    orig_cwd = path_cwd();
    string rundir;
    config->getConfParam("idxrundir", rundir);
    if (!rundir.empty()) {
        if (!rundir.compare("tmp")) {
            rundir = tmplocation();
        }
        LOGDEB2("rclgrep: changing current directory to [" << rundir << "]\n");
        if (!path_chdir(rundir)) {
            LOGSYSERR("main", "chdir", rundir);
        }
    }
    std::vector<std::string> paths;
    if (aremain == 0) {
        // Read from stdin
    } else {
        while (aremain--) {
            paths.push_back(argv[optind++]);
        }
    }

    bool status = processpaths(config, paths, selpatterns);
    return status ? 0 : 1;
}
