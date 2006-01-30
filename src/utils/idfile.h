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
#ifndef _IDFILE_H_INCLUDED_
#define _IDFILE_H_INCLUDED_
/* @(#$Id: idfile.h,v 1.3 2006-01-30 11:15:28 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

// Return mime type for file or empty string. The system's file utility does
// a bad job on mail folders. idFile only looks for mail file types for now, 
// but this may change
extern std::string idFile(const char *fn);

// Return all types known to us
extern std::list<std::string> idFileAllTypes();

#endif /* _IDFILE_H_INCLUDED_ */
