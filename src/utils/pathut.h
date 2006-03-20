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
#ifndef _PATHUT_H_INCLUDED_
#define _PATHUT_H_INCLUDED_
/* @(#$Id: pathut.h,v 1.7 2006-03-20 09:54:22 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

/// Add a / at the end if none there yet.
extern void path_catslash(std::string &s);
/// Concatenate 2 paths
extern std::string path_cat(const std::string &s1, const std::string &s2);
/// Get the simple file name (get rid of any directory path prefix
extern std::string path_getsimple(const std::string &s);
/// Simple file name + optional suffix stripping
extern std::string path_basename(const std::string &s, const std::string &suff="");
/// Get the father directory
extern std::string path_getfather(const std::string &s);
/// Get the current user's home directory
extern std::string path_home();
/// Expand ~ at the beginning of string 
extern std::string path_tildexpand(const std::string &s);
/// Clean up path by removing duplicated / and resolving ../
extern std::string path_canon(const std::string &s);
/// Use glob(3) to return a list of file names matching pattern inside dir
extern std::list<std::string> path_dirglob(const std::string &dir, 
					   const std::string pattern);
#endif /* _PATHUT_H_INCLUDED_ */
