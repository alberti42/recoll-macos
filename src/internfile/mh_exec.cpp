#ifndef lint
static char rcsid[] = "@(#$Id: mh_exec.cpp,v 1.10 2008-08-26 07:31:54 dockes Exp $ (C) 2005 J.F.Dockes";
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

#include "execmd.h"
#include "mh_exec.h"
#include "mh_html.h"
#include "debuglog.h"
#include "cancelcheck.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

class MEAdv : public ExecCmdAdvise {
public:
    void newData(int) {
	CancelCheck::instance().checkCancel();
    }
};

// Execute an external program to translate a file from its native format
// to html. Then call the html parser to do the actual indexing
bool MimeHandlerExec::next_document()
{
    if (m_havedoc == false)
	return false;
    m_havedoc = false;
    if (params.empty()) {
	// Hu ho
	LOGERR(("MimeHandlerExec::mkDoc: empty params\n"));
	m_reason = "RECFILTERROR BADCONFIG";
	return false;
    }

    // Command name
    string cmd = params.front();
    
    // Build parameter list: delete cmd name and add the file name
    list<string>::iterator it = params.begin();
    list<string>myparams(++it, params.end());
    myparams.push_back(m_fn);
    if (!m_ipath.empty())
	myparams.push_back(m_ipath);

    // Execute command and store the result text, which is supposedly html
    string& html = m_metaData["content"];
    html.erase();
    ExecCmd mexec;
    MEAdv adv;
    mexec.setAdvise(&adv);
    mexec.putenv(m_forPreview ? "RECOLL_FILTER_FORPREVIEW=yes" :
		"RECOLL_FILTER_FORPREVIEW=no");
    int status = mexec.doexec(cmd, myparams, 0, &html);
    if (status) {
	LOGERR(("MimeHandlerExec: command status 0x%x: %s\n", 
		status, cmd.c_str()));
	// If the output string begins with RECFILTERROR, then it's 
	// interpretable error information
	if (html.find("RECFILTERROR") == 0)
	    m_reason = html;
	return false;
    }

    m_metaData["origcharset"] = m_defcharset;
    // All recoll filters output utf-8
    m_metaData["charset"] = "utf-8";
    m_metaData["mimetype"] = "text/html";
    return true;
}
