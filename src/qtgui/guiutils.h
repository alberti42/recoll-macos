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
#ifndef _GUIUTILS_H_INCLUDED_
#define _GUIUTILS_H_INCLUDED_
/* 
 * @(#$Id: guiutils.h,v 1.29 2008-10-03 08:09:35 dockes Exp $  (C) 2005 Jean-Francois Dockes 
 *                         jean-francois.dockes@wanadoo.fr
 *
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

#include <string>
#include <list>
#include <vector>

#include <qstring.h>
#include <qstringlist.h>

#ifndef NO_NAMESPACES
using std::string;
using std::list;
using std::vector;
#endif

/** Retrieve configured stemming languages */
bool getStemLangs(list<string>& langs);

/** Start a browser on the help document */
extern bool startHelpBrowser(const string& url = "");


/** Holder for preferences (gets saved to user Qt prefs) */
class PrefsPack {
 public:
    bool autoSearchOnWS;
    int respagesize;
    int maxhltextmbs;
    QString reslistfontfamily;
    QString qtermcolor; // Color for query terms in reslist and preview
    int reslistfontsize;
    // Result list format string
    QString reslistformat;
    string  creslistformat;
    QString queryStemLang;
    int mainwidth;
    int mainheight;
    int pvwidth; // Preview window geom
    int pvheight;
    int ssearchTyp;
    QString htmlBrowser;
    bool    useDesktopOpen; // typically xdg-open, instead of mimeview settings
    bool    keepSort; // remember sort status between invocations
    bool    sortActive; // Remembered sort state.
    bool queryBuildAbstract;
    bool queryReplaceAbstract;
    bool startWithAdvSearchOpen;
    bool startWithSortToolOpen;
    bool previewHtml;
    bool collapseDuplicates;
    // Extra query indexes. This are encoded to base64 before storing
    // to the qt settings file to avoid any bin string/ charset conv issues
    list<string> allExtraDbs;
    list<string> activeExtraDbs;
    // Advanced search subdir restriction: we don't activate the last value
    // but just remember previously entered values
    QStringList asearchSubdirHist;
    // Textual history of simple searches (this is just the combobox list)
    QStringList ssearchHistory;
    // Make phrase out of search terms and add to search in simple search
    bool ssearchAutoPhrase;
    // Ignored file types in adv search (startup default)
    QStringList asearchIgnFilTyps;
    bool        fileTypesByCats;

    // Synthetized abstract length and word context size
    int syntAbsLen;
    int syntAbsCtx;

    // Sort specs (sort_w.cpp knows how to deal with the values
    int sortDepth;
    int sortSpec;

    // Remembered term match mode
    int termMatchType;

    // Program version that wrote this
    int rclVersion;

    // Advanced search window clause list state
    vector<int> advSearchClauses;

    PrefsPack() :
	respagesize(8), 
	reslistfontsize(10),
	ssearchTyp(0),
	queryBuildAbstract(true),
	queryReplaceAbstract(false),
	startWithAdvSearchOpen(false),
	startWithSortToolOpen(false),
	termMatchType(0),
	rclVersion(1009)
	    {
	    }
    static const char *getDfltResListFormat() {
	return 	"<img src=\"%I\" align=\"left\">"
	"%R %S %L &nbsp;&nbsp;<b>%T</b><br>"
	"%M&nbsp;%D&nbsp;&nbsp;&nbsp;<i>%U</i><br>"
	"%A %K";
    }
    static const char *getV18DfltResListFormat() {
	return 	"%R %S %L &nbsp;&nbsp;<b>%T</b><br>"
	"%M&nbsp;%D&nbsp;&nbsp;&nbsp;<i>%U</i><br>"
	"%A %K";
    }
};

/** Global preferences record */
extern PrefsPack prefs;

/** Read write settings from disk file */
extern void rwSettings(bool dowrite);

extern QString g_stringAllStem, g_stringNoStem;

#endif /* _GUIUTILS_H_INCLUDED_ */
