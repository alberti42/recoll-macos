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
#ifndef _MAIL_H_INCLUDED_
#define _MAIL_H_INCLUDED_
/* @(#$Id: mh_mail.h,v 1.6 2006-04-07 08:51:15 dockes Exp $  (C) 2004 J.F.Dockes */

#include "mimehandler.h"

namespace Binc {
    class MimeDocument;
}

/** 
 * Translate a mail folder file into internal documents (also works
 * for maildir files). This has to keep state while parsing a mail folder
 * file. 
 */
class MimeHandlerMail : public MimeHandler {
 public:
    MimeHandlerMail() : m_vfp(0), m_msgnum(0), m_conf(0) {}

    virtual MimeHandler::Status 
	mkDoc(RclConfig *conf, const std::string &fn, 
	      const std::string &mtype, Rcl::Doc &docout, std::string& ipath);

    virtual ~MimeHandlerMail();

 private:
    void      *m_vfp;    // File pointer for folder
    int        m_msgnum; // Current message number in folder. Starts at 1
    RclConfig *m_conf;   // Keep pointer to rclconfig around

    MimeHandler::Status processone(const string &fn, Binc::MimeDocument& doc,
				   Rcl::Doc &docout);
    MimeHandler::Status processmbox(const string &fn, Rcl::Doc &docout, 
				   string &ipath);
};

#endif /* _MAIL_H_INCLUDED_ */
