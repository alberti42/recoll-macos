#ifndef lint
static char rcsid[] = "@(#$Id: mimehandler.cpp,v 1.17 2006-03-20 16:05:41 dockes Exp $ (C) 2004 J.F.Dockes";
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
#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

#include "mimehandler.h"
#include "debuglog.h"
#include "smallut.h"
#include "mh_html.h"
#include "mh_mail.h"
#include "mh_text.h"
#include "mh_exec.h"
#include "mh_unknown.h"
  
/** Create internal handler object appropriate for given mime type */
static MimeHandler *mhFactory(const string &mime)
{
    if (!stringlowercmp("text/plain", mime))
	return new MimeHandlerText;
    else if (!stringlowercmp("text/html", mime))
	return new MimeHandlerHtml;
    else if (!stringlowercmp("text/x-mail", mime))
	return new MimeHandlerMail;
    else if (!stringlowercmp("message/rfc822", mime))
	return new MimeHandlerMail;
    return 0;
}

/**
 * Return handler object for given mime type:
 */
MimeHandler *getMimeHandler(const string &mtype, RclConfig *cfg)
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
	if (!stringlowercmp("internal", toks.front())) {
	    return mhFactory(mtype);
	} else if (!stringlowercmp("dll", toks.front())) {
	} else if (!stringlowercmp("exec", toks.front())) {
	    if (toks.size() < 2) {
		LOGERR(("getMimeHandler: bad line for %s: %s\n", 
			mtype.c_str(), hs.c_str()));
		return 0;
	    }
	    MimeHandlerExec *h = new MimeHandlerExec;
	    list<string>::const_iterator it1 = toks.begin();
	    it1++;
	    for (;it1 != toks.end();it1++)
		h->params.push_back(*it1);
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
	return new MimeHandlerUnknown;
    } else {
	return 0;
    }
}
