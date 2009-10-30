#ifndef lint
static char rcsid[] = "@(#$Id: rclquery.cpp,v 1.11 2008-12-19 09:55:36 dockes Exp $ (C) 2008 J.F.Dockes";
#endif

#include <stdlib.h>
#include <string.h>

#include <list>
#include <vector>

#include "xapian/sorter.h"

#include "rcldb.h"
#include "rcldb_p.h"
#include "rclquery.h"
#include "rclquery_p.h"
#include "debuglog.h"
#include "conftree.h"
#include "smallut.h"
#include "searchdata.h"
#include "rclconfig.h"

#ifndef NO_NAMESPACES
namespace Rcl {
#endif


class FilterMatcher : public Xapian::MatchDecider {
public:
    FilterMatcher(const string &topdir)
	: m_topdir(topdir)
    {}
    virtual ~FilterMatcher() {}

    virtual 
#if XAPIAN_MAJOR_VERSION < 1
    int 
#else
    bool
#endif
    operator()(const Xapian::Document &xdoc) const 
    {
	// Parse xapian document's data and populate doc fields
	string data = xdoc.get_data();
	ConfSimple parms(data);

	// The only filtering for now is on file path (subtree)
	string url;
	parms.get(Doc::keyurl, url);
	LOGDEB2(("FilterMatcher topdir [%s] url [%s]\n",
		 m_topdir.c_str(), url.c_str()));
	if (url.find(m_topdir, 7) == 7) {
	    return true; 
	} else {
	    return false;
	}
    }
    
private:
    string m_topdir;
};

// Sort helper class
class QSorter : public Xapian::Sorter {
public:
    QSorter(const string& f) : m_fld(docfToDatf(f) + "=") {}

    virtual std::string operator()(const Xapian::Document& xdoc) const {
	string data = xdoc.get_data();

	// It would be simpler to do the record->Rcl::Doc thing, but
	// hand-doing this will be faster. It makes more assumptions
	// about the format than a ConfTree though:
	string::size_type i1, i2;
	i1 = data.find(m_fld);
	if (i1 == string::npos) 
	    return string();
	i1 += m_fld.length();
	if (i1 >= data.length())
	    return string();
	i2 = data.find_first_of("\n\r", i1);
	if (i2 == string::npos)
	    return string();
	return data.substr(i1, i2-i1);
    }

private:
    string m_fld;
};

Query::Query(Db *db)
    : m_nq(new Native(this)), m_db(db), m_sorter(0), m_sortAscending(true),
      m_collapseDuplicates(false)
{
}

Query::~Query()
{
    deleteZ(m_nq);
    if (m_sorter) {
	delete (QSorter*)m_sorter;
	m_sorter = 0;
    }
}

string Query::getReason() const
{
    return m_reason;
}

Db *Query::whatDb() 
{
    return m_db;
}

void Query::setSortBy(const string& fld, bool ascending) {
    m_sortField = m_db->getConf()->fieldCanon(fld);
    m_sortAscending = ascending;
    LOGDEB0(("RclQuery::setSortBy: [%s] %s\n", m_sortField.c_str(),
	     m_sortAscending ? "ascending" : "descending"));
}

//#define ISNULL(X) (X).isNull()
#define ISNULL(X) !(X)

// Prepare query out of user search data
bool Query::setQuery(RefCntr<SearchData> sdata)
{
    LOGDEB(("Query::setQuery:\n"));

    if (!m_db || ISNULL(m_nq)) {
	LOGERR(("Query::setQuery: not initialised!\n"));
	return false;
    }
    m_reason.erase();

    m_filterTopDir = sdata->getTopdir();
    m_nq->clear();

    if (!m_filterTopDir.empty()) {
#if XAPIAN_FILTERING
	m_nq->decider = 
#else
        m_nq->postfilter =
#endif
	    new FilterMatcher(m_filterTopDir);
    }

    Xapian::Query xq;
    if (!sdata->toNativeQuery(*m_db, &xq)) {
	m_reason += sdata->getReason();
	return false;
    }
    m_nq->xquery = xq;

    string d;
    for (int tries = 0; tries < 2; tries++) {
	try {
            m_nq->xenquire = new Xapian::Enquire(m_db->m_ndb->xrdb);
            if (m_collapseDuplicates) {
                m_nq->xenquire->set_collapse_key(Rcl::VALUE_MD5);
            } else {
                m_nq->xenquire->set_collapse_key(Xapian::BAD_VALUENO);
            }
            if (!m_sortField.empty()) {
                if (m_sorter) {
                    delete (QSorter*)m_sorter;
                    m_sorter = 0;
                }
                m_sorter = new QSorter(m_sortField);
                // It really seems there is a xapian bug about sort order, we 
                // invert here.
                m_nq->xenquire->set_sort_by_key((QSorter*)m_sorter, 
                                                !m_sortAscending);
            }
            m_nq->xenquire->set_query(m_nq->xquery);
            m_nq->xmset = Xapian::MSet();
            // Get the query description and trim the "Xapian::Query"
            d = m_nq->xquery.get_description();
            m_reason.erase();
            break;
	} catch (const Xapian::DatabaseModifiedError &e) {
            m_reason = e.get_msg();
	    m_db->m_ndb->xrdb.reopen();
            continue;
	} XCATCHERROR(m_reason);
        break;
    }

    if (!m_reason.empty()) {
	LOGDEB(("Query::SetQuery: xapian error %s\n", m_reason.c_str()));
	return false;
    }
	
    if (d.find("Xapian::Query") == 0)
	d.erase(0, strlen("Xapian::Query"));
    if (!m_filterTopDir.empty()) {
	d += string(" [dir: ") + m_filterTopDir + "]";
    }
    sdata->setDescription(d);
    LOGDEB(("Query::SetQuery: Q: %s\n", sdata->getDescription().c_str()));
    return true;
}


bool Query::getQueryTerms(list<string>& terms)
{
    if (ISNULL(m_nq))
	return false;

    terms.clear();
    Xapian::TermIterator it;
    string ermsg;
    try {
	for (it = m_nq->xquery.get_terms_begin(); 
	     it != m_nq->xquery.get_terms_end(); it++) {
	    terms.push_back(*it);
	}
    } XCATCHERROR(ermsg);
    if (!ermsg.empty()) {
	LOGERR(("getQueryTerms: xapian error: %s\n", ermsg.c_str()));
	return false;
    }
    return true;
}

bool Query::getMatchTerms(const Doc& doc, list<string>& terms)
{
    if (ISNULL(m_nq) || !m_nq->xenquire) {
	LOGERR(("Query::getMatchTerms: no query opened\n"));
	return -1;
    }

    terms.clear();
    Xapian::TermIterator it;
    Xapian::docid id = Xapian::docid(doc.xdocid);

    XAPTRY(terms.insert(terms.begin(),
                        m_nq->xenquire->get_matching_terms_begin(id),
                        m_nq->xenquire->get_matching_terms_end(id)),
           m_db->m_ndb->xrdb, m_reason);

    if (!m_reason.empty()) {
	LOGERR(("getQueryTerms: xapian error: %s\n", m_reason.c_str()));
	return false;
    }

    return true;
}

// Mset size
static const int qquantum = 30;

// Get estimated result count for query. Xapian actually does most of
// the search job in there, this can be long
int Query::getResCnt()
{
    if (ISNULL(m_nq) || !m_nq->xenquire) {
	LOGERR(("Query::getResCnt: no query opened\n"));
	return -1;
    }

    int ret = -1;
    if (m_nq->xmset.size() <= 0) {
        Chrono chron;

        XAPTRY(m_nq->xmset = 
               m_nq->xenquire->get_mset(0, qquantum,0, m_nq->decider);
               ret = m_nq->xmset.get_matches_lower_bound(),
               m_db->m_ndb->xrdb, m_reason);

        LOGDEB(("Query::getResCnt: %d mS\n", chron.millis()));
	if (!m_reason.empty())
	    LOGERR(("xenquire->get_mset: exception: %s\n", m_reason.c_str()));
    }
    return ret;
}


// Get document at rank i in query (i is the index in the whole result
// set, as in the enquire class. We check if the current mset has the
// doc, else ask for an other one. We use msets of 10 documents. Don't
// know if the whole thing makes sense at all but it seems to work.
//
// If there is a postquery filter (ie: file names), we have to
// maintain a correspondance from the sequential external index
// sequence to the internal Xapian hole-y one (the holes being the documents 
// that dont match the filter).
bool Query::getDoc(int exti, Doc &doc)
{
    LOGDEB1(("Query::getDoc: exti %d\n", exti));
    if (ISNULL(m_nq) || !m_nq->xenquire) {
	LOGERR(("Query::getDoc: no query opened\n"));
	return false;
    }

    int xapi;
    if (m_nq->postfilter) {
	// There is a postquery filter, does this fall in already known area ?
	if (exti >= (int)m_nq->m_dbindices.size()) {
	    // Have to fetch xapian docs and filter until we get
	    // enough or fail
	    m_nq->m_dbindices.reserve(exti+1);
	    // First xapian doc we fetch is the one after last stored 
	    int first = m_nq->m_dbindices.size() > 0 ? 
		m_nq->m_dbindices.back() + 1 : 0;
	    // Loop until we get enough docs
	    while (exti >= (int)m_nq->m_dbindices.size()) {
		LOGDEB(("Query::getDoc: fetching %d starting at %d\n",
			qquantum, first));

		XAPTRY(m_nq->xmset = m_nq->xenquire->get_mset(first, qquantum),
                       m_db->m_ndb->xrdb, m_reason);

                if (!m_reason.empty()) {
                    LOGERR(("enquire->get_mset: exception: %s\n", 
                            m_reason.c_str()));
                    return false;
		}

		if (m_nq->xmset.empty()) {
		    LOGDEB(("Query::getDoc: got empty mset\n"));
		    return false;
		}
		first = m_nq->xmset.get_firstitem();
		for (unsigned int i = 0; i < m_nq->xmset.size() ; i++) {
		    LOGDEB(("Query::getDoc: [%d]\n", i));
		    Xapian::Document xdoc = m_nq->xmset[i].get_document();
		    if ((*m_nq->postfilter)(xdoc)) {
			m_nq->m_dbindices.push_back(first + i);
		    }
		}
		first = first + m_nq->xmset.size();
	    }
	}
	xapi = m_nq->m_dbindices[exti];
    } else {
	xapi = exti;
    }

    // From there on, we work with a xapian enquire item number. Fetch it
    int first = m_nq->xmset.get_firstitem();
    int last = first + m_nq->xmset.size() -1;

    if (!(xapi >= first && xapi <= last)) {
	LOGDEB(("Fetching for first %d, count %d\n", xapi, qquantum));

	XAPTRY(m_nq->xmset = m_nq->xenquire->get_mset(xapi, qquantum,
                                                      0, m_nq->decider),
               m_db->m_ndb->xrdb, m_reason);

        if (!m_reason.empty()) {
            LOGERR(("enquire->get_mset: exception: %s\n", m_reason.c_str()));
            return false;
	}
	if (m_nq->xmset.empty()) {
            LOGDEB(("enquire->get_mset: got empty result\n"));
	    return false;
        }
	first = m_nq->xmset.get_firstitem();
	last = first + m_nq->xmset.size() -1;
    }

    LOGDEB1(("Query::getDoc: Qry [%s] win [%d-%d] Estimated results: %d",
            m_nq->query.get_description().c_str(), 
            first, last, m_nq->xmset.get_matches_lower_bound()));

    Xapian::Document xdoc;
    Xapian::docid docid = 0;
    int pc = 0;
    string data;
    m_reason.erase();
    for (int xaptries=0; xaptries < 2; xaptries++) {
        try {
            xdoc = m_nq->xmset[xapi-first].get_document();
            docid = *(m_nq->xmset[xapi-first]);
            pc = m_nq->xmset.convert_to_percent(m_nq->xmset[xapi-first]);
            data = xdoc.get_data();
            m_reason.erase();
            break;
        } catch (Xapian::DatabaseModifiedError &error) {
            // retry or end of loop
            m_reason = error.get_msg();
            continue;
        }
        XCATCHERROR(m_reason);
        break;
    }
    if (!m_reason.empty()) {
        LOGERR(("Query::getDoc: %s\n", m_reason.c_str()));
        return false;
    }
    // Parse xapian document's data and populate doc fields
    return m_db->m_ndb->dbDataToRclDoc(docid, data, doc, pc);
}

list<string> Query::expand(const Doc &doc)
{
    list<string> res;
    if (ISNULL(m_nq) || !m_nq->xenquire) {
	LOGERR(("Query::expand: no query opened\n"));
	return res;
    }

    for (int tries = 0; tries < 2; tries++) {
	try {
	    Xapian::RSet rset;
	    rset.add_document(Xapian::docid(doc.xdocid));
	    // We don't exclude the original query terms.
	    Xapian::ESet eset = m_nq->xenquire->get_eset(20, rset, false);
	    LOGDEB(("ESet terms:\n"));
	    // We filter out the special terms
	    for (Xapian::ESetIterator it = eset.begin(); 
		 it != eset.end(); it++) {
		LOGDEB((" [%s]\n", (*it).c_str()));
		if ((*it).empty() || ((*it).at(0)>='A' && (*it).at(0)<='Z'))
		    continue;
		res.push_back(*it);
		if (res.size() >= 10)
		    break;
	    }
            m_reason.erase();
            break;
	} catch (const Xapian::DatabaseModifiedError &e) {
            m_reason = e.get_msg();                    
            m_db->m_ndb->xrdb.reopen();
            continue;
	} XCATCHERROR(m_reason);
	break;
    }

    if (!m_reason.empty()) {
        LOGERR(("Query::expand: xapian error %s\n", m_reason.c_str()));
        res.clear();
    }

    return res;
}

}
