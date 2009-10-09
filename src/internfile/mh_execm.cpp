#ifndef lint
static char rcsid[] = "@(#$Id: mh_exec.cpp,v 1.14 2008-10-09 09:19:37 dockes Exp $ (C) 2005 J.F.Dockes";
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
#include <sstream>

#include "mh_execm.h"
#include "mh_html.h"
#include "debuglog.h"
#include "cancelcheck.h"
#include "smallut.h"
#include "transcode.h"
#include "md5.h"

#include <sys/types.h>
#include <sys/wait.h>

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

bool MimeHandlerExecMultiple::startCmd()
{
    LOGDEB(("MimeHandlerExecMultiple::startCmd\n"));
    // Command name
    string cmd = params.front();
    
    // Build parameter list: delete cmd name
    list<string>::iterator it = params.begin();
    list<string>myparams(++it, params.end());

    // Start filter
    m_cmd.putenv(m_forPreview ? "RECOLL_FILTER_FORPREVIEW=yes" :
		"RECOLL_FILTER_FORPREVIEW=no");
    if (m_cmd.startExec(cmd, myparams, 1, 1) < 0) {
        missingHelper = true;
        return false;
    }
    return true;
}

bool MimeHandlerExecMultiple::readDataElement(string& name)
{
    string ibuf;
    if (m_cmd.getline(ibuf) <= 0) {
        LOGERR(("MHExecMultiple: getline error\n"));
        return false;
    }
    if (!ibuf.compare("\n")) {
        LOGDEB(("MHExecMultiple: Got empty line\n"));
        name = "";
        return true;
    }

    // We're expecting something like paramname: len\n
    list<string> tokens;
    stringToTokens(ibuf, tokens);
    if (tokens.size() != 2) {
        LOGERR(("MHExecMultiple: bad line in filter output: [%s]\n",
                ibuf.c_str()));
        return false;
    }
    list<string>::iterator it = tokens.begin();
    name = *it++;
    string& slen = *it;
    int len;
    if (sscanf(slen.c_str(), "%d", &len) != 1) {
        LOGERR(("MHExecMultiple: bad line in filter output: [%s]\n",
                ibuf.c_str()));
        return false;
    }
    LOGDEB(("MHExecMultiple: got paramname [%s] len: %d\n", 
            name.c_str(), len));
    // We only care about the "data:" field for now
    string discard;
    string *datap;
    if (!stringlowercmp("data:", name)) {
        datap = &m_metaData["content"];
    } else {
        datap = &discard;
    }
    // Then the data.
    datap->erase();
    if (m_cmd.receive(*datap, len) != len) {
        LOGERR(("MHExecMultiple: expected %d bytes of data, got %d\n", 
                len, datap->length()));
        return false;
    }
    return true;
}

// Execute an external program to translate a file from its native
// format to text or html.
bool MimeHandlerExecMultiple::next_document()
{
    if (m_havedoc == false)
	return false;
    if (missingHelper) {
	LOGDEB(("MHExecMultiple::next_document(): helper known missing\n"));
	return false;
    }
    if (params.empty()) {
	// Hu ho
	LOGERR(("MHExecMultiple::mkDoc: empty params\n"));
	m_reason = "RECFILTERROR BADCONFIG";
	return false;
    }

    if (m_cmd.getChildPid() < 0 && !startCmd()) {
        return false;
    }

    // Send request to child process
    ostringstream obuf;
    obuf << "FileName: " << m_fn.length() << endl << m_fn << endl;
    if (m_cmd.send(obuf.str()) < 0) {
        LOGERR(("MHExecMultiple: send error\n"));
        return false;
    }

    // Read answer
    LOGDEB(("MHExecMultiple: reading answer\n"));
    for (int loop=0;;loop++) {
        string name;
        if (!readDataElement(name)) {
            return false;
        }
        if (name.empty())
            break;
        if (loop == 10) {
            // ?? 
            LOGERR(("MHExecMultiple: filter sent too many parameters\n"));
            return false;
        }
    }
    
    finaldetails();
    m_havedoc = false;
    return true;
}
