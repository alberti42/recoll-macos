#ifndef _HTML_H_INCLUDED_
#define _HTML_H_INCLUDED_
/* @(#$Id: mh_html.h,v 1.5 2005-11-08 21:02:55 dockes Exp $  (C) 2004 J.F.Dockes */
#include "mimehandler.h"
#include <string>

/// Translate html document to an internal one. 
///
/// There are 2 interfaces, depending if we're working on a file, or
/// on a string. The string form is applied to the output of external
/// handlers for foreign formats: they return a result in html, which
/// has the advantage to be text (easy to use in shell-scripts), and
/// semi-structured (can carry titles, abstracts, whatever)
class MimeHandlerHtml : public MimeHandler {
 public:
    std::string charsethint;
    /// Create internal document from html file (standard interface)
    virtual MimeHandler::Status mkDoc(RclConfig *conf, const string &fn, 
			const string &mtype, Rcl::Doc &docout, string&);
    /// Create internal doc from html string (postfilter for external ones)
    virtual MimeHandler::Status mkDoc(RclConfig *conf, const string &fn, 
			 const string& htext,
			 const string &mtype, Rcl::Doc &docout);
};
#endif /* _HTML_H_INCLUDED_ */
