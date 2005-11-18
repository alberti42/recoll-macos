#ifndef _MH_TEXT_H_INCLUDED_
#define _MH_TEXT_H_INCLUDED_
/* @(#$Id: mh_text.h,v 1.1 2005-11-18 13:23:46 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>

#include "rclconfig.h"
#include "rcldb.h"
#include "mimehandler.h"

/**
 * Handler for text/plain files. 
 *
 * Maybe try to guess charset, or use default, then transcode to utf8
 */
class MimeHandlerText : public MimeHandler {
 public:
    MimeHandler::Status mkDoc(RclConfig *conf, const std::string &fn, 
			      const std::string &mtype, Rcl::Doc &docout, 
			      std::string&);
    
};

#endif /* _MH_TEXT_H_INCLUDED_ */
