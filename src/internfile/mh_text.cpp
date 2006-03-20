#ifndef lint
static char rcsid[] = "@(#$Id: mh_text.cpp,v 1.5 2006-03-20 15:14:08 dockes Exp $ (C) 2005 J.F.Dockes";
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

#include "mh_text.h"
#include "csguess.h"
#include "debuglog.h"
#include "readfile.h"
#include "transcode.h"

// Process a plain text file
MimeHandler::Status MimeHandlerText::mkDoc(RclConfig *conf, const string &fn, 
			     const string &mtype, Rcl::Doc &docout, string&)
{
    string otext;
    if (!file_to_string(fn, otext))
	return MimeHandler::MHError;
	
    // Try to guess charset, then convert to utf-8, and fill document
    // fields The charset guesser really doesnt work well in general
    // and should be avoided (especially for short documents)
    string charset;
    if (conf->getGuessCharset()) {
	charset = csguess(otext, conf->getDefCharset());
    } else
	charset = conf->getDefCharset();

    LOGDEB1(("MimeHandlerText::mkDoc: transcod from %s to utf-8\n", 
	     charset.c_str()));

    string utf8;
    if (!transcode(otext, utf8, charset, "UTF-8")) {
	LOGERR(("MimeHandlerText::mkDoc: transcode to utf-8 failed "
		"for charset [%s]\n", charset.c_str()));
	otext.erase();
	return MimeHandler::MHError;
    }

    docout.origcharset = charset;
    docout.text = utf8;
    return MimeHandler::MHDone;
}
