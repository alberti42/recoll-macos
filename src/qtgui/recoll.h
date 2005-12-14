#ifndef _RECOLL_H_INCLUDED_
#define _RECOLL_H_INCLUDED_
/* @(#$Id: recoll.h,v 1.8 2005-12-14 16:15:39 dockes Exp $  (C) 2004 J.F.Dockes */
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
extern string iconsdir;
extern RclDHistory *history;
extern int recollNeedsExit;

class QString;
extern bool prefs_showicons;
extern int prefs_respagesize;
extern QString prefs_reslistfontfamily;
extern int prefs_reslistfontsize;


#endif /* _RECOLL_H_INCLUDED_ */
