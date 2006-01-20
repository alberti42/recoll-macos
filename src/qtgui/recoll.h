#ifndef _RECOLL_H_INCLUDED_
#define _RECOLL_H_INCLUDED_
/* @(#$Id: recoll.h,v 1.11 2006-01-20 14:58:57 dockes Exp $  (C) 2004 J.F.Dockes */
#include <string>

#include "rclconfig.h"
#include "rcldb.h"
#include "idxthread.h"

// Misc declarations in need of sharing between the UI files
extern bool maybeOpenDb(std::string &reason);
extern bool startHelpBrowser(const string& url = "");

extern RclConfig *rclconfig;
extern Rcl::Db *rcldb;
extern int recollNeedsExit;

class QString;
extern bool prefs_showicons;
extern int prefs_respagesize;
extern QString prefs_reslistfontfamily;
extern int prefs_reslistfontsize;
extern QString prefs_queryStemLang;

#endif /* _RECOLL_H_INCLUDED_ */
