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
#ifndef _STEMDB_H_INCLUDED_
#define _STEMDB_H_INCLUDED_
/// Stem database code
/// 
/// Stem databases list stems and the set of index terms they expand to. They 
/// are computed from index data by stemming each term and regrouping those 
/// that stem to the same value.
/// Stem databases are stored as separate xapian databases (used as an 
/// Isam method), in subdirectories of the index.

#include <list>
#include <string>

#include <xapian.h>

#ifndef NO_NAMESPACES
using std::string;
using std::list;
namespace Rcl {
namespace StemDb {
#endif // NO_NAMESPACES

/// Get languages of existing stem databases
extern std::list<std::string> getLangs(const std::string& dbdir);
/// Delete stem database for given language
extern bool deleteDb(const std::string& dbdir, const std::string& lang);
/// Create stem database for given language
extern bool createDb(Xapian::Database& xdb, 
		     const std::string& dbdir, const std::string& lang);
/// Expand term to stem siblings
extern bool stemExpand(const std::string& dbdir, 
		       const std::string& langs,
		       const std::string& term,
		       list<string>& result);
#ifndef NO_NAMESPACES
}
}
#endif // NO_NAMESPACES

#endif /* _STEMDB_H_INCLUDED_ */
