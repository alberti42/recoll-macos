#ifndef _HTML_H_INCLUDED_
#define _HTML_H_INCLUDED_
/* @(#$Id: mh_html.h,v 1.1 2005-02-01 17:20:05 dockes Exp $  (C) 2004 J.F.Dockes */
#include "mimehandler.h"

class MimeHandlerHtml : public MimeHandler {
 public:
    virtual bool worker(RclConfig *conf, const string &fn, 
			const string &mtype, Rcl::Doc &docout);
    virtual bool worker1(RclConfig *conf, const string &fn, 
			 const string& htext,
			 const string &mtype, Rcl::Doc &docout);
};
#endif /* _HTML_H_INCLUDED_ */
