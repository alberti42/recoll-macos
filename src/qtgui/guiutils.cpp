#ifndef lint
static char rcsid[] = "@(#$Id: guiutils.cpp,v 1.1 2006-01-27 13:43:04 dockes Exp $ (C) 2005 Jean-Francois Dockes";
#endif
/*
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

#include "debuglog.h"
#include "smallut.h"
#include "recoll.h"
#include "guiutils.h"
#include "pathut.h"

#include <qsettings.h>

const static char *htmlbrowserlist = 
    "opera konqueror firefox mozilla netscape";

// Search for and launch an html browser for the documentation. If the
// user has set a preference, we use it directly instead of guessing.
bool startHelpBrowser(const string &iurl)
{
    string url;
    if (iurl.empty()) {
	url = path_cat(recoll_datadir, "doc");
	url = path_cat(url, "usermanual.html");
	url = string("file://") + url;
    } else
	url = iurl;


    // If the user set a preference with an absolute path then things are
    // simple
    if (!prefs.htmlBrowser.isEmpty() && prefs.htmlBrowser.find('/') != -1) {
	if (access(prefs.htmlBrowser.ascii(), X_OK) != 0) {
	    LOGERR(("startHelpBrowser: %s not found or not executable\n",
		    prefs.htmlBrowser.ascii()));
	}
	string cmd = string(prefs.htmlBrowser.ascii()) + " " + url + "&";
	if (system(cmd.c_str()) == 0)
	    return true;
	else 
	    return false;
    }

    string searched;
    if (prefs.htmlBrowser.isEmpty()) {
	searched = htmlbrowserlist;
    } else {
	searched = prefs.htmlBrowser.ascii();
    }
    list<string> blist;
    stringToTokens(searched, blist, " ");

    const char *path = getenv("PATH");
    if (path == 0)
	path = "/bin:/usr/bin:/usr/bin/X11:/usr/X11R6/bin:/usr/local/bin";

    list<string> pathl;
    stringToTokens(path, pathl, ":");
    
    // For each browser name, search path and exec/stop if found
    for (list<string>::const_iterator bit = blist.begin(); 
	 bit != blist.end(); bit++) {
	for (list<string>::const_iterator pit = pathl.begin(); 
	     pit != pathl.end(); pit++) {
	    string exefile = path_cat(*pit, *bit);
	    LOGDEB1(("startHelpBrowser: trying %s\n", exefile.c_str()));
	    if (access(exefile.c_str(), X_OK) == 0) {
		string cmd = exefile + " " + url + "&";
		if (system(cmd.c_str()) == 0) {
		    return true;
		}
	    }
	}
    }

    LOGERR(("startHelpBrowser: no html browser found. Looked for: %s\n", 
	    searched.c_str()));
    return false;
}


///////////////////////// 
// Global variables for user preferences. These are set in the user preference
// dialog and saved restored to the appropriate place in the qt settings
PrefsPack prefs;

#define SETTING_RW(var, nm, tp, def)			\
    if (writing) {					\
	settings.writeEntry(nm , var);			\
    } else {						\
	var = settings.read##tp##Entry(nm, def);	\
    }						

void rwSettings(bool writing)
{
    QSettings settings;
    settings.setPath("Recoll.org", "Recoll");

    SETTING_RW(prefs.mainwidth, "/Recoll/geometry/width", Num, 590);
    SETTING_RW(prefs.mainheight, "/Recoll/geometry/height", Num, 810);
    SETTING_RW(prefs.ssall, "/Recoll/prefs/simpleSearchAll", Bool, false);
    SETTING_RW(prefs.showicons, "/Recoll/prefs/reslist/showicons", Bool, true);
    SETTING_RW(prefs.respagesize, "/Recoll/prefs/reslist/pagelen", Num, 8);
    SETTING_RW(prefs.reslistfontfamily, "/Recoll/prefs/reslist/fontFamily", ,
	       "");
    SETTING_RW(prefs.reslistfontsize, "/Recoll/prefs/reslist/fontSize", Num, 
	       0);
    SETTING_RW(prefs.queryStemLang, "/Recoll/prefs/query/stemLang", ,
	       "english");

    SETTING_RW(prefs.queryBuildAbstract, 
	       "/Recoll/prefs/query/buildAbstract", Bool, true);
    SETTING_RW(prefs.queryReplaceAbstract, 
	       "/Recoll/prefs/query/replaceAbstract", Bool, false);
}


