#ifndef _DOCSEQ_H_INCLUDED_
#define _DOCSEQ_H_INCLUDED_
/* @(#$Id: docseq.h,v 1.4 2006-01-11 15:08:22 dockes Exp $  (C) 2004 J.F.Dockes */

#include "rcldb.h"
#include "history.h"

/** Interface for a list of documents coming from some source.

    The result list display data may come from different sources (ie:
    history or Db query). We have an interface to make things cleaner.
*/
class DocSequence {
    std::string m_title;
 public:
    DocSequence(const std::string &t) : m_title(t) {}
    virtual ~DocSequence() {}
    virtual bool getDoc(int num, Rcl::Doc &doc, int *percent, string *sh = 0) 
	= 0;
    virtual int getResCnt() = 0;
    virtual std::string title() {return m_title;}
};


/** A DocSequence from a Db query (there should be one active for this
    to make sense */
class DocSequenceDb : public DocSequence {
 public:
    DocSequenceDb(Rcl::Db *d, const std::string &t) : 
	DocSequence(t), m_db(d), m_rescnt(-1) 
	{}
    virtual ~DocSequenceDb() {}
    virtual bool getDoc(int num, Rcl::Doc &doc, int *percent, string * = 0);
    virtual int getResCnt();
 private:
    Rcl::Db *m_db;
    int m_rescnt;
};

/** A DocSequence coming from the history file */
class DocSequenceHistory : public DocSequence {
 public:
    DocSequenceHistory(Rcl::Db *d, RclDHistory *h, const std::string &t) 
	: DocSequence(t), m_db(d), m_hist(h), m_prevnum(-1), m_prevtime(-1) {}
    virtual ~DocSequenceHistory() {}

    virtual bool getDoc(int num, Rcl::Doc &doc, int *percent, string *sh = 0);
    virtual int getResCnt();
 private:
    Rcl::Db *m_db;
    RclDHistory *m_hist;
    int m_prevnum;
    long m_prevtime;

    std::list<RclDHistoryEntry> m_hlist;
    std::list<RclDHistoryEntry>::const_iterator m_it;
};

#endif /* _DOCSEQ_H_INCLUDED_ */
