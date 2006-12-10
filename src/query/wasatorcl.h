#ifndef _WASATORCL_H_INCLUDED_
#define _WASATORCL_H_INCLUDED_
/* @(#$Id: wasatorcl.h,v 1.3 2006-12-10 17:03:08 dockes Exp $  (C) 2006 J.F.Dockes */

#include <string>
using std::string;

#include "rcldb.h"
#include "searchdata.h"

extern Rcl::SearchData *wasaQueryToRcl(WasaQuery *wasa);

#endif /* _WASATORCL_H_INCLUDED_ */
