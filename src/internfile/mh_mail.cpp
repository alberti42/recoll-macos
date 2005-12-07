#ifndef lint
static char rcsid[] = "@(#$Id: mh_mail.cpp,v 1.12 2005-12-07 15:41:50 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#include <map>
#include <sstream>

#include "mimehandler.h"
#include "debuglog.h"
#include "csguess.h"
#include "readfile.h"
#include "transcode.h"
#include "mimeparse.h"
#include "indextext.h"
#include "mh_mail.h"
#include "debuglog.h"
#include "smallut.h"
#include "mimeparse.h"
#include "mh_html.h"

// binc imap mime definitions
#include "mime.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

static void 
walkmime(RclConfig *cnf, string &out, Binc::MimePart& doc, int depth);

MimeHandlerMail::~MimeHandlerMail()
{
    if (vfp) {
	fclose((FILE *)vfp);
	vfp = 0;
    }
}

// We are called for two different file types: mbox-type folders
// holding multiple messages, and maildir-type files with one message
MimeHandler::Status 
MimeHandlerMail::mkDoc(RclConfig *cnf, const string &fn, 
			const string &mtype, Rcl::Doc &docout, string& ipath)
{
    LOGDEB2(("MimeHandlerMail::mkDoc: %s [%s]\n", mtype.c_str(), fn.c_str()));
    conf = cnf;

    if (!stringlowercmp("message/rfc822", mtype)) {
	ipath = "";
	int fd;
	if ((fd = open(fn.c_str(), 0)) < 0) {
	    LOGERR(("MimeHandlerMail::mkDoc: open(%s) errno %d\n",
		    fn.c_str(), errno));
	    return MimeHandler::MHError;
	}
	Binc::MimeDocument doc;
	doc.parseFull(fd);
	MimeHandler::Status ret = processone(fn, doc, docout);
	close(fd);
	return ret;
    } else  if (!stringlowercmp("text/x-mail", mtype)) {
	return processmbox(fn, docout, ipath);
    } else // hu ho
	return MimeHandler::MHError;
}

MimeHandler::Status 
MimeHandlerMail::processmbox(const string &fn, Rcl::Doc &docout, string& ipath)
{
    int mtarg = 0;
    if (ipath != "") {
	sscanf(ipath.c_str(), "%d", &mtarg);
    }
    LOGDEB2(("MimeHandlerMail::processmbox: fn %s, mtarg %d\n", fn.c_str(),
	    mtarg));

    FILE *fp;
    if (vfp) {
	fp = (FILE *)vfp;
    } else {
	fp = fopen(fn.c_str(), "r");
	if (fp == 0) {
	    LOGERR(("MimeHandlerMail::processmbox: error opening %s\n", 
		    fn.c_str()));
	    return MimeHandler::MHError;
	}
	vfp = fp;
    }
    if (mtarg > 0) {
	fseek(fp, 0, SEEK_SET);
	msgnum = 0;
    }

    off_t start, end;
    bool iseof = false;
    do  {
	// Look for next 'From ' Line, start of message. Set start to
	// line after this
	char line[301];
	for (;;) {
	    if (!fgets(line, 300, fp)) {
		// Eof hit while looking for 'From ' -> file done. We'd need
		// another return code here
		return MimeHandler::MHError;
	    }

	    if (!strncmp("From ", line, 5)) {
		start = ftello(fp);
		break;
	    }
	}

	// Look for next 'From ' line or eof, end of message (we let a
	// spurious empty line in)
	for (;;) {
	    end = ftello(fp);
	    if (!fgets(line, 300, fp) || !strncmp("From ", line, 5)) {
		if (ferror(fp) || feof(fp))
		    iseof = true;
		break;
	    }
	}
	msgnum++;
	fseek(fp, end, SEEK_SET);
    } while (mtarg > 0 && msgnum < mtarg);


    size_t size = end - start;
    fseek(fp, start, SEEK_SET);
    char *cp = (char *)malloc(size);
    if (cp == 0) {
	LOGERR(("MimeHandlerMail::processmbox: malloc(%d) failed\n", size));
	return MimeHandler::MHError;
    }
    if (fread(cp, 1, size, fp) != size) {
	LOGERR(("MimeHandlerMail::processmbox: fread failed (errno %d)\n",
		errno));
	free(cp);
	return MimeHandler::MHError;
    }
    string msgbuf(cp, size);
    free(cp);
    stringstream s(msgbuf);
    Binc::MimeDocument doc;
    doc.parseFull(s);
    MimeHandler::Status ret = processone(fn, doc, docout);
    if (ret == MimeHandler::MHError)
	return ret;
    char buf[20];
    sprintf(buf, "%d", msgnum);
    ipath = buf;
    return iseof ? MimeHandler::MHDone : 
	(mtarg > 0) ? MimeHandler::MHDone : MimeHandler::MHAgain;
}


// Transform a single message into a document. The subject becomes the
// title, and any simple body part with a content-type of text or html
// and content-disposition inline gets concatenated as text.
MimeHandler::Status 
MimeHandlerMail::processone(const string &fn, Binc::MimeDocument& doc, 
			    Rcl::Doc &docout)
{
    if (!doc.isHeaderParsed() && !doc.isAllParsed()) {
	LOGERR(("MimeHandlerMail::processone: mime parse error for %s\n", 
		fn.c_str()));
	return MimeHandler::MHError;
    }

    // Handle some headers. 
    Binc::HeaderItem hi;
    string transcoded;
    if (doc.h.getFirstHeader("Subject", hi)) {
	rfc2047_decode(hi.getValue(), transcoded);
	docout.title = transcoded;
    }
    if (doc.h.getFirstHeader("From", hi)) {
	rfc2047_decode(hi.getValue(), transcoded);
	docout.text += string("From: ") + transcoded + string("\n");
    }
    if (doc.h.getFirstHeader("To", hi)) {
	rfc2047_decode(hi.getValue(), transcoded);
	docout.text += string("To: ") + transcoded + string("\n");
    }
    if (doc.h.getFirstHeader("Date", hi)) {
	rfc2047_decode(hi.getValue(), transcoded);
	// Try to set the mtime from the date field.
	string date = transcoded;
	string::size_type pos;
	// Possibly get rid of the day
	if ((pos = date.find(",")) != string::npos)
	    date = date.substr(pos+1);
	struct tm tm;
	if (strptime(date.c_str(), " %d %b %Y %H:%M:%S %z ", &tm)) {
	    char ascuxtime[100];
	    sprintf(ascuxtime, "%ld", (long)mktime(&tm));
	    docout.dmtime = ascuxtime;
	} else {
	    LOGDEB(("strptime failed for [%s]\n", date.c_str()));
	}

	docout.text += string("Date: ") + transcoded + string("\n");
    }
    if (doc.h.getFirstHeader("Subject", hi)) {
	rfc2047_decode(hi.getValue(), transcoded);
	docout.text += string("Subject: ") + transcoded + string("\n");
    }

    LOGDEB2(("MimeHandlerMail::processone: ismultipart %d mime subtype '%s'\n",
	    doc.isMultipart(), doc.getSubType().c_str()));
    walkmime(conf, docout.text, doc, 0);

    // LOGDEB(("MimeHandlerMail::processone: text: '%s'\n", docout.text.c_str()));
    return MimeHandler::MHDone;
}

// Recursively walk the message mime parts and concatenate all the
// inline html or text that we find anywhere.
static void walkmime(RclConfig *cnf, string &out, Binc::MimePart& doc, 
		     int depth)
{
    if (depth > 5) {
	LOGINFO(("walkmime: max depth exceeded\n"));
	return;
    }

    if (doc.isMultipart()) {
	LOGDEB2(("walkmime: ismultipart %d subtype '%s'\n", 
		doc.isMultipart(), doc.getSubType().c_str()));
	// We only handle alternative, related and mixed for now. For
	// alternative, we look for a text/plain part, else html and
	// process it For mixed and related, we process each part.
	std::vector<Binc::MimePart>::iterator it;
	if (!stringicmp("mixed", doc.getSubType()) || 
	    !stringicmp("related", doc.getSubType())) {
	    for (it = doc.members.begin(); it != doc.members.end();it++) {
		walkmime(cnf, out, *it, depth+1);
	    }
	} else if (!stringicmp("alternative", doc.getSubType())) {
	    std::vector<Binc::MimePart>::iterator ittxt, ithtml;
	    ittxt = ithtml = doc.members.end();
	    int i = 1;
	    for (it = doc.members.begin(); it != doc.members.end();it++, i++) {
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
	    if (ittxt != doc.members.end()) {
		LOGDEB2(("walkmime: alternative: chose text/plain part\n"))
		walkmime(cnf, out, *ittxt, depth+1);
	    } else if (ithtml != doc.members.end()) {
		LOGDEB2(("walkmime: alternative: chose text/html part\n"))
		walkmime(cnf, out, *ithtml, depth+1);
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
	LOGDEB2(("walkmime:content-type: %s\n", ctt.c_str()));
	MimeHeaderValue content_type;
	parseMimeHeaderValue(ctt, content_type);
	if (stringlowercmp("text/plain", content_type.value) && 
	    stringlowercmp("text/html", content_type.value)) {
	    return;
	}

	// Normally the default charset is us-ascii. But it happens that
	// 8 bit chars exist in a message that is stated as us-ascii. Ie the 
	// mailer used by yahoo support ('KANA') does this. We could convert 
	// to iso-8859 only if the transfer-encoding is 8 bit, or test for
	// actual 8 bit chars, but what the heck, le'ts use 8859-1 as default
	string charset = "iso-8859-1";
	map<string,string>::const_iterator it;
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

	LOGDEB2(("walkmime: final: body start offset %d, length %d\n", 
		 doc.getBodyStartOffset(), doc.getBodyLength()));
	string body;
	doc.getBody(body, 0, doc.bodylength);

	// Decode content transfer encoding
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

	string transcoded;
	if (!stringlowercmp("text/html", content_type.value)) {
	    MimeHandlerHtml mh;
	    Rcl::Doc hdoc;
	    mh.charsethint = charset;
	    mh.mkDoc(cnf, "", body, content_type.value,  hdoc);
	    transcoded = hdoc.text;
	} else {
	    // Transcode to utf-8 
	    if (!transcode(body, transcoded, charset, "UTF-8")) {
		LOGERR(("walkmime: transcode failed from cs '%s' to UTF-8\n",
			charset.c_str()));
		transcoded = body;
	    }
	}

	out += string("\r\n") + transcoded;
	LOGDEB2(("walkmime: out now: [%s]\n", out.c_str()));
    }
}
