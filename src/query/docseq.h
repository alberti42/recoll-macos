#ifndef _DOCSEQ_H_INCLUDED_
#define _DOCSEQ_H_INCLUDED_
/* @(#$Id: docseq.h,v 1.2 2005-11-28 15:31:01 dockes Exp $  (C) 2004 J.F.Dockes */

#include "rcldb.h"
#include "history.h"

/** Interface for a list of documents coming from some source.

    The result list display data may come from different sources (ie:
    history or Db query). We have an interface to make things cleaner.
*/
class DocSequence {
 public:
    virtual bool getDoc(int num, Rcl::Doc &doc, int *percent, string *sh = 0) 
	= 0;
    virtual int getResCnt() = 0;
    virtual std::string title() = 0;
};


/** A DocSequence from a Db query (there should be one active for this
    to make sense */
class DocSequenceDb : public DocSequence {
 public:
    DocSequenceDb(Rcl::Db *d) : db(d) {}
    virtual ~DocSequenceDb() {}
    virtual bool getDoc(int num, Rcl::Doc &doc, int *percent, string * = 0);
    virtual int getResCnt();
    virtual std::string title() {return string("Query results");}
 private:
    Rcl::Db *db;
};

/** A DocSequence coming from the history file */
class DocSequenceHistory : public DocSequence {
 public:
    DocSequenceHistory(Rcl::Db *d, RclDHistory *h) 
	: db(d), hist(h), prevnum(-1), prevtime(-1) {}
    virtual ~DocSequenceHistory() {}

    virtual bool getDoc(int num, Rcl::Doc &doc, int *percent, string *sh = 0);
    virtual int getResCnt();
    virtual std::string title() {return string("Document history");}
 private:
    Rcl::Db *db;
    RclDHistory *hist;
    int prevnum;
    long prevtime;

    std::list<RclDHistoryEntry> hlist;
    std::list<RclDHistoryEntry>::const_iterator it;
};

#endif /* _DOCSEQ_H_INCLUDED_ */
