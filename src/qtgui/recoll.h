#ifndef _RECOLL_H_INCLUDED_
#define _RECOLL_H_INCLUDED_
/* @(#$Id: recoll.h,v 1.1 2005-02-01 17:20:05 dockes Exp $  (C) 2004 J.F.Dockes */

#include "rclconfig.h"
#include "rcldb.h"
#include "idxthread.h"

extern void recollCleanup();

// Misc declarations in need of sharing between the UI files
extern RclConfig *rclconfig;
extern Rcl::Db *rcldb;

extern int recollNeedsExit;

#endif /* _RECOLL_H_INCLUDED_ */
