/* Copyright (C) 2004 J.F.Dockes
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
#include <string>
#include "indexer.h"
#include "rcldoc.h"

// These two deal with starting / stopping the thread itself, not
// indexing sessions.
// cnf will be cloned each time we start an indexing pass. The pointer must
// stay valid for the whole program duration.
extern void start_idxthread();
extern void stop_idxthread();

// Use these to to request action from thread
extern void start_indexing(bool rezero = false, 
			   const vector<Rcl::Doc> docs = vector<Rcl::Doc>());
extern void stop_indexing();

// Final status of indexing. indexingstatus is NULL iff indexing is
// currently in progress.
enum IdxThreadStatus {IDXTS_NULL = 0, IDXTS_OK = 1, IDXTS_ERROR = 2};
extern IdxThreadStatus idxthread_getStatus();
extern string idxthread_getReason();

// Current status of running indexing (phase, file name etc.)
extern DbIxStatus idxthread_idxStatus();
// Did last op fail because of stop request ?
extern bool idxthread_idxInterrupted(); 
#endif /* _IDXTHREAD_H_INCLUDED_ */
