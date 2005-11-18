#ifndef _MH_EXEC_H_INCLUDED_
#define _MH_EXEC_H_INCLUDED_
/* @(#$Id: mh_exec.h,v 1.1 2005-11-18 13:23:46 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

#include "rclconfig.h"
#include "rcldb.h"
#include "mimehandler.h"

/** 
    Turn external document into internal one by executing an external filter.
    The command to execute, and its parameters, come from the mimeconf file
*/
class MimeHandlerExec : public MimeHandler {
 public:
    std::list<std::string> params;
    virtual ~MimeHandlerExec() {}
    virtual MimeHandler::Status 
	mkDoc(RclConfig *conf, const std::string &fn, 
	      const std::string &mtype, Rcl::Doc &docout, std::string&);

};

#endif /* _MH_EXEC_H_INCLUDED_ */
