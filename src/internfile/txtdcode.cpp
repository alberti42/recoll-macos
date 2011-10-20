/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "autoconfig.h"

#include "cstr.h"
#include "transcode.h"
#include "mimehandler.h"
#include "debuglog.h"

bool RecollFilter::txtdcode(const string& who)
{
    if (m_metaData[cstr_mimetype].compare(cstr_textplain)) {
	LOGERR(("%s::txtdcode: called on non txt/plain: %s\n", who.c_str(),
		m_metaData[cstr_mimetype].c_str()));
	return false;
    }

    string& ocs = m_metaData[cstr_origcharset];
    string& itext = m_metaData[cstr_content];
    LOGDEB0(("%s::txtdcode: %d bytes from [%s] to UTF-8\n", 
	     who.c_str(), itext.size(), ocs.c_str()));
    int ecnt;
    bool ret;
    string otext;
    if (!(ret=transcode(itext, otext, ocs, "UTF-8", &ecnt)) || 
	ecnt > int(itext.size() / 4)) {
	LOGERR(("%s::txtdcode: transcode %d bytes to UTF-8 failed "
		"for input charset [%s] ret %d ecnt %d\n", 
		who.c_str(), itext.size(), ocs.c_str(), ret, ecnt));
	itext.erase();
	return false;
    }
    itext.swap(otext);
    m_metaData[cstr_charset] = "UTF-8";
    return true;
}
