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
#ifndef _DOCSEQ_H_INCLUDED_
#define _DOCSEQ_H_INCLUDED_
/* @(#$Id: docseq.h,v 1.14 2008-09-08 16:49:10 dockes Exp $  (C) 2004 J.F.Dockes */
#include <string>
#include <list>
#include <vector>
#ifndef NO_NAMESPACES
using std::string;
using std::list;
using std::vector;
#endif

#include "rcldoc.h"

// A result list entry. 
struct ResListEntry {
    Rcl::Doc doc;
    int percent;
    string subHeader;
    ResListEntry() : percent(0) {}
};

/** Interface for a list of documents coming from some source.

    The result list display data may come from different sources (ie:
    history or Db query), and be post-processed (DocSeqSorted).
*/
class DocSequence {
 public:
    DocSequence(const string &t) : m_title(t) {}
    virtual ~DocSequence() {}

    /** Get document at given rank. 
     *
     * @param num document rank in sequence
     * @param doc return data
     * @param percent this will be updated with the percentage of relevance, if
     *        available, depending on the type of sequence. Return -1 in there 
     *        to indicate that the specified document data is
     *        unavailable but that there may be available data further
     *        in the sequence
     * @param sh subheader to display before this result (ie: date change 
     *           inside history)
     * @return true if ok, false for error or end of data
     */
    virtual bool getDoc(int num, Rcl::Doc &doc, int *percent, string *sh = 0) 
	= 0;

    /** Get next page of documents. This accumulates entries into the result
     *  list (doesn't reset it). */
    virtual int getSeqSlice(int offs, int cnt, vector<ResListEntry>& result);

    /** Get abstract for document. This is special because it may take time.
     *  The default is to return the input doc's abstract fields, but some 
     *  sequences can compute a better value (ie: docseqdb) */
    virtual string getAbstract(Rcl::Doc& doc) {
	return doc.meta[Rcl::Doc::keyabs];
    }

    /** Get estimated total count in results */
    virtual int getResCnt() = 0;

    /** Get title for result list */
    virtual string title() {return m_title;}

    /** Get description for underlying query */
    virtual string getDescription() = 0;

    /** Get search terms (for highlighting abstracts). Some sequences
     * may have no associated search terms. Implement this for them. */
    virtual bool getTerms(vector<string>& terms, 
			  vector<vector<string> >& groups, 
			  vector<int>& gslks) const {
	terms.clear(); groups.clear(); gslks.clear(); return true;
    }
    virtual list<string> expand(Rcl::Doc &) {list<string> e; return e;}
 private:
    string m_title;
};

#endif /* _DOCSEQ_H_INCLUDED_ */
