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
/* @(#$Id: docseq.h,v 1.17 2008-09-29 11:33:55 dockes Exp $  (C) 2004 J.F.Dockes */
#include <string>
#include <list>
#include <vector>
#ifndef NO_NAMESPACES
using std::string;
using std::list;
using std::vector;
#endif

#include "rcldoc.h"
#include "refcntr.h"

// A result list entry. 
struct ResListEntry {
    Rcl::Doc doc;
    string subHeader;
};

/** Sort specification. Will be made closer to the db interface one day */
class DocSeqSortSpec {
 public:
    DocSeqSortSpec() : sortdepth(0) {}
    enum Field {RCLFLD_MIMETYPE, RCLFLD_MTIME};
    void addCrit(Field fld, bool desc = false) {
	crits.push_back(fld);
	dirs.push_back(desc);
    }
    bool isNotNull() {return sortdepth > 0;}

    int sortdepth; // We only re-sort the first sortdepth most relevant docs
    std::vector<Field> crits;
    std::vector<bool> dirs;
};

/** Filtering spec. This is only used to filter by doc category for now, hence
    the rather specialized interface */
class DocSeqFiltSpec {
 public:
    DocSeqFiltSpec() {}
    enum Crit {DSFS_MIMETYPE};
    void orCrit(Crit crit, const string& value) {
	crits.push_back(crit);
	values.push_back(value);
    }
    std::vector<Crit> crits;
    std::vector<string> values;
    void reset() {crits.clear(); values.clear();}
    bool isNotNull() {return crits.size() != 0;}
};

/** Interface for a list of documents coming from some source.

    The result list display data may come from different sources (ie:
    history or Db query), and be post-processed (DocSeqSorted).
    Additional functionality like filtering/sorting can either be
    obtained by stacking DocSequence objects (ie: sorting history), or
    by native capability (ex: docseqdb can sort and filter). The
    implementation might be nicer by using more sophisticated c++ with
    multiple inheritance of sort and filter virtual interfaces, but
    the current one will have to do for now...
*/
class DocSequence {
 public:
    DocSequence(const string &t) : m_title(t) {}
    virtual ~DocSequence() {}

    /** Get document at given rank. 
     *
     * @param num document rank in sequence
     * @param doc return data
     * @param sh subheader to display before this result (ie: date change 
     *           inside history)
     * @return true if ok, false for error or end of data
     */
    virtual bool getDoc(int num, Rcl::Doc &doc, string *sh = 0) = 0;

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
			  vector<int>& gslks) 
    {
	terms.clear(); groups.clear(); gslks.clear(); return true;
    }
    virtual list<string> expand(Rcl::Doc &) {return list<string>();}

    /** Optional functionality. Yeah, not nice */
    virtual bool canFilter() {return false;}
    virtual bool setFiltSpec(DocSeqFiltSpec &) {return false;}
    virtual bool canSort() {return false;}
    virtual bool setSortSpec(DocSeqSortSpec &) {return false;}

 private:
    string m_title;
};

/** A modifier has a child sequence which does the real work and does
 * something with the results. Some operations are just delegated
 */
class DocSeqModifier : public DocSequence {
public:
    DocSeqModifier(const string &t, RefCntr<DocSequence> iseq) 
	: DocSequence(t), m_seq(iseq) 
    {}
    virtual ~DocSeqModifier() {}

    virtual string getAbstract(Rcl::Doc& doc) 
    {
	return m_seq->getAbstract(doc);
    }
    virtual string getDescription() 
    {
	return m_seq->getDescription();
    }
    virtual bool getTerms(vector<string>& terms, 
			  vector<vector<string> >& groups, 
			  vector<int>& gslks) 
    {
	return m_seq->getTerms(terms, groups, gslks);
    }

protected:
    RefCntr<DocSequence>    m_seq;
};


#endif /* _DOCSEQ_H_INCLUDED_ */
