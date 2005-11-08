#ifndef _MAIL_H_INCLUDED_
#define _MAIL_H_INCLUDED_
/* @(#$Id: mh_mail.h,v 1.3 2005-11-08 21:02:55 dockes Exp $  (C) 2004 J.F.Dockes */
#include "mimehandler.h"
namespace Binc {
    class MimeDocument;
}

/// Translate a mail folder file into internal documents (also works
/// for maildir files)
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
	mkDoc(RclConfig *conf, const string &fn, 
	       const string &mtype, Rcl::Doc &docout, string& ipath);
};

#endif /* _MAIL_H_INCLUDED_ */
