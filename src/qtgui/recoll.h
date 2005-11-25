#ifndef _RECOLL_H_INCLUDED_
#define _RECOLL_H_INCLUDED_
/* @(#$Id: recoll.h,v 1.6 2005-11-25 10:02:36 dockes Exp $  (C) 2004 J.F.Dockes */
#include <string>
#include <list>

#include "rclconfig.h"
#include "rcldb.h"
#include "idxthread.h"
#include "history.h"
#include "docseq.h"

// Misc declarations in need of sharing between the UI files
extern void recollCleanup();
extern void getQueryStemming(bool &dostem, std::string &stemlang);
extern bool maybeOpenDb(std::string &reason);

extern RclConfig *rclconfig;
extern Rcl::Db *rcldb;
extern std::string tmpdir;
extern bool showicons;
extern string iconsdir;
extern RclQHistory *history;
extern int recollNeedsExit;


#endif /* _RECOLL_H_INCLUDED_ */
