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
#ifndef _SORTSEQ_H_INCLUDED_
#define _SORTSEQ_H_INCLUDED_
/* @(#$Id: sortseq.h,v 1.9 2007-01-19 10:32:39 dockes Exp $  (C) 2004 J.F.Dockes */

#include <vector>
#include <string>

#include "refcntr.h"
#include "docseq.h"

class DocSeqSortSpec {
 public:
    DocSeqSortSpec() : sortwidth(0) {}
    int sortwidth; // We only re-sort the first sortwidth most relevant docs
    enum Field {RCLFLD_URL, RCLFLD_IPATH, RCLFLD_MIMETYPE, RCLFLD_MTIME};
    void addCrit(Field fld, bool desc = false) {
	crits.push_back(fld);
	dirs.push_back(desc);
    }
    std::vector<Field> crits;
    std::vector<bool> dirs;
};

/** 
 * A sorted sequence is created from the first N documents of another one, 
 * and sorts them according to the given criteria.
 */
class DocSeqSorted : public DocSequence {
 public:
    DocSeqSorted(RefCntr<DocSequence> iseq, DocSeqSortSpec &sortspec, 
		 const std::string &t);
    virtual ~DocSeqSorted() {}
    virtual bool getDoc(int num, Rcl::Doc &doc, int *percent, string *sh = 0);
    virtual int getResCnt() {return m_spec.sortwidth;}
    virtual string getAbstract(Rcl::Doc& doc);

 private:
    RefCntr<DocSequence>    m_seq;
    DocSeqSortSpec          m_spec;
    std::vector<Rcl::Doc>   m_docs;
    std::vector<Rcl::Doc *> m_docsp;
};

#endif /* _SORTSEQ_H_INCLUDED_ */
