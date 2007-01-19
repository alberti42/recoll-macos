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
#ifndef _DOCSEQHIST_H_INCLUDED_
#define _DOCSEQHIST_H_INCLUDED_
/* @(#$Id: docseqhist.h,v 1.2 2007-01-19 15:22:50 dockes Exp $  (C) 2004 J.F.Dockes */

#include "docseq.h"
#include "history.h"

namespace Rcl {
    class Db;
}

/** A DocSequence coming from the history file. 
 *  History is kept as a list of urls. This queries the db to fetch
 *  metadata for an url key */
class DocSequenceHistory : public DocSequence {
 public:
    DocSequenceHistory(Rcl::Db *d, RclHistory *h, const string &t) 
	: DocSequence(t), m_db(d), m_hist(h), m_prevnum(-1), m_prevtime(-1) {}
    virtual ~DocSequenceHistory() {}

    virtual bool getDoc(int num, Rcl::Doc &doc, int *percent, string *sh = 0);
    virtual int getResCnt();
    virtual string getDescription() {return m_description;}
    void setDescription(const string& desc) {m_description = desc;}
 private:
    Rcl::Db    *m_db;
    RclHistory *m_hist;
    int         m_prevnum;
    long        m_prevtime;
    string      m_description; // This is just an nls translated 'doc history'
    list<RclDHistoryEntry> m_hlist;
    list<RclDHistoryEntry>::const_iterator m_it;
};

#endif /* _DOCSEQ_H_INCLUDED_ */
