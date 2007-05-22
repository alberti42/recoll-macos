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
#include "mh_html.h"
#include "smallut.h"

#include <iostream>
#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */


bool MimeHandlerHtml::set_document_file(const string &fn)
{
    LOGDEB(("textHtmlToDoc: %s\n", fn.c_str()));
    string otext;
    if (!file_to_string(fn, otext)) {
	LOGINFO(("textHtmlToDoc: cant read: %s\n", fn.c_str()));
	return false;
    }
    return set_document_string(otext);
}

bool MimeHandlerHtml::set_document_string(const string& htext) 
{
    m_html = htext;
    m_havedoc = true;
    return true;
}

bool MimeHandlerHtml::next_document()
{
    if (m_havedoc == false)
	return false;
    m_havedoc = false;
    string charset = m_defcharset;
    LOGDEB(("textHtmlToDoc: next_document. defcharset: %s\n",charset.c_str()));

    // - We first try to convert from the default configured charset
    //   (which may depend of the current directory) to utf-8. If this
    //   fails, we keep the original text
    // - During parsing, if we find a charset parameter, and it differs from
    //   what we started with, we abort and restart with the parameter value
    //   instead of the configuration one.
    LOGDEB(("textHtmlToDoc: charset before parsing: [%s]\n", charset.c_str()));


    MyHtmlParser result;
    for (int pass = 0; pass < 2; pass++) {
	string transcoded;
	LOGDEB(("Html::mkDoc: pass %d\n", pass));
	MyHtmlParser p;
	// Try transcoding. If it fails, use original text.
	if (!transcode(m_html, transcoded, charset, "UTF-8")) {
	    LOGERR(("textHtmlToDoc: transcode failed from cs '%s' to UTF-8\n",
		    charset.c_str()));
	    transcoded = m_html;
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
	    // No exception: ok?
	    result = p;
	    break;
	} catch (bool diag) {
	    result = p;
	    if (diag == true)
		break;
	    LOGDEB(("textHtmlToDoc: charset [%s] doc charset [%s]\n",
		    charset.c_str(),result.doccharset.c_str()));
	    if (!result.doccharset.empty() && 
		!samecharset(result.doccharset, result.ocharset)) {
		LOGDEB(("textHtmlToDoc: reparse for charsets\n"));
		charset = result.doccharset;
	    } else {
		LOGERR(("textHtmlToDoc:: error: non charset exception\n"));
		return false;
	    }
	}
    }

    m_metaData["origcharset"] = m_defcharset;
    m_metaData["content"] = result.dump;
    m_metaData["charset"] = "utf-8";
    m_metaData["title"] = result.title;
    m_metaData["keywords"] = result.keywords;
    // Avoid setting empty values which would crush ones possibly inherited
    // from parent (if we're an attachment)
    if (!result.author.empty())
	m_metaData["author"] = result.author;
    if (!result.dmtime.empty())
	m_metaData["modificationdate"] = result.dmtime;
    m_metaData["sample"] = result.sample;
    m_metaData["mimetype"] = "text/plain";
    return true;
}
