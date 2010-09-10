#ifndef _WASATORCL_H_INCLUDED_
#define _WASATORCL_H_INCLUDED_
/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
/* @(#$Id: wasatorcl.h,v 1.5 2007-02-07 12:00:17 dockes Exp $  (C) 2006 J.F.Dockes */

#include <string>
using std::string;

#include "rcldb.h"
#include "searchdata.h"

extern Rcl::SearchData *wasaStringToRcl(const string& query, string &reason,
                                        const string& autosuffs = string());
class WasaQuery;
extern Rcl::SearchData *wasaQueryToRcl(WasaQuery *wasa,
                                       const string& autosuffs = string());

#endif /* _WASATORCL_H_INCLUDED_ */
