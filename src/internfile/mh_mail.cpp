#ifndef lint
static char rcsid[] = "@(#$Id: mh_mail.cpp,v 1.24 2006-12-15 12:40:02 dockes Exp $ (C) 2005 J.F.Dockes";
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

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#include <map>
#include <sstream>

#include "mimehandler.h"
#include "readfile.h"
#include "transcode.h"
#include "mimeparse.h"
#include "mh_mail.h"
#include "debuglog.h"
#include "smallut.h"
#include "mh_html.h"

// binc imap mime definitions
#include "mime.h"

using namespace std;

static const int maxdepth = 20;

MimeHandlerMail::~MimeHandlerMail() 
{
    delete m_bincdoc;
    if (m_fd >= 0)
	close(m_fd);
    delete m_stream;
}
bool MimeHandlerMail::set_document_file(const string &fn)
{
    if (m_fd >= 0) {
	close(m_fd);
	m_fd = -1;
    }
    m_fd = open(fn.c_str(), 0);
    if (m_fd < 0) {
	LOGERR(("MimeHandlerMail::set_document_file: open(%s) errno %d\n",
		fn.c_str(), errno));
	return false;
    }
    delete m_bincdoc;
    m_bincdoc = new Binc::MimeDocument;
    m_bincdoc->parseFull(m_fd);
    if (!m_bincdoc->isHeaderParsed() && !m_bincdoc->isAllParsed()) {
	LOGERR(("MimeHandlerMail::mkDoc: mime parse error for %s\n",
		fn.c_str()));
	return false;
    }
    m_havedoc = true;
    return true;
}

bool MimeHandlerMail::set_document_string(const string &msgtxt)
{
    LOGDEB2(("Message text: [%s]\n", msgtxt.c_str()));
    delete m_stream;
    m_stream = new stringstream(msgtxt);
    delete m_bincdoc;
    m_bincdoc = new Binc::MimeDocument;
    m_bincdoc->parseFull(*m_stream);
    if (!m_bincdoc->isHeaderParsed() && !m_bincdoc->isAllParsed()) {
	LOGERR(("MimeHandlerMail::set_document_string: mime parse error\n"));
	return false;
    }
    m_havedoc = true;
    return true;
}

bool MimeHandlerMail::next_document()
{
    if (!m_havedoc)
	return false;
    m_havedoc = false;
    m_metaData["mimetype"] = "text/plain";
    return processMsg(m_bincdoc, 0);
}

// Transform a single message into a document. The subject becomes the
// title, and any simple body part with a content-type of text or html
// and content-disposition inline gets concatenated as text.
// 
// If depth is not zero, we're called recursively for an
// message/rfc822 part and we must not touch the doc fields except the
// text
bool MimeHandlerMail::processMsg(Binc::MimePart *doc, int depth)
{
    LOGDEB2(("MimeHandlerMail::processMsg: depth %d\n", depth));
    if (depth++ >= maxdepth) {
	// Have to stop somewhere
	LOGDEB(("MimeHandlerMail::processMsg: maxdepth %d exceeded\n", 
		maxdepth));
	// Return true anyway, better to index partially than not at all
	return true;
    }
	
    // Handle some headers. 
    string& text = m_metaData["content"];
    Binc::HeaderItem hi;
    string transcoded;
    if (doc->h.getFirstHeader("From", hi)) {
	rfc2047_decode(hi.getValue(), transcoded);
	text += string("From: ") + transcoded + string("\n");
    }
    if (doc->h.getFirstHeader("To", hi)) {
	rfc2047_decode(hi.getValue(), transcoded);
	text += string("To: ") + transcoded + string("\n");
    }
    if (doc->h.getFirstHeader("Date", hi)) {
	rfc2047_decode(hi.getValue(), transcoded);
	if (depth == 1) {
	    time_t t = rfc2822DateToUxTime(transcoded);
	    if (t != (time_t)-1) {
		char ascuxtime[100];
		sprintf(ascuxtime, "%ld", (long)t);
		m_metaData["modificationdate"] = ascuxtime;
	    } else {
		// Leave mtime field alone, ftime will be used instead.
		LOGDEB(("rfc2822Date...: failed: [%s]\n", transcoded.c_str()));
	    }
	}
	text += string("Date: ") + transcoded + string("\n");
    }
    if (doc->h.getFirstHeader("Subject", hi)) {
	rfc2047_decode(hi.getValue(), transcoded);
	if (depth == 1)
	    m_metaData["title"] = transcoded;
	text += string("Subject: ") + transcoded + string("\n");
    }
    text += '\n';

    LOGDEB2(("MimeHandlerMail::rocessMsg:ismultipart %d mime subtype '%s'\n",
	    doc->isMultipart(), doc->getSubType().c_str()));
    walkmime(doc, depth);

    LOGDEB2(("MimeHandlerMail::processMsg:text:[%s]\n", 
	    m_metaData["content"].c_str()));
    return true;
}

// Recursively walk the message mime parts and concatenate all the
// inline html or text that we find anywhere.  
//
// RFC2046 reminder: 
// Top level media types: 
//      Simple:    text, image, audio, video, application, 
//      Composite: multipart, message.
// 
// multipart can be mixed, alternative, parallel, digest.
// message/rfc822 may also be of interest.
void MimeHandlerMail::walkmime(Binc::MimePart* doc, int depth)
{
    LOGDEB2(("MimeHandlerMail::walkmime: depth %d\n", depth));
    if (depth++ >= maxdepth) {
	LOGINFO(("walkmime: max depth (%d) exceeded\n", maxdepth));
	return;
    }

    string& out = m_metaData["content"];

    if (doc->isMultipart()) {
	LOGDEB2(("walkmime: ismultipart %d subtype '%s'\n", 
		doc->isMultipart(), doc->getSubType().c_str()));
	// We only handle alternative, related and mixed (no digests). 
	std::vector<Binc::MimePart>::iterator it;

	if (!stringicmp("mixed", doc->getSubType()) || 
	    !stringicmp("related", doc->getSubType())) {
	    // Multipart mixed and related:  process each part.
	    for (it = doc->members.begin(); it != doc->members.end();it++) {
		walkmime(&(*it), depth);
	    }

	} else if (!stringicmp("alternative", doc->getSubType())) {
	    // Multipart/alternative: look for a text/plain part, then html.
	    // Process if found
	    std::vector<Binc::MimePart>::iterator ittxt, ithtml;
	    ittxt = ithtml = doc->members.end();
	    int i = 1;
	    for (it = doc->members.begin(); 
		 it != doc->members.end(); it++, i++) {
		// Get and parse content-type header
		Binc::HeaderItem hi;
		if (!it->h.getFirstHeader("Content-Type", hi)) {
		    LOGDEB(("No content-type header for part %d\n", i));
		    continue;
		}
		MimeHeaderValue content_type;
		parseMimeHeaderValue(hi.getValue(), content_type);
		LOGDEB2(("walkmime: C-type: %s\n",content_type.value.c_str()));
		if (!stringlowercmp("text/plain", content_type.value))
		    ittxt = it;
		else if (!stringlowercmp("text/html", content_type.value)) 
		    ithtml = it;
	    }
	    if (ittxt != doc->members.end()) {
		LOGDEB2(("walkmime: alternative: chose text/plain part\n"))
		    walkmime(&(*ittxt), depth);
	    } else if (ithtml != doc->members.end()) {
		LOGDEB2(("walkmime: alternative: chose text/html part\n"))
		    walkmime(&(*ithtml), depth);
	    }
	}
	return;
    } 
    
    // Part is not multipart: it must be either simple or message. Take
    // a look at interesting headers and a possible filename parameter

    // Get and parse content-type header.
    Binc::HeaderItem hi;
    string ctt = "text/plain";
    if (doc->h.getFirstHeader("Content-Type", hi)) {
	ctt = hi.getValue();
    }
    LOGDEB2(("walkmime:content-type: %s\n", ctt.c_str()));
    MimeHeaderValue content_type;
    parseMimeHeaderValue(ctt, content_type);
	    
    // Get and parse Content-Disposition header
    string ctd = "inline";
    if (doc->h.getFirstHeader("Content-Disposition", hi)) {
	ctd = hi.getValue();
    }
    MimeHeaderValue content_disposition;
    parseMimeHeaderValue(ctd, content_disposition);
    LOGDEB2(("Content_disposition:[%s]\n", content_disposition.value.c_str()));
    string dispindic;
    if (stringlowercmp("inline", content_disposition.value))
	dispindic = "Attachment";
    else 
	dispindic = "Inline";

    // See if we have a filename.
    string filename;
    map<string,string>::const_iterator it;
    it = content_disposition.params.find(string("filename"));
    if (it != content_disposition.params.end())
	filename = it->second;

    if (doc->isMessageRFC822()) {
	LOGDEB2(("walkmime: message/RFC822 part\n"));
	
	// The first part is the already parsed message.  Call
	// processMsg instead of walkmime so that mail headers get
	// printed. The depth will tell it what to do
	if (doc->members.empty()) {
	    //??
	    return;
	}
	out += "\n";
	if (m_forPreview)
	    out += "[" + dispindic + " " + content_type.value + ": ";
	out += filename;
	if (m_forPreview)
	    out += "]";
	out += "\n\n";
	processMsg(&doc->members[0], depth);
	return;
    }

    // "Simple" part. 
    LOGDEB2(("walkmime: simple  part\n"));

    // If the Content-Disposition is not inline, we treat it as
    // attachment, as per rfc2183. We don't process attachments
    // for now, except for indexing/displaying the file name
    // If it is inline but not text or html, same thing.
    if (stringlowercmp("inline", content_disposition.value) ||
	(stringlowercmp("text/plain", content_type.value) && 
	 stringlowercmp("text/html", content_type.value)) ) {
	if (!filename.empty()) {
	    out += "\n";
	    if (m_forPreview)
		out += "[" + dispindic + " " + content_type.value + ": ";
	    out += filename;
	    if (m_forPreview)
		out += "]";
	    out += "\n\n";
	}
	// We're done with this part
	return;
    }

    // We are dealing with an inline part of text/plain or text/html type

    // Normally the default charset is us-ascii. But it happens that
    // 8 bit chars exist in a message that is stated as us-ascii. Ie the 
    // mailer used by yahoo support ('KANA') does this. We could convert 
    // to iso-8859 only if the transfer-encoding is 8 bit, or test for
    // actual 8 bit chars, but what the heck, le'ts use 8859-1 as default
    string charset = "iso-8859-1";
    it = content_type.params.find(string("charset"));
    if (it != content_type.params.end())
	charset = it->second;
    if (charset.empty() || 
	!stringlowercmp("us-ascii", charset) || 
	!stringlowercmp("default", charset) || 
	!stringlowercmp("x-user-defined", charset) || 
	!stringlowercmp("x-unknown", charset) || 
	!stringlowercmp("unknown", charset) ) {
	charset = "iso-8859-1";
    }

    // Content transfer encoding
    string cte = "7bit";
    if (doc->h.getFirstHeader("Content-Transfer-Encoding", hi)) {
	cte = hi.getValue();
    } 

    LOGDEB2(("walkmime: final: body start offset %d, length %d\n", 
	     doc->getBodyStartOffset(), doc->getBodyLength()));
    string body;
    doc->getBody(body, 0, doc->bodylength);

    // Decode according to content transfer encoding
    if (!stringlowercmp("quoted-printable", cte)) {
	string decoded;
	if (!qp_decode(body, decoded)) {
	    LOGERR(("walkmime: quoted-printable decoding failed !\n"));
	    return;
	}
	body = decoded;
    } else if (!stringlowercmp("base64", cte)) {
	string decoded;
	if (!base64_decode(body, decoded)) {
	    LOGERR(("walkmime: base64 decoding failed !\n"));
#if 0
	    FILE *fp = fopen("/tmp/recoll_decodefail", "w");
	    if (fp) {
		fprintf(fp, "%s", body.c_str());
		fclose(fp);
	    }
#endif
	    return;
	}
	body = decoded;
    }

    // Handle html stripping and transcoding to utf8
    string utf8;
    const string *putf8 = 0;
    if (!stringlowercmp("text/html", content_type.value)) {
	MimeHandlerHtml mh("text/html");
	mh.set_property(Dijon::Filter::OPERATING_MODE, 
			m_forPreview ? "view" : "index");
	mh.set_property(Dijon::Filter::DEFAULT_CHARSET, charset);
	mh.set_document_string(body);
	mh.next_document();
	map<string, string>::const_iterator it = 
	    mh.get_meta_data().find("content");
	if (it != mh.get_meta_data().end())
	    putf8 = &it->second;
    } else {
	// Transcode to utf-8 
	if (!transcode(body, utf8, charset, "UTF-8")) {
	    LOGERR(("walkmime: transcode failed from cs '%s' to UTF-8\n",
		    charset.c_str()));
	    putf8 = &body;
	} else {
	    putf8 = &utf8;
	}
    }
    if (putf8)
	out += *putf8;
    if (out.length() && out[out.length()-1] != '\n')
	out += '\n';
    
    LOGDEB2(("walkmime: out now: [%s]\n", out.c_str()));
}
