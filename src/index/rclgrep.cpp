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
#include "indexer.h"
#include "smallut.h"
#include "pathut.h"
#include "rclutil.h"
#include "cancelcheck.h"
#include "execmd.h"
#include "internfile.h"
#include "rcldoc.h"
#include "fstreewalk.h"

// Command line options
static int     op_flags;
#define OPT_c 0x2
#define OPT_f 0x40    
#define OPT_h 0x80    
#define OPT_i 0x200   
#define OPT_p 0x10000 

static struct option long_options[] = {
    {0, 0, 0, 0}
};

SimpleRegexp *exp_p;

void grepit(const Rcl::Doc& doc)
{
    std::vector<std::string> lines;    
    stringToTokens(doc.text, lines, "\n");
    for (const auto& line: lines) {
        //std::cout << "LINE:[" << line << "]\n";
        if (exp_p->simpleMatch(line)) {
            std::cout << fileurltolocalpath(doc.url) << ":" << doc.ipath << "::" << line << "\n";
        }
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
"rclgrep [-h] \n"
"    Print help\n"
"rclgrep [-f] [<path [path ...]>]\n"
"    Index individual files. No database purge or stem database updates\n"
"    Will read paths on stdin if none is given as argument\n"
"    -f : ignore skippedPaths and skippedNames while doing this\n"
"Common options:\n"
"    -c <configdir> : specify config directory, overriding $RECOLL_CONFDIR\n"
;

static void Usage()
{
    FILE *fp = (op_flags & OPT_h) ? stdout : stderr;
    fprintf(fp, "%s: Usage: %s", path_getsimple(thisprog).c_str(), usage);
    fprintf(fp, "Recoll version: %s\n", Rcl::version_string().c_str());
    exit((op_flags & OPT_h)==0);
}

static RclConfig *config;

// Working directory before we change: it's simpler to change early
// but some options need the original for computing absolute paths.
static std::string orig_cwd;

int main(int argc, char *argv[])
{
    int ret;
    std::string a_config;
    vector<string> selpatterns;
    int reflags = SimpleRegexp::SRE_NOSUB;
    
    while ((ret = getopt_long(argc, argv, "c:fhip:", long_options, NULL)) != -1) {
        switch (ret) {
        case 'c':  op_flags |= OPT_c; a_config = optarg; break;
        case 'f': op_flags |= OPT_f; break;
        case 'h': op_flags |= OPT_h; break;
        case 'i': op_flags |= OPT_i; reflags |= SimpleRegexp::SRE_ICASE; break;
        case 'p': op_flags |= OPT_p; selpatterns.push_back(optarg); break;
        default: Usage(); break;
        }
    }
    int aremain = argc - optind;
    if (op_flags & OPT_h)
        Usage();
    if (aremain == 0)
        Usage();
    std::string pattern =  argv[optind++];
    aremain--;

    exp_p = new SimpleRegexp(pattern, reflags);
    
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
        char line[1024];
        while (fgets(line, 1023, stdin)) {
            string sl(line);
            trimstring(sl, "\n\r");
            paths.push_back(sl);
        }
    } else {
        while (aremain--) {
            paths.push_back(argv[optind++]);
        }
    }

    bool status = processpaths(config, paths, selpatterns);
    return status ? 0 : 1;
}
