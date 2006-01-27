#ifndef _GUIUTILS_H_INCLUDED_
#define _GUIUTILS_H_INCLUDED_
/* 
 * @(#$Id: guiutils.h,v 1.1 2006-01-27 13:43:04 dockes Exp $  (C) 2005 Jean-Francois Dockes 
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
#include <qstring.h>

/** Start a browser on the help document */
extern bool startHelpBrowser(const string& url = "");

/** Holder for preferences (gets saved to user Qt prefs) */
class PrefsPack {
 public:
    bool showicons;
    int respagesize;
    QString reslistfontfamily;
    int reslistfontsize;
    QString queryStemLang;
    int mainwidth;
    int mainheight;
    bool ssall;
    QString htmlBrowser;
    bool queryBuildAbstract;
    bool queryReplaceAbstract;
    PrefsPack() :
	showicons(true), 
	respagesize(8), 
	reslistfontsize(10),
	ssall(false),
	queryBuildAbstract(true),
	queryReplaceAbstract(false)
	    {
	    }
};

/** Global preferences record */
extern PrefsPack prefs;

/** Read write settings from disk file */
extern void rwSettings(bool dowrite);

#endif /* _GUIUTILS_H_INCLUDED_ */
