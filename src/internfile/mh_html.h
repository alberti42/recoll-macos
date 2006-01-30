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
#ifndef _HTML_H_INCLUDED_
#define _HTML_H_INCLUDED_
/* @(#$Id: mh_html.h,v 1.7 2006-01-30 11:15:27 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

#include "mimehandler.h"

/**
 Translate html document to internal one. 

 There are 2 interfaces, depending if we're working on a file, or
 on a string. The string form is applied to the output of external
 handlers for foreign formats: they return a result in html, which
 has the advantage to be text (easy to use in shell-scripts), and
 semi-structured (can carry titles, abstracts, whatever)
*/
class MimeHandlerHtml : public MimeHandler {
 public:
    std::string charsethint;

    /** Create internal document from html file (standard interface) */
    virtual MimeHandler::Status 
	mkDoc(RclConfig *conf, const std::string &fn, 
	      const std::string &mtype, Rcl::Doc &docout, std::string&);

    /** Create internal doc from html string (postfilter for external ones) */
    virtual MimeHandler::Status 
	mkDoc(RclConfig *conf, const std::string &fn, const std::string& htext,
	      const std::string &mtype, Rcl::Doc &docout);
};

#endif /* _HTML_H_INCLUDED_ */
