#ifndef _HTML_H_INCLUDED_
#define _HTML_H_INCLUDED_
/* @(#$Id: mh_html.h,v 1.2 2005-03-17 15:35:49 dockes Exp $  (C) 2004 J.F.Dockes */
#include "mimehandler.h"

// Code to turn an html document into an internal one. There are 2
// interfaces, depending if we're working on a file, or on a
// string. The string form is with external handlers for foreign
// formats: they return a result in html, which has the advantage to
// be text (easy to use in shell-scripts), and semi-structured (can
// carry titles, abstracts, whatever)
class MimeHandlerHtml : public MimeHandler {
 public:
    virtual bool worker(RclConfig *conf, const string &fn, 
			const string &mtype, Rcl::Doc &docout);
    virtual bool worker1(RclConfig *conf, const string &fn, 
			 const string& htext,
			 const string &mtype, Rcl::Doc &docout);
};
#endif /* _HTML_H_INCLUDED_ */
