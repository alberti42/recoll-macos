#ifndef _MAIL_H_INCLUDED_
#define _MAIL_H_INCLUDED_
/* @(#$Id: mh_mail.h,v 1.4 2005-11-18 13:23:46 dockes Exp $  (C) 2004 J.F.Dockes */

#include "mimehandler.h"

namespace Binc {
    class MimeDocument;
}

/** 
    Translate a mail folder file into internal documents (also works
    for maildir files). This has to keep state while parsing a mail folder
    file. 
*/
class MimeHandlerMail : public MimeHandler {
 public:
    MimeHandlerMail() : vfp(0), msgnum(0), conf(0) {}

    virtual MimeHandler::Status 
	mkDoc(RclConfig *conf, const std::string &fn, 
	      const std::string &mtype, Rcl::Doc &docout, std::string& ipath);

    virtual ~MimeHandlerMail();

 private:
    void *vfp;
    int msgnum;
    RclConfig *conf;
    MimeHandler::Status processone(const string &fn, Binc::MimeDocument& doc,
				   Rcl::Doc &docout);
    MimeHandler::Status processmbox(const string &fn, Rcl::Doc &docout, 
				   string &ipath);
};

#endif /* _MAIL_H_INCLUDED_ */
