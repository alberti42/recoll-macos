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
#ifndef TEST_APPFORMIME
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

typedef map<string, vector<DesktopDb::AppDef> > AppMap;
static AppMap theAppMap;

static std::string o_reason;
static bool o_ok;

class FstCb : public FsTreeWalkerCB {
public:
    FstCb(AppMap *appdefs)
        : m_appdefs(appdefs)
        {
        }
    virtual FsTreeWalker::Status 
    processone(const string &, const struct stat *, FsTreeWalker::CbFlag);
    AppMap *m_appdefs;
};

FsTreeWalker::Status FstCb::processone(const string& fn, const struct stat *, 
                                       FsTreeWalker::CbFlag flg) 
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
    if (theDb == 0) {
        theDb = new DesktopDb();
    }
    if (o_ok)
        return theDb;
    return 0;
}

DesktopDb::DesktopDb()
{
    FstCb procapp(&theAppMap);
    FsTreeWalker walker;
    if (walker.walk(topappsdir, procapp) != FsTreeWalker::FtwOk) {
        o_ok = false;
        o_reason = walker.getReason();
    }
    o_ok = true;
}

bool DesktopDb::appForMime(const string& mime, vector<AppDef> *apps, 
                           string *reason)
{
    AppMap::const_iterator it = theAppMap.find(mime);
    if (it == theAppMap.end()) {
        if (reason)
            *reason = string("No application found for ") + mime;
        return false;
    }
    *apps = it->second;
    return true;
}

const string& DesktopDb::getReason()
{
    return o_reason;
}

#else // TEST_APPFORMIME

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <string>
#include <iostream>
#include <vector>
using namespace std;

#include "appformime.h"

static char *thisprog;

static char usage [] =
"  appformime <mime type>\n\n"
;
static void
Usage(void)
{
    fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

int main(int argc, char **argv)
{
  thisprog = argv[0];
  argc--; argv++;

  if (argc != 1)
    Usage();
  string mime = *argv++;argc--;

  string reason;
  vector<DesktopDb::AppDef> appdefs;
  DesktopDb *ddb = DesktopDb::getDb();
  if (ddb == 0) {
      cerr << "Could not initialize desktop db: " << DesktopDb::getReason()
           << endl;
      exit(1);
  }
  if (!ddb->appForMime(mime, &appdefs, &reason)) {
      cerr << "appForMime failed: " << reason << endl;
      exit(1);
  }
  if (appdefs.empty()) {
      cerr << "No application found for [" << mime << "]" << endl;
      exit(1);
  }
  cout << mime << " -> ";
  for (vector<DesktopDb::AppDef>::const_iterator it = appdefs.begin();
       it != appdefs.end(); it++) {
      cout << "[" << it->name << ", " << it->command << "], ";
  }
  cout << endl;

  exit(0);
}

#endif //TEST_APPFORMIME
