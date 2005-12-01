#ifndef _SORTSEQ_H_INCLUDED_
#define _SORTSEQ_H_INCLUDED_
/* @(#$Id: sortseq.h,v 1.1 2005-12-01 16:23:09 dockes Exp $  (C) 2004 J.F.Dockes */

#include <vector>
#include <string>

#include "docseq.h"

class RclSortSpec {
 public:
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
    DocSeqSorted(DocSequence &iseq, int cnt, RclSortSpec &sortspec);
    virtual ~DocSeqSorted() {}
    virtual bool getDoc(int num, Rcl::Doc &doc, int *percent, string *sh = 0);
    virtual int getResCnt() {return m_count;}
    virtual std::string title() {return m_title;}
 private:
    std::string m_title;
    int m_count;
    std::vector<Rcl::Doc> m_docs;
    std::vector<int> m_pcs;
};

#endif /* _SORTSEQ_H_INCLUDED_ */
