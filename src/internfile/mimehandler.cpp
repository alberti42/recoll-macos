#ifndef lint
static char rcsid[] = "@(#$Id: mimehandler.cpp,v 1.25 2008-10-09 09:19:37 dockes Exp $ (C) 2004 J.F.Dockes";
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
#include "autoconfig.h"

#include <errno.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "mimehandler.h"
#include "debuglog.h"
#include "rclconfig.h"
#include "smallut.h"

#include "mh_exec.h"
#include "mh_execm.h"
#include "mh_html.h"
#include "mh_mail.h"
#include "mh_mbox.h"
#include "mh_text.h"
#include "mh_unknown.h"

// Performance help: we use a pool of already known and created
// handlers. There can be several instances for a given mime type
// (think email attachment in email message: 2 rfc822 handlers are
// needed simulteanously)
//
// FIXME: this is not compatible with multiple threads and a potential
// problem in the recoll gui (indexing thread + preview request). A
// simple lock would be enough as handlers are removed from the cache
// while in use and multiple copies are allowed
static multimap<string, Dijon::Filter*>  o_handlers;

/** For mime types set as "internal" in mimeconf: 
  * create appropriate handler object. */
static Dijon::Filter *mhFactory(const string &mime)
{
    string lmime(mime);
    stringtolower(lmime);
    if ("text/plain" == lmime) {
	return new MimeHandlerText(lmime);
    } else if ("text/html" == lmime) {
	return new MimeHandlerHtml(lmime);
    } else if ("text/x-mail" == lmime) {
	return new MimeHandlerMbox(lmime);
    } else if ("message/rfc822" == lmime) {
	return new MimeHandlerMail(lmime);
    } else if (lmime.find("text/") == 0) {
        // Try to handle unknown text/xx as text/plain. This
        // only happen if the text/xx was defined as "internal" in
        // mimeconf, not at random. For programs, for example this
        // allows indexing and previewing as text/plain (no filter
        // exec) but still opening with a specific editor.
        return new MimeHandlerText(lmime); 
    } else {
	// We should not get there. It means that "internal" was set
	// as a handler in mimeconf for a mime type we actually can't
	// handle.
	LOGERR(("mhFactory: mime type [%s] set as internal but unknown\n", 
		lmime.c_str()));
	return new MimeHandlerUnknown(lmime);
    }
}

/**
 * Create a filter that executes an external program or script
 * A filter def can look like:
 *      someprog -v -t " h i j";charset= xx; mimetype=yy
 * A semi-colon list of attr=value pairs may come after the exec spec.
 * This list is treated by replacing semi-colons with newlines and building
 * a confsimple. This is done quite brutally and we don't support having
 * a ';' inside a quoted string for now. Can't see a use for it.
 */
MimeHandlerExec *mhExecFactory(RclConfig *cfg, const string& mtype, string& hs,
                               bool multiple)
{
    ConfSimple attrs;
    string cmdstr;
    if (!cfg->valueSplitAttributes(hs, cmdstr, attrs)) {
	LOGERR(("mhExecFactory: bad config line for [%s]: [%s]\n", 
		mtype.c_str(), hs.c_str()));
        return 0;
    }

    // Split command name and args, and build exec object
    list<string> cmdtoks;
    stringToStrings(cmdstr, cmdtoks);
    if (cmdtoks.empty()) {
	LOGERR(("mhExecFactory: bad config line for [%s]: [%s]\n", 
		mtype.c_str(), hs.c_str()));
	return 0;
    }
    MimeHandlerExec *h = multiple ? 
        new MimeHandlerExecMultiple(mtype.c_str()) :
        new MimeHandlerExec(mtype.c_str());
    list<string>::iterator it = cmdtoks.begin();
    h->params.push_back(cfg->findFilter(*it++));
    h->params.insert(h->params.end(), it, cmdtoks.end());

    // Handle additional attributes. We substitute the semi-colons
    // with newlines and use a ConfSimple
    string value;
    if (attrs.get("charset", value)) 
        h->cfgCharset = stringtolower((const string&)value);
    if (attrs.get("mimetype", value))
        h->cfgMtype = stringtolower((const string&)value);

#if 1
    string scmd;
    for (it = h->params.begin(); it != h->params.end(); it++) {
	scmd += string("[") + *it + "] ";
    }
    LOGDEB(("mhExecFactory:mt [%s] cfgmt [%s] cfgcs [%s] cmd: [%s]\n",
	    mtype.c_str(), h->cfgMtype.c_str(), h->cfgCharset.c_str(),
	    scmd.c_str()));
#endif

    return h;
}

/* Return mime handler to pool */
void returnMimeHandler(Dijon::Filter *handler)
{
    typedef multimap<string, Dijon::Filter*>::value_type value_type;
    if (handler) {
	handler->clear();
	o_handlers.insert(value_type(handler->get_mime_type(), handler));
    }
}

void clearMimeHandlerCache()
{
    typedef multimap<string, Dijon::Filter*>::value_type value_type;
    map<string, Dijon::Filter *>::iterator it;
    for (it = o_handlers.begin(); it != o_handlers.end(); it++) {
	delete it->second;
    }
    o_handlers.clear();
}


/* Get handler/filter object for given mime type: */
Dijon::Filter *getMimeHandler(const string &mtype, RclConfig *cfg, 
			      bool filtertypes)
{
    LOGDEB2(("getMimeHandler: mtype [%s] filtertypes %d\n", 
             mtype.c_str(), filtertypes));

    // Get handler definition for mime type. We do this even if an
    // appropriate handler object may be in the cache (indexed by mime
    // type). This is fast, and necessary to conform to the
    // configuration, (ie: text/html might be filtered out by
    // indexedmimetypes but an html handler could still be in the
    // cache because it was needed by some other interning stack).
    string hs;
    hs = cfg->getMimeHandlerDef(mtype, filtertypes);

    if (!hs.empty()) { // Got a handler definition line

        // Do we already have a handler object in the cache ?
        map<string, Dijon::Filter *>::iterator it = o_handlers.find(mtype);
        if (it != o_handlers.end()) {
            Dijon::Filter *h = it->second;
            o_handlers.erase(it);
            LOGDEB2(("getMimeHandler: found in cache\n"));
            return h;
        }

	// Not in cache. Break definition into type and name/command
        // string and instanciate handler object
        string::size_type b1 = hs.find_first_of(" \t");
        string handlertype = hs.substr(0, b1);
	string cmdstr;
	if (b1 != string::npos) {
	    cmdstr = hs.substr(b1);
            trimstring(cmdstr);
	}
	if (!stringlowercmp("internal", handlertype)) {
	    // If there is a parameter after "internal" it's the mime
	    // type to use. This is so that we can have bogus mime
	    // types like text/x-purple-html-log (for ie: specific
	    // icon) and still use the html filter on them. This is
	    // partly redundant with the localfields/rclaptg, but
	    // better and the latter will probably go away at some
	    // point in the future
	    if (!cmdstr.empty())
		return mhFactory(cmdstr);
	    return mhFactory(mtype);
	} else if (!stringlowercmp("dll", handlertype)) {
	} else {
            if (cmdstr.empty()) {
		LOGERR(("getMimeHandler: bad line for %s: %s\n", 
			mtype.c_str(), hs.c_str()));
		return 0;
	    }
            if (!stringlowercmp("exec", handlertype)) {
                return mhExecFactory(cfg, mtype, cmdstr, false);
            } else if (!stringlowercmp("execm", handlertype)) {
                return mhExecFactory(cfg, mtype, cmdstr, true);
            } else {
		LOGERR(("getMimeHandler: bad line for %s: %s\n", 
			mtype.c_str(), hs.c_str()));
		return 0;
            }
	}
    }

    // We get here if there was no specific error, but there is no
    // identified mime type, or no handler associated.

#ifdef INDEX_UNKNOWN_TEXT_AS_PLAIN
    // If the type is an unknown text/xxx, index as text/plain and
    // hope for the best (this wouldn't work too well with text/rtf...)
    if (mtype.find("text/") == 0) {
        return mhFactory("text/plain");
    }
#endif

    // Finally, unhandled files are either ignored or their name and
    // generic metadata is indexed, depending on configuration
    bool indexunknown = false;
    cfg->getConfParam("indexallfilenames", &indexunknown);
    if (indexunknown) {
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
