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
#ifndef _IDXTHREAD_H_INCLUDED_
#define _IDXTHREAD_H_INCLUDED_
/* @(#$Id: idxthread.h,v 1.4 2006-03-22 16:24:41 dockes Exp $  (C) 2004 J.F.Dockes */
#include <string>

class RclConfig;

// These two deal with starting / stopping the thread itself, not indexing
// sessions.
extern void start_idxthread(const RclConfig& cnf);
extern void stop_idxthread();
extern std::string idxthread_currentfile();

extern int startindexing;
extern int indexingdone;
extern bool indexingstatus;

#endif /* _IDXTHREAD_H_INCLUDED_ */
