#ifndef _SEARCHDATA_H_INCLUDED_
#define _SEARCHDATA_H_INCLUDED_
/* @(#$Id: searchdata.h,v 1.5 2006-11-15 14:57:53 dockes Exp $  (C) 2004 J.F.Dockes */

#include <string>
#include <list>

#include "rcldb.h"

#ifndef NO_NAMESPACES
using std::list;
using std::string;

namespace Rcl {
#endif // NO_NAMESPACES

/** Search clause types */
enum SClType {
    SCLT_AND, 
    SCLT_OR, SCLT_EXCL, SCLT_FILENAME, SCLT_PHRASE, SCLT_NEAR,
    SCLT_SUB
};

class SearchDataClause;

/** 
 * Holder for a list of search clauses. Some of the clauses can be comples
 * subqueries.
 */
class SearchData {
 public:
    SClType                  m_tp; // Only SCLT_AND or SCLT_OR here
    list<SearchDataClause *> m_query;
    list<string>             m_filetypes; // Restrict to filetypes if set.
    string                   m_topdir; // Restrict to subtree.
    // Printable expanded version of the complete query, obtained from Xapian
    // valid after setQuery() call
    string m_description; 
    string m_reason;

    SearchData(SClType tp) : m_tp(tp) {}
    ~SearchData() {erase();}

    /** Make pristine */
    void erase();

    /** Is there anything but a file name search in here ? */
    bool fileNameOnly();

    /** Translate to Xapian query. rcldb knows about the void*  */
    bool toNativeQuery(Rcl::Db &db, void *, const string& stemlang);

    /** We become the owner of cl and will delete it */
    bool addClause(SearchDataClause *cl);

    string getReason() {return m_reason;}

 private:
    /* Copyconst and assignment private and forbidden */
    SearchData(const SearchData &) {}
    SearchData& operator=(const SearchData&) {return *this;};
};

class SearchDataClause {
 public:
    SClType m_tp;

    SearchDataClause(SClType tp) : m_tp(tp) {}
    virtual ~SearchDataClause() {}
    virtual bool toNativeQuery(Rcl::Db &db, void *, const string&) = 0;
    virtual bool isFileName() {return m_tp == SCLT_FILENAME ? true : false;}
    string getReason() {return m_reason;}
 protected:
    string m_reason;
};

class SearchDataClauseSimple : public SearchDataClause {
public:
    SearchDataClauseSimple(SClType tp, string txt)
	: SearchDataClause(tp), m_text(txt) {}
    virtual ~SearchDataClauseSimple() {}
    virtual bool toNativeQuery(Rcl::Db &db, void *, const string& stemlang);
protected:
    string  m_text;
};

class SearchDataClauseFilename : public SearchDataClauseSimple {
 public:
    SearchDataClauseFilename(string txt)
	: SearchDataClauseSimple(SCLT_FILENAME, txt) {}
    virtual ~SearchDataClauseFilename() {}
    virtual bool toNativeQuery(Rcl::Db &db, void *, const string& stemlang);
};

class SearchDataClauseDist : public SearchDataClauseSimple {
public:
    SearchDataClauseDist(SClType tp, string txt, int slack) 
	: SearchDataClauseSimple(tp, txt), m_slack(slack) {}
    virtual ~SearchDataClauseDist() {}
    virtual bool toNativeQuery(Rcl::Db &db, void *, const string& stemlang);

protected:
    int     m_slack;
};

class SearchDataClauseSub : public SearchDataClause {
 public:
    SearchDataClauseSub(SClType tp, SClType stp) 
	: SearchDataClause(tp), m_sub(stp) {}
    virtual ~SearchDataClauseSub() {}
    virtual bool toNativeQuery(Rcl::Db &db, void *, const string& stemlang);

protected:
    SearchData m_sub;
};

} // Namespace Rcl
#endif /* _SEARCHDATA_H_INCLUDED_ */
