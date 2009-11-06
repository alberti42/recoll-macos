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
#ifndef _DOCSEQDB_H_INCLUDED_
#define _DOCSEQDB_H_INCLUDED_
/* @(#$Id: docseqdb.h,v 1.6 2008-09-29 11:33:55 dockes Exp $  (C) 2004 J.F.Dockes */
#include "docseq.h"
#include "refcntr.h"

#include "searchdata.h"
#include "rclquery.h"

/** A DocSequence from a Db query (there should be one active for this
    to make sense) */
class DocSequenceDb : public DocSequence {
 public:
    DocSequenceDb(RefCntr<Rcl::Query> q, const string &t, 
		  RefCntr<Rcl::SearchData> sdata);
    virtual ~DocSequenceDb();
    virtual bool getDoc(int num, Rcl::Doc &doc, string * = 0);
    virtual int getResCnt();
    virtual bool getTerms(vector<string>& terms, 
			  vector<vector<string> >& groups, 
			  vector<int>& gslks);
    virtual string getAbstract(Rcl::Doc &doc);
    virtual bool getEnclosing(Rcl::Doc& doc, Rcl::Doc& pdoc);
    virtual string getDescription();
    virtual list<string> expand(Rcl::Doc &doc);
    virtual bool canFilter() {return true;}
    virtual bool setFiltSpec(DocSeqFiltSpec &filtspec);
    virtual bool canSort() {return false;} 
    virtual bool setSortSpec(DocSeqSortSpec &sortspec);
    virtual string title();
    virtual void setAbstractParams(bool qba, bool qra)
    {
        m_queryBuildAbstract = qba;
        m_queryReplaceAbstract = qra;
    }

 private:
    RefCntr<Rcl::Query>      m_q;
    RefCntr<Rcl::SearchData> m_sdata;
    RefCntr<Rcl::SearchData> m_fsdata; // Filtered 
    int                      m_rescnt;
    bool                     m_filt;
    bool                     m_queryBuildAbstract;
    bool                     m_queryReplaceAbstract;
};

#endif /* _DOCSEQDB_H_INCLUDED_ */
