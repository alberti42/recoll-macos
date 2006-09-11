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
#ifndef _RECOLL_H_INCLUDED_
#define _RECOLL_H_INCLUDED_
/* @(#$Id: recoll.h,v 1.16 2006-09-11 09:08:44 dockes Exp $  (C) 2004 J.F.Dockes */
#include <string>

#include "rclconfig.h"
#include "rcldb.h"
#include "idxthread.h"
#include "history.h"

// Misc declarations in need of sharing between the UI files

// Open the database if needed. We now force a close/open by default
extern bool maybeOpenDb(std::string &reason, bool force = true);

extern RclConfig *rclconfig;
extern Rcl::Db *rcldb;
extern int recollNeedsExit;
extern const std::string recoll_datadir;
extern RclHistory *g_dynconf;

#endif /* _RECOLL_H_INCLUDED_ */
