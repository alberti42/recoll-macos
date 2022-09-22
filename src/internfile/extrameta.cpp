/* Copyright (C) 2004-2022 J.F.Dockes 
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

#include <errno.h>

#include "rclconfig.h"
#include "pxattr.h"
#include "log.h"
#include "cstr.h"
#include "rcldoc.h"
#include "execmd.h"

using std::string;
using std::map;

static void docfieldfrommeta(RclConfig* cfg, const string& name, 
                             const string &value, Rcl::Doc& doc)
{
    string fieldname = cfg->fieldCanon(name);
    LOGDEB0("Internfile:: setting [" << fieldname << "] from cmd/xattr value [" << value << "]\n");
    if (fieldname == cstr_dj_keymd) {
        doc.dmtime = value;
    } else {
        doc.meta[fieldname] = value;
    }
}

void reapXAttrs(const RclConfig* cfg, const string& path,  map<string, string>& xfields)
{
    LOGDEB2("reapXAttrs: [" << path << "]\n");
#ifndef _WIN32
    // Retrieve xattrs names from files and mapping table from config
    vector<string> xnames;
    if (!pxattr::list(path, &xnames)) {
        if (errno == ENOTSUP) {
            LOGDEB("FileInterner::reapXattrs: pxattr::list: errno " << errno << "\n");
        } else {
            LOGSYSERR("FileInterner::reapXattrs", "pxattr::list", path);
        }
        return;
    }
    const map<string, string>& xtof = cfg->getXattrToField();

    // Record the xattrs: names found in the config are either skipped
    // or mapped depending if the translation is empty. Other names
    // are recorded as-is
    for (const auto& xkey : xnames) {
        string key = xkey;
        auto mit = xtof.find(xkey);
        if (mit != xtof.end()) {
            if (mit->second.empty()) {
                continue;
            } else {
                key = mit->second;
            }
        }
        string value;
        if (!pxattr::get(path, xkey, &value, pxattr::PXATTR_NOFOLLOW)) {
            LOGSYSERR("FileInterner::reapXattrs", "pxattr::get", path + " : " + xkey);
            continue;
        }
        // Encode should we ?
        xfields[key] = value;
        LOGDEB2("reapXAttrs: [" << key << "] -> [" << value << "]\n");
    }
#else
    PRETEND_USE(cfg);
    PRETEND_USE(path);
    PRETEND_USE(xfields);
#endif
}

void docFieldsFromXattrs(RclConfig *cfg, const map<string, string>& xfields, Rcl::Doc& doc)
{
    for (const auto& fld : xfields) {
        docfieldfrommeta(cfg, fld.first, fld.second, doc);
    }
}

void reapMetaCmds(RclConfig* cfg, const string& path, map<string, string>& cfields)
{
    const auto& reapers = cfg->getMDReapers();
    if (reapers.empty())
        return;
    map<char,string> smap = {{'f', path}};
    for (const auto& reaper : reapers) {
        vector<string> cmd;
        for (const auto& arg : reaper.cmdv) {
            string s;
            pcSubst(arg, s, smap);
            cmd.push_back(s);
        }
        string output;
        if (ExecCmd::backtick(cmd, output)) {
            cfields[reaper.fieldname] =  output;
        }
    }
}

// Set fields from external commands
// These override those from xattrs and can be later augmented by
// values from inside the file.
//
// This is a bit atrocious because some entry names are special:
// "modificationdate" will set mtime instead of an ordinary field,
// and the output from anything beginning with "rclmulti" will be
// interpreted as multiple fields in configuration file format...
void docFieldsFromMetaCmds(RclConfig *cfg, const map<string, string>& cfields, Rcl::Doc& doc)
{
    for (const auto& cfld : cfields) {
        if (!cfld.first.compare(0, 8, "rclmulti")) {
            ConfSimple simple(cfld.second);
            if (simple.ok()) {
                auto names = simple.getNames("");
                for (const auto& nm : names) {
                    string value;
                    if (simple.get(nm, value)) {
                        docfieldfrommeta(cfg, nm, value, doc);
                    }
                }
            }
        } else {
            docfieldfrommeta(cfg, cfld.first, cfld.second, doc);
        }
    }
}

