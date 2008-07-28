#ifndef _rcldb_p_h_included_
#define _rcldb_p_h_included_

#include "xapian.h"

namespace Rcl {
/* @(#$Id: rcldb_p.h,v 1.2 2008-07-28 08:42:52 dockes Exp $  (C) 2007 J.F.Dockes */

// Generic Xapian exception catching code. We do this quite often,
// and I have no idea how to do this except for a macro
#define XCATCHERROR(MSG) \
 catch (const Xapian::Error &e) {		   \
    MSG = e.get_msg();				   \
    if (MSG.empty()) MSG = "Empty error message";  \
 } catch (const string &s) {			   \
    MSG = s;					   \
    if (MSG.empty()) MSG = "Empty error message";  \
 } catch (const char *s) {			   \
    MSG = s;					   \
    if (MSG.empty()) MSG = "Empty error message";  \
 } catch (...) {				   \
    MSG = "Caught unknown xapian exception";	   \
 } 

class Query;

// A class for data and methods that would have to expose
// Xapian-specific stuff if they were in Rcl::Db. There could actually be
// 2 different ones for indexing or query as there is not much in
// common.
class Db::Native {
 public:
    Db *m_db;
    bool m_isopen;
    bool m_iswritable;

    // Indexing
    Xapian::WritableDatabase wdb;

    // Querying
    Xapian::Database db;
    
    Native(Db *db) 
	: m_db(db), m_isopen(false), m_iswritable(false)
    { }

    ~Native() {
    }

    string makeAbstract(Xapian::docid id, Query *query);

    bool dbDataToRclDoc(Xapian::docid docid, std::string &data, Doc &doc);

    /** Compute list of subdocuments for a given path (given by hash) 
     *  We look for all Q terms beginning with the path/hash
     *  As suggested by James Aylett, a better method would be to add 
     *  a single term (ie: XP/path/to/file) to all subdocs, then finding
     *  them would be a simple matter of retrieving the posting list for the
     *  term. There would still be a need for the current Qterm though, as a
     *  unique term for replace_document, and for retrieving by
     *  path/ipath (history)
     */
    bool subDocs(const string &uniterm, vector<Xapian::docid>& docids);

};
}
#endif /* _rcldb_p_h_included_ */
