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

#include <conftree.h>
#include <fstreewalk.h>

#include <iostream>
using namespace std;

#include "pathut.h"
#include "smallut.h"
#include "appformime.h"

static const string topappsdir("/usr/share/applications");
static const string desktopext("desktop");

static DesktopDb *theDb;

class FstCb : public FsTreeWalkerCB {
public:
    FstCb(DesktopDb::AppMap *appdefs)
        : m_appdefs(appdefs) {}
    virtual FsTreeWalker::Status 
    processone(const string &, const struct PathStat *, FsTreeWalker::CbFlag);
    DesktopDb::AppMap *m_appdefs;
};

FsTreeWalker::Status FstCb::processone(
    const string& fn, const struct PathStat*, FsTreeWalker::CbFlag flg) 
{
    if (flg != FsTreeWalker::FtwRegular)
        return FsTreeWalker::FtwOk;

    if (path_suffix(fn).compare(desktopext)) {
        //cerr << fn << " does not end with .desktop" << endl;
        return FsTreeWalker::FtwOk;
    }

    ConfSimple dt(fn.c_str(), true);
    if (!dt.ok()) {
        cerr << fn << " cant parse" << endl;
        return FsTreeWalker::FtwOk;
    }
    string tp, nm, cmd, mt;
    if (!dt.get("Type", tp, "Desktop Entry")) {
        //cerr << fn << " no Type" << endl;
        return FsTreeWalker::FtwOk;
    }
    if (tp.compare("Application")) {
        //cerr << fn << " wrong Type " << tp << endl;
        return FsTreeWalker::FtwOk;
    }
    if (!dt.get("Exec", cmd, "Desktop Entry")) {
        //cerr << fn << " no Exec" << endl;
        return FsTreeWalker::FtwOk;
    }
    if (!dt.get("Name", nm, "Desktop Entry")) {
        //cerr << fn << " no Name" << endl;
        nm = path_basename(fn, desktopext);
    }
    if (!dt.get("MimeType", mt, "Desktop Entry")) {
        //cerr << fn << " no MimeType" << endl;
        return FsTreeWalker::FtwOk;
    }
    DesktopDb::AppDef appdef(nm, cmd);
    // Breakup mime type list, and push app to mime entries
    vector<string> mimes;
    stringToTokens(mt, mimes, ";");
    for (vector<string>::const_iterator it = mimes.begin();
         it != mimes.end(); it++) {
        (*m_appdefs)[*it].push_back(appdef);
    }
    return FsTreeWalker::FtwOk;
}

DesktopDb* DesktopDb::getDb()
{
    if (nullptr == theDb) {
        theDb = new DesktopDb();
    }
    if (theDb && theDb->m_ok)
        return theDb;
    return nullptr;
}

void DesktopDb::build(const string& dir)
{
    FstCb procapp(&m_appMap);
    FsTreeWalker walker;
    if (walker.walk(dir, procapp) != FsTreeWalker::FtwOk) {
        m_ok = false;
        m_reason = walker.getReason();
    }
    m_ok = true;
}

DesktopDb::DesktopDb()
{
    build(topappsdir);
}

DesktopDb::DesktopDb(const string& dir)
{
    build(dir);
}

bool DesktopDb::appForMime(const string& mime, vector<AppDef> *apps, 
                           string *reason)
{
    AppMap::const_iterator it = m_appMap.find(mime);
    if (it == m_appMap.end()) {
        if (reason)
            *reason = string("No application found for ") + mime;
        return false;
    }
    *apps = it->second;
    return true;
}

bool DesktopDb::allApps(vector<AppDef> *apps)
{
    map<string, AppDef> allaps;
    for (AppMap::const_iterator it = m_appMap.begin();
         it != m_appMap.end(); it++) {
        for (vector<AppDef>::const_iterator it1 = it->second.begin();
             it1 != it->second.end(); it1++) {
            allaps.insert(pair<string, AppDef>
                          (it1->name, AppDef(it1->name, it1->command)));
        }
    }
    for (map<string, AppDef>::const_iterator it = allaps.begin();
         it != allaps.end(); it++) {
        apps->push_back(it->second);
    }
    return true;
}

bool DesktopDb::appByName(const string& nm, AppDef& app)
{
    for (AppMap::const_iterator it = m_appMap.begin();
         it != m_appMap.end(); it++) {
        for (vector<AppDef>::const_iterator it1 = it->second.begin();
             it1 != it->second.end(); it1++) {
            if (!nm.compare(it1->name)) {
                app.name = it1->name;
                app.command = it1->command;
                return true;
            }
        }
    }
    return false;
}

const string& DesktopDb::getReason()
{
    return m_reason;
}

bool mimeIsImage(const std::string& tp)
{
    return !tp.compare(0, 6, "image/") &&
        tp.compare("image/vnd.djvu") && tp.compare("image/svg+xml");
}
