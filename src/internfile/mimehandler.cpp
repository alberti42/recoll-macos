#ifndef lint
static char rcsid[] = "@(#$Id: mimehandler.cpp,v 1.21 2006-12-19 08:40:50 dockes Exp $ (C) 2004 J.F.Dockes";
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

#include <iostream>
#include <string>

using namespace std;

#include "mimehandler.h"
#include "debuglog.h"
#include "rclconfig.h"
#include "smallut.h"

#include "mh_exec.h"
#include "mh_html.h"
#include "mh_mail.h"
#include "mh_mbox.h"
#include "mh_text.h"
#include "mh_unknown.h"
  
/** Create internal handler object appropriate for given mime type */
static Dijon::Filter *mhFactory(const string &mime)
{
    string lmime(mime);
    stringtolower(lmime);
    if ("text/plain" == lmime)
	return new MimeHandlerText(lmime);
    else if ("text/html" == lmime)
	return new MimeHandlerHtml(lmime);
    else if ("text/x-mail" == lmime)
	return new MimeHandlerMbox(lmime);
    else if ("message/rfc822" == lmime)
	return new MimeHandlerMail(lmime);
    else 
	return new MimeHandlerUnknown(lmime);
}

/**
 * Return handler object for given mime type:
 */
Dijon::Filter *getMimeHandler(const string &mtype, RclConfig *cfg)
{
    // Get handler definition for mime type
    string hs;
    if (!mtype.empty())
	hs = cfg->getMimeHandlerDef(mtype);

    if (!hs.empty()) {
	// Break definition into type and name 
	list<string> toks;
	stringToStrings(hs, toks);
	if (toks.empty()) {
	    LOGERR(("getMimeHandler: bad mimeconf line for %s\n", 
		    mtype.c_str()));
	    return 0;
	}

	// Retrieve handler function according to type
	list<string>::iterator it = toks.begin();
	if (!stringlowercmp("internal", *it)) {
	    return mhFactory(mtype);
	} else if (!stringlowercmp("dll", *it)) {
	} else if (!stringlowercmp("exec", *it)) {
	    if (toks.size() < 2) {
		LOGERR(("getMimeHandler: bad line for %s: %s\n", 
			mtype.c_str(), hs.c_str()));
		return 0;
	    }
	    MimeHandlerExec *h = new MimeHandlerExec(mtype.c_str());
	    it++;
	    h->params.push_back(cfg->findFilter(*it++));
	    h->params.insert(h->params.end(), it, toks.end());
	    return h;
	}
    }

    // We are supposed to get here if there was no specific error, but
    // there is no identified mime type, or no handler
    // associated. These files are either ignored or their name is
    // indexed, depending on configuration
    bool indexunknown = false;
    cfg->getConfParam("indexallfilenames", &indexunknown);
    if (indexunknown) {
	LOGDEB(("getMimeHandler: returning MimeHandlerUnknown\n"));
	return new MimeHandlerUnknown("application/octet-stream");
    } else {
	return 0;
    }
}


/// Can this mime type be interned (according to config) ?
bool canIntern(const std::string mtype, RclConfig *cfg)
{
    if (mtype.empty())
	return false;
    string hs = cfg->getMimeHandlerDef(mtype);
    if (hs.empty())
	return false;
    return true;
}
