#ifndef lint
static char rcsid[] = "@(#$Id: mimehandler.cpp,v 1.13 2005-11-21 14:31:24 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <iostream>
#include <string>
using namespace std;

#include "mimehandler.h"
#include "debuglog.h"
#include "smallut.h"
#include "mh_html.h"
#include "mh_mail.h"
#include "mh_text.h"
#include "mh_exec.h"
  
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
    string hs = cfg->getMimeHandlerDef(mtype);
    if (hs.empty())
	return 0;

    // Break definition into type and name 
    list<string> toks;
    ConfTree::stringToStrings(hs, toks);
    if (toks.empty()) {
	LOGERR(("getMimeHandler: bad mimeconf line for %s\n", mtype.c_str()));
	return 0;
    }

    // Retrieve handler function according to type
    if (!stringlowercmp("internal", toks.front())) {
	return mhFactory(mtype);
    } else if (!stringlowercmp("dll", toks.front())) {
	return 0;
    } else if (!stringlowercmp("exec", toks.front())) {
	if (toks.size() < 2) {
	    LOGERR(("getMimeHandler: bad line for %s: %s\n", mtype.c_str(),
		    hs.c_str()));
	    return 0;
	}
	MimeHandlerExec *h = new MimeHandlerExec;
	list<string>::const_iterator it1 = toks.begin();
	it1++;
	for (;it1 != toks.end();it1++)
	    h->params.push_back(*it1);
	return h;
    }
    return 0;
}
