#ifndef _RECOLL_H_INCLUDED_
#define _RECOLL_H_INCLUDED_
/* @(#$Id: recoll.h,v 1.3 2005-10-17 13:36:53 dockes Exp $  (C) 2004 J.F.Dockes */
#include <string>
#include <list>
#include "rclconfig.h"
#include "rcldb.h"
#include "idxthread.h"

extern void recollCleanup();

// Misc declarations in need of sharing between the UI files
extern RclConfig *rclconfig;
extern Rcl::Db *rcldb;
extern string tmpdir;

extern int recollNeedsExit;

// Holder for data collected by the advanced search dialog
struct AdvSearchData {
    std::string allwords;
    std::string phrase;
    std::string orwords;
    std::string nowords;
    std::list<std::string> filetypes; // restrict to types. Empty if inactive
    std::string topdir; // restrict to subtree. Empty if inactive
};

#endif /* _RECOLL_H_INCLUDED_ */
