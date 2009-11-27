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
#ifndef _MBOX_H_INCLUDED_
#define _MBOX_H_INCLUDED_
/* @(#$Id: mh_mbox.h,v 1.3 2008-10-04 14:26:59 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "mimehandler.h"

/** 
 * Translate a mail folder file into internal documents (also works
 * for maildir files). This has to keep state while parsing a mail folder
 * file. 
 */
class MimeHandlerMbox : public RecollFilter {
 public:
    MimeHandlerMbox(const string& mime) 
      : RecollFilter(mime), m_vfp(0), m_msgnum(0), m_lineno(0), m_fsize(0)
    {}
    virtual ~MimeHandlerMbox();
    virtual bool set_document_file(const string &file_path);
    virtual bool next_document();
    virtual bool skip_to_document(const string& ipath) {
	m_ipath = ipath;
	return true;
    }
    virtual void clear();
    typedef long long mbhoff_type;
 private:
    string     m_fn;     // File name
    void      *m_vfp;    // File pointer for folder
    int        m_msgnum; // Current message number in folder. Starts at 1
    string     m_ipath;
    int        m_lineno; // debug 
    mbhoff_type m_fsize;
    vector<mbhoff_type> m_offsets;
};

#endif /* _MBOX_H_INCLUDED_ */
