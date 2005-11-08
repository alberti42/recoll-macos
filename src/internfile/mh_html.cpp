/* htmlparse.cc: simple HTML parser for omega indexer
 *
 * ----START-LICENCE----
 * Copyright 1999,2000,2001 BrightStation PLC
 * Copyright 2001 Ananova Ltd
 * Copyright 2002 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 * -----END-LICENCE-----
 */

// This file has code from omindex + an adaptor function for recoll at the end

#include "mimehandler.h"
#include "debuglog.h"
#include "csguess.h"
#include "readfile.h"
#include "transcode.h"
#include "mimeparse.h"
#include "myhtmlparse.h"
#include "indextext.h"
#include "html.h"

#include <iostream>
using namespace std;


MimeHandler::Status 
MimeHandlerHtml::mkDoc(RclConfig *conf, const string &fn, 
			const string &mtype, Rcl::Doc &docout, string&)
{
    LOGDEB(("textHtmlToDoc: %s\n", fn.c_str()));
    string otext;
    if (!file_to_string(fn, otext)) {
	LOGINFO(("textHtmlToDoc: cant read: %s\n", fn.c_str()));
	return MimeHandler::MHError;
    }
    return mkDoc(conf, fn, otext, mtype, docout);
}

MimeHandler::Status 
MimeHandlerHtml::mkDoc(RclConfig *conf, const string &, 
			 const string& htext,
			 const string &mtype, Rcl::Doc &docout)
{
    //LOGDEB(("textHtmlToDoc: htext: %s\n", htext.c_str()));
    // Character set handling: the initial guessed charset depends on
    // external factors: possible hint (ie mime charset in a mail
    // message), charset guessing, or default configured charset.
    string charset;
    if (!charsethint.empty()) {
	charset = charsethint;
    } else if (conf->getGuessCharset()) {
	charset = csguess(htext, conf->getDefCharset());
    } else
	charset = conf->getDefCharset();


    // - We first try to convert from the default configured charset
    //   (which may depend of the current directory) to utf-8. If this
    //   fails, we keep the original text
    // - During parsing, if we find a charset parameter, and it differs from
    //   what we started with, we abort and restart with the parameter value
    //   instead of the configuration one.
    LOGDEB(("textHtmlToDoc: charset before parsing: [%s]\n", charset.c_str()));

    MyHtmlParser pres;
    for (int pass = 0; pass < 2; pass++) {
	string transcoded;

	MyHtmlParser p;
	// Try transcoding. If it fails, use original text.
	if (!transcode(htext, transcoded, charset, "UTF-8")) {
	    LOGERR(("textHtmlToDoc: transcode failed from cs '%s' to UTF-8\n",
		    charset.c_str()));
	    transcoded = htext;
	    // We don't know the charset, at all
	    p.ocharset = p.charset = charset = "";
	} else {
	    // ocharset has the putative source charset, transcoded is now
	    // in utf-8
	    p.ocharset = charset;
	    p.charset = "utf-8";
	}

	try {
	    p.parse_html(transcoded);
	} catch (bool) {
	    pres = p;
	    if (!pres.doccharset.empty() && 
		pres.doccharset != pres.ocharset) {
		LOGDEB(("textHtmlToDoc: charset '%s' doc charset '%s',"
			"reparse\n", charset.c_str(),pres.doccharset.c_str()));
		charset = pres.doccharset;
	    } else
		break;
	}
    }

    Rcl::Doc out;
    out.origcharset = charset;
    out.text = pres.dump;
    // LOGDEB(("textHtmlToDoc: dump : %s\n", pres.dump.c_str()));
    out.title = pres.title;
    out.keywords = pres.keywords;
    out.abstract = pres.sample;
    docout = out;
    return MimeHandler::MHDone;
}
