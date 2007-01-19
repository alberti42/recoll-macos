#ifndef _WASATORCL_H_INCLUDED_
#define _WASATORCL_H_INCLUDED_
/* @(#$Id: wasatorcl.h,v 1.4 2007-01-19 10:22:06 dockes Exp $  (C) 2006 J.F.Dockes */

#include <string>
using std::string;

#include "rcldb.h"
#include "searchdata.h"

extern Rcl::SearchData *wasaStringToRcl(const string& query, string &reason);
class WasaQuery;
extern Rcl::SearchData *wasaQueryToRcl(WasaQuery *wasa);

#endif /* _WASATORCL_H_INCLUDED_ */
