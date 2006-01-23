#ifndef _RECOLL_H_INCLUDED_
#define _RECOLL_H_INCLUDED_
/* @(#$Id: recoll.h,v 1.12 2006-01-23 13:32:06 dockes Exp $  (C) 2004 J.F.Dockes */
#include <string>

#include "rclconfig.h"
#include "rcldb.h"
#include "idxthread.h"

// Misc declarations in need of sharing between the UI files
extern bool maybeOpenDb(std::string &reason);

extern RclConfig *rclconfig;
extern Rcl::Db *rcldb;
extern int recollNeedsExit;
extern const std::string recoll_datadir;

#endif /* _RECOLL_H_INCLUDED_ */
