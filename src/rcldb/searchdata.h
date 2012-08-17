/* Copyright (C) 2004 J.F.Dockes
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
#ifndef _SEARCHDATA_H_INCLUDED_
#define _SEARCHDATA_H_INCLUDED_

/** 
 * Structures to hold data coming almost directly from the gui
 * and handle its translation to Xapian queries.
 * This is not generic code, it reflects the choices made for the user 
 * interface, and it also knows some specific of recoll's usage of Xapian 
 * (ie: term prefixes)
 */
#include <string>
#include <vector>

#include "rcldb.h"
#include "refcntr.h"
#include "smallut.h"
#include "cstr.h"
#include "hldata.h"

class RclConfig;

namespace Rcl {

/** Search clause types */
enum SClType {
    SCLT_AND, 
    SCLT_OR, SCLT_EXCL, SCLT_FILENAME, SCLT_PHRASE, SCLT_NEAR,
    SCLT_SUB
};

class SearchDataClause;

/** 
    Data structure representing a Recoll user query, for translation
    into a Xapian query tree. This could probably better called a 'question'.

    This is a list of search clauses combined through either OR or AND.

    Clauses either reflect user entry in a query field: some text, a
    clause type (AND/OR/NEAR etc.), possibly a distance, or points to
    another SearchData representing a subquery.

    The content of each clause when added may not be fully parsed yet
    (may come directly from a gui field). It will be parsed and may be
    translated to several queries in the Xapian sense, for exemple
    several terms and phrases as would result from 
    ["this is a phrase"  term1 term2] . 

    This is why the clauses also have an AND/OR/... type. 

    A phrase clause could be added either explicitly or using double quotes:
    {SCLT_PHRASE, [this is a phrase]} or as {SCLT_XXX, ["this is a phrase"]}

*/
class SearchData {
public:
    SearchData(SClType tp) 
    : m_tp(tp), m_topdirexcl(false), m_topdirweight(1.0), 
      m_haveDates(false), m_maxSize(size_t(-1)),
      m_minSize(size_t(-1)), m_haveWildCards(false) 
    {
	if (m_tp != SCLT_OR && m_tp != SCLT_AND) 
	    m_tp = SCLT_OR;
    }
    ~SearchData() {erase();}

    /** Make pristine */
    void erase();

    /** Is there anything but a file name search in here ? */
    bool fileNameOnly();

    /** Do we have wildcards anywhere apart from filename searches ? */
    bool haveWildCards() {return m_haveWildCards;}

    /** Translate to Xapian query. rcldb knows about the void*  */
    bool toNativeQuery(Rcl::Db &db, void *);

    /** We become the owner of cl and will delete it */
    bool addClause(SearchDataClause *cl);

    /** If this is a simple query (one field only, no distance clauses),
     * add phrase made of query terms to query, so that docs containing the
     * user terms in order will have higher relevance. This must be called 
     * before toNativeQuery().
     * @param threshold: don't use terms more frequent than the value 
     *     (proportion of docs where they occur) 	
     */
    bool maybeAddAutoPhrase(Rcl::Db &db, double threshold);

    /** Set/get top subdirectory for filtering results */
    void setTopdir(const std::string& t, bool excl = false, float w = 1.0) 
    {
	m_topdir = t;
	m_topdirexcl = excl;
	m_topdirweight = w;
    }

    void setMinSize(size_t size) {m_minSize = size;}
    void setMaxSize(size_t size) {m_maxSize = size;}

    /** Set date span for filtering results */
    void setDateSpan(DateInterval *dip) {m_dates = *dip; m_haveDates = true;}

    /** Add file type for filtering results */
    void addFiletype(const std::string& ft) {m_filetypes.push_back(ft);}
    /** Add file type to not wanted list */
    void remFiletype(const std::string& ft) {m_nfiletypes.push_back(ft);}

    void setStemlang(const std::string& lang = "english") {m_stemlang = lang;}

    /** Retrieve error description */
    std::string getReason() {return m_reason;}

    /** Return term expansion data. Mostly used by caller for highlighting
     */
    void getTerms(HighlightData& hldata) const;

    /** 
     * Get/set the description field which is retrieved from xapian after
     * initializing the query. It is stored here for usage in the GUI.
     */
    std::string getDescription() {return m_description;}
    void setDescription(const std::string& d) {m_description = d;}

private:
    // Combine type. Only SCLT_AND or SCLT_OR here
    SClType                   m_tp; 
    // Complex query descriptor
    std::vector<SearchDataClause*> m_query;
    // Restricted set of filetypes if not empty.
    std::vector<std::string>            m_filetypes; 
    // Excluded set of file types if not empty
    std::vector<std::string>            m_nfiletypes;
    // Restrict to subtree.
    std::string                    m_topdir; 
    bool                      m_topdirexcl; // Invert meaning
    float                     m_topdirweight; // affect weight instead of filter
    bool                      m_haveDates;
    DateInterval              m_dates; // Restrict to date interval
    size_t                    m_maxSize;
    size_t                    m_minSize;
    // Printable expanded version of the complete query, retrieved/set
    // from rcldb after the Xapian::setQuery() call
    std::string m_description; 
    std::string m_reason;
    bool   m_haveWildCards;
    std::string m_stemlang;
    bool expandFileTypes(RclConfig *cfg, std::vector<std::string>& exptps);
    /* Copyconst and assignment private and forbidden */
    SearchData(const SearchData &) {}
    SearchData& operator=(const SearchData&) {return *this;};
};

class SearchDataClause {
public:
    enum Modifier {SDCM_NONE=0, SDCM_NOSTEMMING=1, SDCM_ANCHORSTART=2,
		   SDCM_ANCHOREND=4};

    SearchDataClause(SClType tp) 
    : m_tp(tp), m_parentSearch(0), m_haveWildCards(0), 
      m_modifiers(SDCM_NONE), m_weight(1.0)
    {}
    virtual ~SearchDataClause() {}
    virtual bool toNativeQuery(Rcl::Db &db, void *, const std::string&) = 0;
    bool isFileName() const {return m_tp == SCLT_FILENAME ? true: false;}
    virtual std::string getReason() const {return m_reason;}
    virtual void getTerms(HighlightData & hldata) const = 0;

    SClType getTp() 
    {
	return m_tp;
    }
    void setParent(SearchData *p) 
    {
	m_parentSearch = p;
    }
    virtual void setModifiers(Modifier mod) 
    {
	m_modifiers = mod;
    }
    virtual int getModifiers() 
    {
	return m_modifiers;
    }
    virtual void addModifier(Modifier mod) 
    {
	int imod = getModifiers();
	imod |= mod;
	setModifiers(Modifier(imod));
    }
    virtual void setWeight(float w) 
    {
	m_weight = w;
    }
    friend class SearchData;

protected:
    std::string      m_reason;
    SClType     m_tp;
    SearchData *m_parentSearch;
    bool        m_haveWildCards;
    Modifier    m_modifiers;
    float       m_weight;
private:
    SearchDataClause(const SearchDataClause&) 
    {
    }
    SearchDataClause& operator=(const SearchDataClause&) 
    {
	return *this;
    }
};
    
/**
 * "Simple" data clause with user-entered query text. This can include 
 * multiple phrases and words, but no specified distance.
 */
class SearchDataClauseSimple : public SearchDataClause {
public:
    SearchDataClauseSimple(SClType tp, const std::string& txt, 
			   const std::string& fld = std::string())
	: SearchDataClause(tp), m_text(txt), m_field(fld)
    {
	m_haveWildCards = 
	    (txt.find_first_of(cstr_minwilds) != std::string::npos);
    }

    virtual ~SearchDataClauseSimple() 
    {
    }

    /** Translate to Xapian query */
    virtual bool toNativeQuery(Rcl::Db &, void *, const std::string& stemlang);

    virtual void getTerms(HighlightData& hldata) const
    {
	hldata.append(m_hldata);
    }
    virtual const std::string& gettext() 
    {
	return m_text;
    }
    virtual const std::string& getfield() 
    {
	return m_field;
    }
protected:
    std::string  m_text;  // Raw user entry text.
    std::string  m_field; // Field specification if any
    HighlightData m_hldata;
};

/** 
 * Filename search clause. This is special because term expansion is only
 * performed against the unsplit file name terms. 
 *
 * There is a big advantage in expanding only against the
 * field, especially for file names, because this makes searches for
 * "*xx" much faster (no need to scan the whole main index).
 */
class SearchDataClauseFilename : public SearchDataClauseSimple {
public:
    SearchDataClauseFilename(const std::string& txt)
	: SearchDataClauseSimple(SCLT_FILENAME, txt) 
    {
	// File name searches don't count when looking for wild cards.
	m_haveWildCards = false;
    }

    virtual ~SearchDataClauseFilename() 
    {
    }

    virtual bool toNativeQuery(Rcl::Db &, void *, const std::string& stemlang);
};

/** 
 * A clause coming from a NEAR or PHRASE entry field. There is only one 
 * std::string group, and a specified distance, which applies to it.
 */
class SearchDataClauseDist : public SearchDataClauseSimple {
public:
    SearchDataClauseDist(SClType tp, const std::string& txt, int slack, 
			 const std::string& fld = std::string())
	: SearchDataClauseSimple(tp, txt, fld), m_slack(slack)
    {
    }

    virtual ~SearchDataClauseDist() 
    {
    }

    virtual bool toNativeQuery(Rcl::Db &, void *, const std::string& stemlang);
private:
    int m_slack;
};

/** Subquery */
class SearchDataClauseSub : public SearchDataClause {
public:
    // We take charge of the SearchData * and will delete it.
    SearchDataClauseSub(SClType tp, RefCntr<SearchData> sub) 
	: SearchDataClause(tp), m_sub(sub) 
    {
    }

    virtual ~SearchDataClauseSub() 
    {
    }

    virtual bool toNativeQuery(Rcl::Db &db, void *p, const std::string&)
    {
	return m_sub->toNativeQuery(db, p);
    }

    virtual void getTerms(HighlightData& hldata) const
    {
	m_sub.getconstptr()->getTerms(hldata);
    }

protected:
    RefCntr<SearchData> m_sub;
};

} // Namespace Rcl

#endif /* _SEARCHDATA_H_INCLUDED_ */
