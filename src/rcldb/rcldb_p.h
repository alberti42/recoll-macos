#ifndef _rcldb_p_h_included_
#define _rcldb_p_h_included_

#include "xapian.h"

namespace Rcl {
/* @(#$Id: rcldb_p.h,v 1.5 2008-09-16 08:18:30 dockes Exp $  (C) 2007 J.F.Dockes */

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

    bool dbDataToRclDoc(Xapian::docid docid, std::string &data, Doc &doc,
			int percent);

    /** Compute list of subdocuments for a given udi. We look for documents 
     * indexed by a parent term matching the udi, the posting list for the 
     * parentterm(udi)  (As suggested by James Aylett)
     *
     * Note that this is not currently recursive: all subdocs are supposed 
     * to be children of the file doc.
     * Ie: in a mail folder, all messages, attachments, attachments of
     * attached messages etc. must have the folder file document as
     * parent. 
     * Parent-child relationships are defined by the indexer (rcldb user)
     * 
     * The file-system indexer currently works this way (flatly), 
     * subDocs() could be relatively easily changed to support full recursivity
     * if needed.
     */
    bool subDocs(const string &udi, vector<Xapian::docid>& docids);

};

// Field names inside the index data record may differ from the rcldoc ones
// (esp.: caption / title)
inline const string& docfToDatf(const string& df)
{
    static const string keycap("caption");
    return df.compare(Doc::keytt) ? df : keycap;
}

}
#endif /* _rcldb_p_h_included_ */
