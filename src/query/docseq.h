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
/* @(#$Id: docseq.h,v 1.9 2006-09-13 14:57:56 dockes Exp $  (C) 2004 J.F.Dockes */
#include <string>
#include <list>

#ifndef NO_NAMESPACES
using std::string;
using std::list;
#endif

#include "rcldb.h"
#include "history.h"

/** Interface for a list of documents coming from some source.

    The result list display data may come from different sources (ie:
    history or Db query). We have an interface to make things cleaner.
*/
class DocSequence {
 public:
    DocSequence(const string &t) : m_title(t) {}
    virtual ~DocSequence() {}
    /** Get document at given rank 
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
    virtual int getResCnt() = 0;
    virtual string title() {return m_title;}
    virtual void getTerms(list<string>& t) {t.clear();}

 private:
    string m_title;
};


/** A DocSequence from a Db query (there should be one active for this
    to make sense */
class DocSequenceDb : public DocSequence {
 public:
    DocSequenceDb(Rcl::Db *d, const string &t) : 
	DocSequence(t), m_db(d), m_rescnt(-1) 
	{}
    virtual ~DocSequenceDb() {}
    virtual bool getDoc(int num, Rcl::Doc &doc, int *percent, string * = 0);
    virtual int getResCnt();
    virtual void getTerms(list<string>&);

 private:
    Rcl::Db *m_db;
    int m_rescnt;
};

/** A DocSequence coming from the history file */
class DocSequenceHistory : public DocSequence {
 public:
    DocSequenceHistory(Rcl::Db *d, RclHistory *h, const string &t) 
	: DocSequence(t), m_db(d), m_hist(h), m_prevnum(-1), m_prevtime(-1) {}
    virtual ~DocSequenceHistory() {}

    virtual bool getDoc(int num, Rcl::Doc &doc, int *percent, string *sh = 0);
    virtual int getResCnt();
 private:
    Rcl::Db *m_db;
    RclHistory *m_hist;
    int m_prevnum;
    long m_prevtime;

    list<RclDHistoryEntry> m_hlist;
    list<RclDHistoryEntry>::const_iterator m_it;
};

#endif /* _DOCSEQ_H_INCLUDED_ */
