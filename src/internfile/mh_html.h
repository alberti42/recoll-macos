#ifndef _HTML_H_INCLUDED_
#define _HTML_H_INCLUDED_
/* @(#$Id: mh_html.h,v 1.6 2005-11-18 13:23:46 dockes Exp $  (C) 2004 J.F.Dockes */

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
