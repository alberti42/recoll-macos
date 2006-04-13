#ifndef _STEMDB_H_INCLUDED_
#define _STEMDB_H_INCLUDED_
/* @(#$Id: stemdb.h,v 1.1 2006-04-13 09:50:03 dockes Exp $  (C) 2004 J.F.Dockes */
#ifdef RCLDB_INTERNAL
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

namespace Rcl {
namespace StemDb {

/// Get languages of existing stem databases
extern std::list<std::string> getLangs(const std::string& dbdir);
/// Delete stem database for given language
extern bool deleteDb(const std::string& dbdir, const std::string& lang);
/// Create stem database for given language
extern bool createDb(Xapian::Database& xdb, 
		     const std::string& dbdir, const std::string& lang);
/// Expand term to stem siblings
extern std::list<std::string> stemExpand(const std::string& dbdir, 
					 const std::string& lang,
					 const std::string& term);

}
}

#endif // RCLDB_INTERNAL
#endif /* _STEMDB_H_INCLUDED_ */
