#ifndef lint
static char rcsid[] = "@(#$Id: mh_mail.cpp,v 1.1 2005-03-25 09:40:27 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include <fcntl.h>
#include <errno.h>

#include <map>
using std::map;

#include "mimehandler.h"
#include "debuglog.h"
#include "csguess.h"
#include "readfile.h"
#include "transcode.h"
#include "mimeparse.h"
#include "indextext.h"
#include "mail.h"
#include "debuglog.h"
#include "smallut.h"
#include "mimeparse.h"

using namespace std;

// We are called for two different file types: mbox-type folders
// holding multiple messages, and maildir-type files with one rfc822
// message
MimeHandler::Status 
MimeHandlerMail::worker(RclConfig *cnf, const string &fn, 
			const string &mtype, Rcl::Doc &docout, string&)
{
    LOGDEB(("MimeHandlerMail::worker: %s [%s]\n", mtype.c_str(), fn.c_str()));
    conf = cnf;

    if (!stringlowercmp("message/rfc822", mtype)) {
	return processone(fn, docout);
    } else  if (!stringlowercmp("text/x-mail", mtype)) {
	return MimeHandler::MHError;
    } else
	return MimeHandler::MHError;
}


#include "mime.h"

const char *hnames[] = {"Subject", "Content-type"};
int nh = sizeof(hnames) / sizeof(char *);

void walkmime(string &out, Binc::MimePart& doc, int fd, int depth);

// Transform a single message into a document. The subject becomes the
// title, and any simple body part with a content-type of text or html
// and content-disposition inline gets concatenated as text.
MimeHandler::Status 
MimeHandlerMail::processone(const string &fn, Rcl::Doc &docout)
{
    int fd;
    if ((fd = open(fn.c_str(), 0)) < 0) {
	LOGERR(("MimeHandlerMail::processone: open(%s) errno %d\n",
		fn.c_str(), errno));
	return MimeHandler::MHError;
    }
    Binc::MimeDocument doc;
    doc.parseFull(fd);

    if (!doc.isHeaderParsed() && !doc.isAllParsed()) {
	LOGERR(("MimeHandlerMail::processone: parse error for %s\n", 
		fn.c_str()));
	close(fd);
	return MimeHandler::MHError;
    }
    LOGDEB(("MimeHandlerMail::processone: ismultipart %d mime subtype '%s'\n", 
	    doc.isMultipart(), doc.getSubType().c_str()));
    walkmime(docout.text, doc, fd, 0);
    close(fd);
    LOGDEB(("MimeHandlerMail::processone: text: '%s'\n",  docout.text.c_str()));
    return MimeHandler::MHError;
}

void walkmime(string &out, Binc::MimePart& doc, int fd, int depth)
{
    if (depth > 5) {
	LOGINFO(("walkmime: max depth exceeded\n"));
	return;
    }

    if (doc.isMultipart()) {
	LOGDEB(("walkmime: ismultipart %d subtype '%s'\n", 
		doc.isMultipart(), doc.getSubType().c_str()));
	// We only handle alternative and mixed for now. For
	// alternative, we look for a text/plain part, else html and process it
	// For mixed, we process each part.
	std::vector<Binc::MimePart>::iterator it;
	if (!stringicmp("mixed", doc.getSubType())) {
	    for (it = doc.members.begin(); it != doc.members.end();it++) {
		walkmime(out, *it, fd, depth+1);
	    }
	} else if (!stringicmp("alternative", doc.getSubType())) {
	    std::vector<Binc::MimePart>::iterator ittxt, ithtml;
	    ittxt = ithtml = doc.members.end();
	    for (it = doc.members.begin(); it != doc.members.end();it++) {
		// Get and parse content-type header
		Binc::HeaderItem hi;
		if (!doc.h.getFirstHeader("Content-Type", hi)) 
		    continue;
		LOGDEB(("walkmime:content-type: %s\n", hi.getValue().c_str()));
	    }
	}
    } else {
	// If content-type is text or html and content-disposition is inline, 
	// decode and add to text.

	// Get and parse content-type header.
	Binc::HeaderItem hi;
	string ctt = "text/plain";
	if (doc.h.getFirstHeader("Content-Type", hi)) {
	    ctt = hi.getValue();
	}
	LOGDEB(("walkmime:content-type: %s\n", ctt.c_str()));
	MimeHeaderValue content_type;
	parseMimeHeaderValue(ctt, content_type);
	if (stringlowercmp("text/plain", content_type.value) && 
	    stringlowercmp("text/html", content_type.value)) {
	    return;
	}
	string charset = "us-ascii";
	map<string,string>::const_iterator it;
	it = content_type.params.find(string("charset"));
	if (it != content_type.params.end())
	    charset = it->second;

	// Content disposition
	string ctd = "inline";
	if (doc.h.getFirstHeader("Content-Disposition", hi)) {
	    ctd = hi.getValue();
	}
	MimeHeaderValue content_disposition;
	parseMimeHeaderValue(ctd, content_disposition);
	if (stringlowercmp("inline", content_disposition.value)) {
	    return;
	}

	// Content transfer encoding
	string cte = "7bit";
	if (doc.h.getFirstHeader("Content-Transfer-Encoding", hi)) {
	    cte = hi.getValue();
	} 

	LOGDEB(("walkmime: final: body start offset %d, length %d\n", 
		doc.getBodyStartOffset(), doc.getBodyLength()));
	string body;
	doc.getBody(fd, body, 0, doc.bodylength);

	// Decode content transfer encoding
	if (stringlowercmp("quoted-printable", content_disposition.value)) {
	    string decoded;
	    qp_decode(body, decoded);
	    body = decoded;
	} else if (stringlowercmp("base64", content_disposition.value)) {
	    string decoded;
	    base64_decode(body, decoded);
	    body = decoded;
	}


        // Transcode to utf-8 
	string transcoded;
	if (!transcode(body, transcoded, charset, "UTF-8")) {
	    LOGERR(("walkmime: transcode failed from cs '%s' to UTF-8\n",
		    charset.c_str()));
	    transcoded = body;
	}

	out += string("\r\n") + transcoded;
    }
}


