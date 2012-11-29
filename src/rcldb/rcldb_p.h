/* Copyright (C) 2007 J.F.Dockes
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

#ifndef _rcldb_p_h_included_
#define _rcldb_p_h_included_

#include "autoconfig.h"

#include <map>

#include <xapian.h>

#ifdef IDX_THREADS
#include "workqueue.h"
#endif // IDX_THREADS
#include "debuglog.h"
#include "xmacros.h"
#include "ptmutex.h"

namespace Rcl {

class Query;

#ifdef IDX_THREADS
// Task for the index update thread. This can be either and add/update
// or a purge op, in which case the txtlen is (size_t)-1
class DbUpdTask {
public:
    DbUpdTask(const string& ud, const string& un, const Xapian::Document &d,
	size_t tl)
	: udi(ud), uniterm(un), doc(d), txtlen(tl)
    {}
    string udi;
    string uniterm;
    Xapian::Document doc;
    size_t txtlen;
};
#endif // IDX_THREADS

// A class for data and methods that would have to expose
// Xapian-specific stuff if they were in Rcl::Db. There could actually be
// 2 different ones for indexing or query as there is not much in
// common.
class Db::Native {
 public:
    Db  *m_rcldb; // Parent
    bool m_isopen;
    bool m_iswritable;
    bool m_noversionwrite; //Set if open failed because of version mismatch!
#ifdef IDX_THREADS
    WorkQueue<DbUpdTask*> m_wqueue;
    int  m_loglevel;
    PTMutexInit m_mutex;
    long long  m_totalworkns;
    bool m_haveWriteQ;
    void maybeStartThreads();
#endif // IDX_THREADS

    // Indexing 
    Xapian::WritableDatabase xwdb;
    // Querying (active even if the wdb is too)
    Xapian::Database xrdb;

    Native(Db *db);
    ~Native();

#ifdef IDX_THREADS
    friend void *DbUpdWorker(void*);
#endif // IDX_THREADS

    // Final steps of doc update, part which need to be single-threaded
    bool addOrUpdateWrite(const string& udi, const string& uniterm, 
			  Xapian::Document& doc, size_t txtlen);

    bool getPagePositions(Xapian::docid docid, vector<int>& vpos);
    int getPageNumberForPosition(const vector<int>& pbreaks, unsigned int pos);

    bool dbDataToRclDoc(Xapian::docid docid, std::string &data, Doc &doc);

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

// This is the word position offset at which we index the body text
// (abstract, keywords, etc.. are stored before this)
static const unsigned int baseTextPosition = 100000;

}
#endif /* _rcldb_p_h_included_ */
