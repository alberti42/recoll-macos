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
#ifndef _MH_UNKNOWN_H_INCLUDED_
#define _MH_UNKNOWN_H_INCLUDED_
/* @(#$Id: mh_unknown.h,v 1.2 2006-12-15 12:40:02 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

#include "mimehandler.h"

/**
 * Handler for files with no content handler: does nothing.
 *
 */
class MimeHandlerUnknown : public RecollFilter {
 public:
    MimeHandlerUnknown(const string& mt) : RecollFilter(mt) {}
    virtual ~MimeHandlerUnknown() {}
    virtual bool set_document_string(const string&) {
	return m_havedoc = true;
    }
    virtual bool set_document_file(const string&) {
	return m_havedoc = true;
    }
    virtual bool next_document() {
	if (m_havedoc == false)
	    return false;
	m_havedoc = false; 
	m_metaData["content"] = "";
	m_metaData["mimetype"] = "text/plain";
	return true;
    }
};

#endif /* _MH_UNKNOWN_H_INCLUDED_ */
