#ifndef _MAIL_H_INCLUDED_
#define _MAIL_H_INCLUDED_
/* @(#$Id: mh_mail.h,v 1.2 2005-03-31 10:04:07 dockes Exp $  (C) 2004 J.F.Dockes */
#include "mimehandler.h"
namespace Binc {
    class MimeDocument;
}

// Code to turn a mail folder file into internal documents
class MimeHandlerMail : public MimeHandler {
    void *vfp;
    int msgnum;
    RclConfig *conf;
    MimeHandler::Status processone(const string &fn, Binc::MimeDocument& doc,
				   Rcl::Doc &docout);
    MimeHandler::Status processmbox(const string &fn, Rcl::Doc &docout, 
				   string &ipath);
 public:
    MimeHandlerMail() : vfp(0), msgnum(0), conf(0) {}
    virtual ~MimeHandlerMail();
    virtual MimeHandler::Status 
	worker(RclConfig *conf, const string &fn, 
	       const string &mtype, Rcl::Doc &docout, string& ipath);
};
#endif /* _MAIL_H_INCLUDED_ */
