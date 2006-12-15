#ifndef lint
static char rcsid[] = "@(#$Id: mh_mbox.cpp,v 1.1 2006-12-15 12:40:24 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include <regex.h>

#include <map>
#include <sstream>

#include "mimehandler.h"
#include "debuglog.h"
#include "readfile.h"
#include "mh_mbox.h"
#include "smallut.h"

using namespace std;

MimeHandlerMbox::~MimeHandlerMbox()
{
    if (m_vfp) {
	fclose((FILE *)m_vfp);
	m_vfp = 0;
    }
}

bool MimeHandlerMbox::set_document_file(const string &fn)
{
    LOGDEB(("MimeHandlerMbox::set_document_file(%s)\n", fn.c_str()));
    m_fn = fn;
    if (m_vfp) {
	fclose((FILE *)m_vfp);
	m_vfp = 0;
    }

    m_vfp = fopen(fn.c_str(), "r");
    if (m_vfp == 0) {
	LOGERR(("MimeHandlerMail::set_document_file: error opening %s\n", 
		fn.c_str()));
	return false;
    }
    m_havedoc = true;
    return true;
}

static const  char *frompat = "^From .* [1-2][0-9][0-9][0-9][\r]*\n$";
static regex_t fromregex;
static bool regcompiled;

bool MimeHandlerMbox::next_document()
{
    if (m_vfp == 0) {
	LOGERR(("MimeHandlerMbox::next_document: not open\n"));
	return false;
    }
    if (!m_havedoc) {
	return false;
    }
    FILE *fp = (FILE *)m_vfp;
    int mtarg = 0;
    if (m_ipath != "") {
	sscanf(m_ipath.c_str(), "%d", &mtarg);
    } else if (m_forPreview) {
	// Can't preview an mbox
	return false;
    }
    LOGDEB(("MimeHandlerMbox::next_document: fn %s, msgnum %d mtarg %d \n", 
	    m_fn.c_str(), m_msgnum, mtarg));

    if (!regcompiled) {
	regcomp(&fromregex, frompat, REG_NOSUB);
	regcompiled = true;
    }

    // If we are called to retrieve a specific message, seek to bof
    // (then scan up to the message). This is for the case where the
    // same object is reused to fetch several messages (else the fp is
    // just opened no need for a seek).  We could also check if the
    // current message number is lower than the requested one and
    // avoid rereading the whole thing in this case. But I'm not sure
    // we're ever used in this way (multiple retrieves on same
    // object).  So:
    if (mtarg > 0) {
	fseek(fp, 0, SEEK_SET);
	m_msgnum = 0;
    }

    off_t start, end;
    bool iseof = false;
    bool hademptyline = true;
    string& msgtxt = m_metaData["content"];
    msgtxt.erase();
    do  {
	// Look for next 'From ' Line, start of message. Set start to
	// line after this
	char line[501];
	for (;;) {
	    if (!fgets(line, 500, fp)) {
		// Eof hit while looking for 'From ' -> file done. We'd need
		// another return code here
		return false;
	    }
	    if (line[0] == '\n' || line[0] == '\r') {
		hademptyline = true;
		continue;
	    }
	    if (hademptyline && !regexec(&fromregex, line, 0, 0, 0)) {
		start = ftello(fp);
		m_msgnum++;
		break;
	    }
	    hademptyline = false;
	}

	// Look for next 'From ' line or eof, end of message.
	for (;;) {
	    end = ftello(fp);
	    if (!fgets(line, 500, fp)) {
		if (ferror(fp) || feof(fp))
		    iseof = true;
		break;
	    }
	    if (hademptyline && !regexec(&fromregex, line, 0, 0, 0)) {
		break;
	    }
	    if (mtarg <= 0 || m_msgnum == mtarg) {
		msgtxt += line;
	    }
	    if (line[0] == '\n' || line[0] == '\r') {
		hademptyline = true;
	    } else {
		hademptyline = false;
	    }
	}
	fseek(fp, end, SEEK_SET);
    } while (mtarg > 0 && m_msgnum < mtarg);

    LOGDEB2(("Message text: [%s]\n", msgtxt.c_str()));
    char buf[20];
    sprintf(buf, "%d", m_msgnum);
    m_metaData["ipath"] = buf;
    m_metaData["mimetype"] = "message/rfc822";
    if (iseof)
	m_havedoc = false;
    return msgtxt.empty() ? false : true;
}
