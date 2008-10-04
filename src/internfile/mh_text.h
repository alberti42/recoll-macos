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
#ifndef _MH_TEXT_H_INCLUDED_
#define _MH_TEXT_H_INCLUDED_
/* @(#$Id: mh_text.h,v 1.5 2008-10-04 14:26:59 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
using std::string;

#include "mimehandler.h"

/**
 * Handler for text/plain files. 
 *
 * Maybe try to guess charset, or use default, then transcode to utf8
 */
class MimeHandlerText : public RecollFilter {
 public:
    MimeHandlerText(const string& mt) : RecollFilter(mt) {}
    virtual ~MimeHandlerText() {}
    virtual bool set_document_file(const string &file_path);
    virtual bool set_document_string(const string&);
    virtual bool is_data_input_ok(DataInput input) const {
	if (input == DOCUMENT_FILE_NAME || input == DOCUMENT_STRING)
	    return true;
	return false;
    }
    virtual bool next_document();
    virtual void clear() 
    {
	m_text.erase(); 
	RecollFilter::clear();
    }
private:
    string m_text;
};

#endif /* _MH_TEXT_H_INCLUDED_ */
