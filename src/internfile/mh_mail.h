#ifndef _MAIL_H_INCLUDED_
#define _MAIL_H_INCLUDED_
/* @(#$Id: mh_mail.h,v 1.1 2005-03-25 09:40:27 dockes Exp $  (C) 2004 J.F.Dockes */
#include "mimehandler.h"

// Code to turn a mail folder file into internal documents
class MimeHandlerMail : public MimeHandler {
    RclConfig *conf;
    MimeHandler::Status processone(const string &fn, Rcl::Doc &docout);
 public:
    MimeHandlerMail() : conf(0) {}
    virtual MimeHandler::Status 
	worker(RclConfig *conf, const string &fn, 
	       const string &mtype, Rcl::Doc &docout, string& ipath);
};
#endif /* _MAIL_H_INCLUDED_ */
