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
/* @(#$Id: pathut.h,v 1.13 2007-06-08 15:30:01 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>
#include "refcntr.h"

#ifndef NO_NAMESPACES
using std::string;
using std::list;
#endif

/// Add a / at the end if none there yet.
extern void path_catslash(string &s);
/// Concatenate 2 paths
extern string path_cat(const string &s1, const string &s2);
/// Get the simple file name (get rid of any directory path prefix
extern string path_getsimple(const string &s);
/// Simple file name + optional suffix stripping
extern string path_basename(const string &s, const string &suff="");
/// Get the father directory
extern string path_getfather(const string &s);
/// Get the current user's home directory
extern string path_home();
/// Expand ~ at the beginning of string 
extern string path_tildexpand(const string &s);
/// Use getcwd() to make absolute path if needed. Beware: ***this can fail***
/// we return an empty path in this case.
extern string path_absolute(const string &s);
/// Clean up path by removing duplicated / and resolving ../ + make it absolute
extern string path_canon(const string &s);
/// Use glob(3) to return a list of file names matching pattern inside dir
extern list<string> path_dirglob(const string &dir, 
					   const string pattern);
/// Encode according to rfc 1738
extern string url_encode(const string url, 
			      string::size_type offs);
/// Stat parameter and check if it's a directory
extern bool path_isdir(const string& path);

/** A small wrapper around statfs et al, to return percentage of disk
    occupation */
bool fsocc(const string &path, int *pc, // Percent occupied
	   long *avmbs = 0 // Mbs available to non-superuser
	   );

/// Create temporary directory
extern bool maketmpdir(string& tdir, string& reason);

/// Temporary file class
class TempFileInternal {
public:
    TempFileInternal(const string& suffix);
    ~TempFileInternal();
    const char *filename() {return m_filename.c_str();}
    const string &getreason() {return m_reason;}
    bool ok() {return !m_filename.empty();}
private:
    string m_filename;
    string m_reason;
};

typedef RefCntr<TempFileInternal> TempFile;

#endif /* _PATHUT_H_INCLUDED_ */
