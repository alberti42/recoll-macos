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
#include "autoconfig.h"

#include <stdio.h>
#include <cstring>
#include <unistd.h>
#include <fnmatch.h>
#include <regex.h>
#include <math.h>
#include <time.h>

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

#include "xapian.h"

#include "rclconfig.h"
#include "debuglog.h"
#include "rcldb.h"
#include "rcldb_p.h"
#include "stemdb.h"
#include "textsplit.h"
#include "transcode.h"
#include "unacpp.h"
#include "conftree.h"
#include "pathut.h"
#include "smallut.h"
#include "utf8iter.h"
#include "searchdata.h"
#include "rclquery.h"
#include "rclquery_p.h"
#include "md5.h"
#include "rclversion.h"
#include "cancelcheck.h"
#include "ptmutex.h"
#include "termproc.h"
#include "expansiondbs.h"

// Recoll index format version is stored in user metadata. When this change,
// we can't open the db and will have to reindex.
static const string cstr_RCL_IDX_VERSION_KEY("RCL_IDX_VERSION_KEY");
static const string cstr_RCL_IDX_VERSION("1");

static const string cstr_mbreaks("rclmbreaks");

#ifndef NO_NAMESPACES
namespace Rcl {
#endif

// Some prefixes that we could get from the fields file, but are not going
// to ever change.
static const string fileext_prefix = "XE";
static const string mimetype_prefix = "T";
static const string xapday_prefix = "D";
static const string xapmonth_prefix = "M";
static const string xapyear_prefix = "Y";
const string pathelt_prefix = "XP";
const string udi_prefix("Q");
const string parent_prefix("F");

// Special terms to mark begin/end of field (for anchored searches), and
// page breaks
#ifdef RCL_INDEX_STRIPCHARS
const string start_of_field_term = "XXST";
const string end_of_field_term = "XXND";
static const string page_break_term = "XXPG";
#else
string start_of_field_term;
string end_of_field_term;
const string page_break_term = "XXPG/";
#endif

// Field name for the unsplit file name. Has to exist in the field file 
// because of usage in termmatch()
static const string unsplitFilenameFieldName = "rclUnsplitFN";
static const string unsplitfilename_prefix = "XSFS";

string version_string(){
    return string("Recoll ") + string(rclversionstr) + string(" + Xapian ") +
        string(Xapian::version_string());
}

// Synthetic abstract marker (to discriminate from abstract actually
// found in document)
static const string cstr_syntAbs("?!#@");

// Compute the unique term used to link documents to their origin. 
// "Q" + external udi
static inline string make_uniterm(const string& udi)
{
    string uniterm(wrap_prefix(udi_prefix));
    uniterm.append(udi);
    return uniterm;
}

// Compute parent term used to link documents to their parent document (if any)
// "F" + parent external udi
static inline string make_parentterm(const string& udi)
{
    // I prefer to be in possible conflict with omega than with
    // user-defined fields (Xxxx) that we also allow. "F" is currently
    // not used by omega (2008-07)
    string pterm(wrap_prefix(parent_prefix));
    pterm.append(udi);
    return pterm;
}

/* See comment in class declaration: return all subdocuments of a
 * document given by its unique id. 
*/
bool Db::Native::subDocs(const string &udi, vector<Xapian::docid>& docids) 
{
    LOGDEB2(("subDocs: [%s]\n", uniterm.c_str()));
    string pterm = make_parentterm(udi);

    XAPTRY(docids.clear();
           docids.insert(docids.begin(), xrdb.postlist_begin(pterm), 
                         xrdb.postlist_end(pterm)),
           xrdb, m_rcldb->m_reason);

    if (!m_rcldb->m_reason.empty()) {
        LOGERR(("Rcl::Db::subDocs: %s\n", m_rcldb->m_reason.c_str()));
        return false;
    } else {
        LOGDEB0(("Db::Native::subDocs: returning %d ids\n", docids.size()));
        return true;
    }
}

// Turn data record from db into document fields
bool Db::Native::dbDataToRclDoc(Xapian::docid docid, std::string &data, 
				Doc &doc)
{
    LOGDEB2(("Db::dbDataToRclDoc: data:\n%s\n", data.c_str()));
    ConfSimple parms(data);
    if (!parms.ok())
	return false;
    parms.get(Doc::keyurl, doc.url);
    parms.get(Doc::keytp, doc.mimetype);
    parms.get(Doc::keyfmt, doc.fmtime);
    parms.get(Doc::keydmt, doc.dmtime);
    parms.get(Doc::keyoc, doc.origcharset);
    parms.get(cstr_caption, doc.meta[Doc::keytt]);
    parms.get(Doc::keykw, doc.meta[Doc::keykw]);
    parms.get(Doc::keyabs, doc.meta[Doc::keyabs]);
    // Possibly remove synthetic abstract indicator (if it's there, we
    // used to index the beginning of the text as abstract).
    doc.syntabs = false;
    if (doc.meta[Doc::keyabs].find(cstr_syntAbs) == 0) {
	doc.meta[Doc::keyabs] = doc.meta[Doc::keyabs].substr(cstr_syntAbs.length());
	doc.syntabs = true;
    }
    parms.get(Doc::keyipt, doc.ipath);
    parms.get(Doc::keypcs, doc.pcbytes);
    parms.get(Doc::keyfs, doc.fbytes);
    parms.get(Doc::keyds, doc.dbytes);
    parms.get(Doc::keysig, doc.sig);
    doc.xdocid = docid;

    // Other, not predefined meta fields:
    vector<string> keys = parms.getNames(string());
    for (vector<string>::const_iterator it = keys.begin(); 
	 it != keys.end(); it++) {
	if (doc.meta.find(*it) == doc.meta.end())
	    parms.get(*it, doc.meta[*it]);
    }
    doc.meta[Doc::keymt] = doc.dmtime.empty() ? doc.fmtime : doc.dmtime;
    return true;
}

// Return the positions list for the page break term
bool Db::Native::getPagePositions(Xapian::docid docid, vector<int>& vpos)
{
    vpos.clear();
    // Need to retrieve the document record to check for multiple page breaks
    // that we store there for lack of better place
    map<int, int> mbreaksmap;
    try {
	Xapian::Document xdoc = xrdb.get_document(docid);
	string data = xdoc.get_data();
	Doc doc;
	string mbreaks;
	if (dbDataToRclDoc(docid, data, doc) && 
	    doc.getmeta(cstr_mbreaks, &mbreaks)) {
	    vector<string> values;
	    stringToTokens(mbreaks, values, ",");
	    for (unsigned int i = 0; i < values.size() - 1; i += 2) {
		int pos  = atoi(values[i].c_str()) + baseTextPosition;
		int incr = atoi(values[i+1].c_str());
		mbreaksmap[pos] = incr;
	    }
	}
    } catch (...) {
    }

    string qterm = page_break_term;
    Xapian::PositionIterator pos;
    try {
	for (pos = xrdb.positionlist_begin(docid, qterm); 
	     pos != xrdb.positionlist_end(docid, qterm); pos++) {
	    int ipos = *pos;
	    if (ipos < int(baseTextPosition)) {
		LOGDEB(("getPagePositions: got page position %d not in body\n",
			ipos));
		// Not in text body. Strange...
		continue;
	    }
	    map<int, int>::iterator it = mbreaksmap.find(ipos);
	    if (it != mbreaksmap.end()) {
		LOGDEB1(("getPagePositions: found multibreak at %d incr %d\n", 
			ipos, it->second));
		for (int i = 0 ; i < it->second; i++) 
		    vpos.push_back(ipos);
	    }
	    vpos.push_back(ipos);
	} 
    } catch (...) {
	// Term does not occur. No problem.
    }
    return true;
}

int Db::Native::getPageNumberForPosition(const vector<int>& pbreaks, 
					 unsigned int pos)
{
    if (pos < baseTextPosition) // Not in text body
	return -1;
    vector<int>::const_iterator it = 
	upper_bound(pbreaks.begin(), pbreaks.end(), pos);
    return it - pbreaks.begin() + 1;
}


/* Rcl::Db methods ///////////////////////////////// */

bool Db::o_inPlaceReset;

Db::Db(RclConfig *cfp)
    : m_ndb(0), m_config(cfp), m_idxAbsTruncLen(250), m_synthAbsLen(250),
      m_synthAbsWordCtxLen(4), m_flushMb(-1), 
      m_curtxtsz(0), m_flushtxtsz(0), m_occtxtsz(0), m_occFirstCheck(1),
      m_maxFsOccupPc(0), m_mode(Db::DbRO)
{
#ifndef RCL_INDEX_STRIPCHARS
    if (start_of_field_term.empty()) {
	if (o_index_stripchars) {
	    start_of_field_term = "XXST";
	    end_of_field_term = "XXND";
	} else {
	    start_of_field_term = "XXST/";
	    end_of_field_term = "XXND/";
	}
    }
#endif

    m_ndb = new Native(this);
    if (m_config) {
	m_config->getConfParam("maxfsoccuppc", &m_maxFsOccupPc);
	m_config->getConfParam("idxflushmb", &m_flushMb);
    }
#ifdef IDX_THREADS
    if (m_ndb && !m_ndb->m_wqueue.start(DbUpdWorker, this)) {
	LOGERR(("Db::Db: Worker start failed\n"));
	return;
    }
#endif // IDX_THREADS
}

Db::~Db()
{
    LOGDEB2(("Db::~Db\n"));
    if (m_ndb == 0)
	return;
    LOGDEB(("Db::~Db: isopen %d m_iswritable %d\n", m_ndb->m_isopen,
	    m_ndb->m_iswritable));
    i_close(true);
}

vector<string> Db::getStemmerNames()
{
    vector<string> res;
    stringToStrings(Xapian::Stem::get_available_languages(), res);
    return res;
}

bool Db::open(OpenMode mode, OpenError *error)
{
    if (error)
	*error = DbOpenMainDb;

    if (m_ndb == 0 || m_config == 0) {
	m_reason = "Null configuration or Xapian Db";
	return false;
    }
    LOGDEB(("Db::open: m_isopen %d m_iswritable %d\n", m_ndb->m_isopen, 
	    m_ndb->m_iswritable));

    if (m_ndb->m_isopen) {
	// We used to return an error here but I see no reason to
	if (!close())
	    return false;
    }
    if (!m_config->getStopfile().empty())
	m_stops.setFile(m_config->getStopfile());
    string dir = m_config->getDbDir();
    string ermsg;
    try {
	switch (mode) {
	case DbUpd:
	case DbTrunc: 
	    {
		int action = (mode == DbUpd) ? Xapian::DB_CREATE_OR_OPEN :
		    Xapian::DB_CREATE_OR_OVERWRITE;
		m_ndb->xwdb = Xapian::WritableDatabase(dir, action);
                // If db is empty, write the data format version at once
                // to avoid stupid error messages:
                if (m_ndb->xwdb.get_doccount() == 0)
                    m_ndb->xwdb.set_metadata(cstr_RCL_IDX_VERSION_KEY, 
                                             cstr_RCL_IDX_VERSION);
		m_ndb->m_iswritable = true;
		// We open a readonly object in all cases (possibly in
		// addition to the r/w one) because some operations
		// are faster when performed through a Database: no
		// forced flushes on allterms_begin(), ie, used in
		// subDocs()
		m_ndb->xrdb = Xapian::Database(dir);
		LOGDEB(("Db::open: lastdocid: %d\n", 
			m_ndb->xwdb.get_lastdocid()));
                LOGDEB2(("Db::open: resetting updated\n"));
                updated.resize(m_ndb->xwdb.get_lastdocid() + 1);
                for (unsigned int i = 0; i < updated.size(); i++)
                    updated[i] = false;
	    }
	    break;
	case DbRO:
	default:
	    m_ndb->m_iswritable = false;
	    m_ndb->xrdb = Xapian::Database(dir);
	    for (vector<string>::iterator it = m_extraDbs.begin();
		 it != m_extraDbs.end(); it++) {
		if (error)
		    *error = DbOpenExtraDb;
		LOGDEB(("Db::Open: adding query db [%s]\n", it->c_str()));
                // An error here used to be non-fatal (1.13 and older)
                // but I can't see why
                m_ndb->xrdb.add_database(Xapian::Database(*it));
	    }
	    break;
	}
	if (error)
	    *error = DbOpenMainDb;

	// Check index format version. Must not try to check a just created or
	// truncated db
	if (mode != DbTrunc && m_ndb->xdb().get_doccount() > 0) {
	    string version = m_ndb->xdb().get_metadata(cstr_RCL_IDX_VERSION_KEY);
	    if (version.compare(cstr_RCL_IDX_VERSION)) {
		m_ndb->m_noversionwrite = true;
		LOGERR(("Rcl::Db::open: file index [%s], software [%s]\n",
			version.c_str(), cstr_RCL_IDX_VERSION.c_str()));
		throw Xapian::DatabaseError("Recoll index version mismatch",
					    "", "");
	    }
	}
	m_mode = mode;
	m_ndb->m_isopen = true;
	m_basedir = dir;
	if (error)
	    *error = DbOpenNoError;
	return true;
    } XCATCHERROR(ermsg);

    m_reason = ermsg;
    LOGERR(("Db::open: exception while opening [%s]: %s\n", 
	    dir.c_str(), ermsg.c_str()));
    return false;
}

// Note: xapian has no close call, we delete and recreate the db
bool Db::close()
{
    LOGDEB2(("Db::close()\n"));
    return i_close(false);
}
bool Db::i_close(bool final)
{
    if (m_ndb == 0)
	return false;
    LOGDEB(("Db::i_close(%d): m_isopen %d m_iswritable %d\n", final,
	    m_ndb->m_isopen, m_ndb->m_iswritable));
    if (m_ndb->m_isopen == false && !final) 
	return true;

    string ermsg;
    try {
	bool w = m_ndb->m_iswritable;
	if (w) {
	    if (!m_ndb->m_noversionwrite)
		m_ndb->xwdb.set_metadata(cstr_RCL_IDX_VERSION_KEY, cstr_RCL_IDX_VERSION);
	    LOGDEB(("Rcl::Db:close: xapian will close. May take some time\n"));
	}
	// Used to do a flush here. Cant see why it should be necessary.
	deleteZ(m_ndb);
	if (w)
	    LOGDEB(("Rcl::Db:close() xapian close done.\n"));
	if (final) {
	    return true;
	}
	m_ndb = new Native(this);
	if (m_ndb) {
	    return true;
	}
	LOGERR(("Rcl::Db::close(): cant recreate db object\n"));
	return false;
    } XCATCHERROR(ermsg);
    LOGERR(("Db:close: exception while deleting db: %s\n", ermsg.c_str()));
    return false;
}

// Reopen the db with a changed list of additional dbs
bool Db::adjustdbs()
{
    if (m_mode != DbRO) {
        LOGERR(("Db::adjustdbs: mode not RO\n"));
        return false;
    }
    if (m_ndb && m_ndb->m_isopen) {
	if (!close())
	    return false;
	if (!open(m_mode)) {
	    return false;
	}
    }
    return true;
}

int Db::docCnt()
{
    int res = -1;
    if (!m_ndb || !m_ndb->m_isopen)
        return -1;

    XAPTRY(res = m_ndb->xdb().get_doccount(), m_ndb->xrdb, m_reason);

    if (!m_reason.empty()) {
        LOGERR(("Db::docCnt: got error: %s\n", m_reason.c_str()));
        return -1;
    }
    return res;
}

int Db::termDocCnt(const string& _term)
{
    int res = -1;
    if (!m_ndb || !m_ndb->m_isopen)
        return -1;

    string term = _term;
#ifndef RCL_INDEX_STRIPCHARS
    if (o_index_stripchars)
#endif
	if (!unacmaybefold(_term, term, "UTF-8", UNACOP_UNACFOLD)) {
	    LOGINFO(("Db::termDocCnt: unac failed for [%s]\n", _term.c_str()));
	    return 0;
	}

    if (m_stops.isStop(term)) {
	LOGDEB1(("Db::termDocCnt [%s] in stop list\n", term.c_str()));
	return 0;
    }

    XAPTRY(res = m_ndb->xdb().get_termfreq(term), m_ndb->xrdb, m_reason);

    if (!m_reason.empty()) {
        LOGERR(("Db::termDocCnt: got error: %s\n", m_reason.c_str()));
        return -1;
    }
    return res;
}

bool Db::addQueryDb(const string &dir) 
{
    LOGDEB(("Db::addQueryDb: ndb %p iswritable %d db [%s]\n", m_ndb,
	      (m_ndb)?m_ndb->m_iswritable:0, dir.c_str()));
    if (!m_ndb)
	return false;
    if (m_ndb->m_iswritable)
	return false;
    if (find(m_extraDbs.begin(), m_extraDbs.end(), dir) == m_extraDbs.end()) {
	m_extraDbs.push_back(dir);
    }
    return adjustdbs();
}

bool Db::rmQueryDb(const string &dir)
{
    if (!m_ndb)
	return false;
    if (m_ndb->m_iswritable)
	return false;
    if (dir.empty()) {
	m_extraDbs.clear();
    } else {
	vector<string>::iterator it = find(m_extraDbs.begin(), 
					 m_extraDbs.end(), dir);
	if (it != m_extraDbs.end()) {
	    m_extraDbs.erase(it);
	}
    }
    return adjustdbs();
}

// Determining what index a doc result comes from is based on the
// modulo of the docid against the db count. Ref:
// http://trac.xapian.org/wiki/FAQ/MultiDatabaseDocumentID
size_t Db::whatDbIdx(const Doc& doc)
{
    if (doc.xdocid == 0) 
	return (size_t)-1;
    if (m_extraDbs.size() == 0)
	return 0;
    return doc.xdocid % (m_extraDbs.size()+1);
}

bool Db::testDbDir(const string &dir)
{
    string aerr;
    LOGDEB(("Db::testDbDir: [%s]\n", dir.c_str()));
    try {
	Xapian::Database db(dir);
    } XCATCHERROR(aerr);
    if (!aerr.empty()) {
	LOGERR(("Db::Open: error while trying to open database "
		"from [%s]: %s\n", dir.c_str(), aerr.c_str()));
	return false;
    }
    return true;
}

bool Db::isopen()
{
    if (m_ndb == 0)
	return false;
    return m_ndb->m_isopen;
}

// Try to translate field specification into field prefix. 
bool Db::fieldToTraits(const string& fld, const FieldTraits **ftpp)
{
    if (m_config && m_config->getFieldTraits(fld, ftpp))
	return true;

    *ftpp = 0;
    return false;
}


// The splitter breaks text into words and adds postings to the Xapian
// document. We use a single object to split all of the document
// fields and position jumps to separate fields
class TextSplitDb : public TextSplitP {
 public:
    Xapian::Document &doc;   // Xapian document 
    // Base for document section. Gets large increment when we change
    // sections, to avoid cross-section proximity matches.
    Xapian::termpos basepos;
    // Current relative position. This is the remembered value from
    // the splitter callback. The term position is reset for each call
    // to text_to_words(), so that the last value of curpos is the
    // section size (last relative term position), and this is what
    // gets added to basepos in addition to the inter-section increment
    // to compute the first position of the next section.
    Xapian::termpos curpos;

    TextSplitDb(Xapian::Document &d, TermProc *prc)
	: TextSplitP(prc), 
	  doc(d), basepos(1), curpos(0), wdfinc(1)
    {}
    // Reimplement text_to_words to add start and end special terms
    virtual bool text_to_words(const string &in);

    void setprefix(const string& pref) 
    {
	if (pref.empty())
	    prefix.clear();
	else
	    prefix = wrap_prefix(pref);
    }

    void setwdfinc(int i) 
    {
	wdfinc = i;
    }

    friend class TermProcIdx;

private:
    // If prefix is set, we also add a posting for the prefixed terms
    // (ie: for titles, add postings for both "term" and "Sterm")
    string  prefix; 
    // Some fields have more weight
    int wdfinc;
};

// Reimplement text_to_words to insert the begin and end anchor terms.
bool TextSplitDb::text_to_words(const string &in) 
{
    bool ret = false;
    string ermsg;

    try {
	// Index the possibly prefixed start term.
	doc.add_posting(prefix + start_of_field_term, basepos, wdfinc);
	++basepos;
    } XCATCHERROR(ermsg);
    if (!ermsg.empty()) {
	LOGERR(("Db: xapian add_posting error %s\n", ermsg.c_str()));
	goto out;
    }

    if (!TextSplitP::text_to_words(in)) {
	LOGDEB(("TextSplitDb: TextSplit::text_to_words failed\n"));
	goto out;
    }

    try {
	// Index the possibly prefixed end term.
	doc.add_posting(prefix + end_of_field_term, basepos+curpos+1, wdfinc);
	++basepos;
    } XCATCHERROR(ermsg);
    if (!ermsg.empty()) {
	LOGERR(("Db: xapian add_posting error %s\n", ermsg.c_str()));
	goto out;
    }

    ret = true;

out:
    basepos += curpos + 100;
    return true;
}

class TermProcIdx : public TermProc {
public:
    TermProcIdx() : TermProc(0), m_ts(0), m_lastpagepos(0), m_pageincr(0) {}
    void setTSD(TextSplitDb *ts) {m_ts = ts;}

    bool takeword(const std::string &term, int pos, int, int)
    {
	// Compute absolute position (pos is relative to current segment),
	// and remember relative.
	m_ts->curpos = pos;
	pos += m_ts->basepos;
	string ermsg;
	try {
	    // Index without prefix, using the field-specific weighting
	    LOGDEB1(("Emitting term at %d : [%s]\n", pos, term.c_str()));
	    m_ts->doc.add_posting(term, pos, m_ts->wdfinc);
#ifdef TESTING_XAPIAN_SPELL
	    if (Db::isSpellingCandidate(term)) {
		m_ts->db.add_spelling(term);
	    }
#endif
	    // Index the prefixed term.
	    if (!m_ts->prefix.empty()) {
		m_ts->doc.add_posting(m_ts->prefix + term, pos, m_ts->wdfinc);
	    }
	    return true;
	} XCATCHERROR(ermsg);
	LOGERR(("Db: xapian add_posting error %s\n", ermsg.c_str()));
	return false;
    }
    void newpage(int pos)
    {
	pos += m_ts->basepos;
	if (pos < int(baseTextPosition)) {
	    LOGDEB(("newpage: not in body\n", pos));
	    return;
	}

	m_ts->doc.add_posting(m_ts->prefix + page_break_term, pos);
	if (pos == m_lastpagepos) {
	    m_pageincr++;
	    LOGDEB2(("newpage: same pos, pageincr %d lastpagepos %d\n", 
		     m_pageincr, m_lastpagepos));
	} else {
	    LOGDEB2(("newpage: pos change, pageincr %d lastpagepos %d\n", 
		     m_pageincr, m_lastpagepos));
	    if (m_pageincr > 0) {
		// Remember the multiple page break at this position
		unsigned int relpos = m_lastpagepos - baseTextPosition;
		LOGDEB2(("Remembering multiple page break. Relpos %u cnt %d\n",
			relpos, m_pageincr));
		m_pageincrvec.push_back(pair<int, int>(relpos, m_pageincr));
	    }
	    m_pageincr = 0;
	}
	m_lastpagepos = pos;
    }

    virtual bool flush()
    {
	if (m_pageincr > 0) {
	    unsigned int relpos = m_lastpagepos - baseTextPosition;
	    LOGDEB2(("Remembering multiple page break. Position %u cnt %d\n",
		    relpos, m_pageincr));
	    m_pageincrvec.push_back(pair<int, int>(relpos, m_pageincr));
	    m_pageincr = 0;
	}
	return TermProc::flush();
    }

    TextSplitDb *m_ts;
    // Auxiliary page breaks data for positions with multiple page breaks.
    int m_lastpagepos;
    // increment of page breaks at same pos. Normally 0, 1.. when several
    // breaks at the same pos
    int m_pageincr; 
    vector <pair<int, int> > m_pageincrvec;
};


#ifdef TESTING_XAPIAN_SPELL
string Db::getSpellingSuggestion(const string& word)
{
    if (m_ndb == 0)
	return string();

    string term = word;

#ifndef RCL_INDEX_STRIPCHARS
    if (o_index_stripchars)
#endif
    if (!unacmaybefold(word, term, "UTF-8", UNACOP_UNACFOLD)) {
	LOGINFO(("Db::getSpelling: unac failed for [%s]\n", word.c_str()));
	return string();
    }

    if (!isSpellingCandidate(term))
	return string();
    return m_ndb->xrdb.get_spelling_suggestion(term);
}
#endif

// Let our user set the parameters for abstract processing
void Db::setAbstractParams(int idxtrunc, int syntlen, int syntctxlen)
{
    LOGDEB1(("Db::setAbstractParams: trunc %d syntlen %d ctxlen %d\n",
	    idxtrunc, syntlen, syntctxlen));
    if (idxtrunc > 0)
	m_idxAbsTruncLen = idxtrunc;
    if (syntlen > 0)
	m_synthAbsLen = syntlen;
    if (syntctxlen > 0)
	m_synthAbsWordCtxLen = syntctxlen;
}

static const int MB = 1024 * 1024;
static const string cstr_nc("\n\r\x0c");

#define RECORD_APPEND(R, NM, VAL) {R += NM + "=" + VAL + "\n";}

#ifdef IDX_THREADS
void *DbUpdWorker(void* vdbp)
{
    Db *dbp = (Db *)vdbp;
    WorkQueue<DbUpdTask*> *tqp = &(dbp->m_ndb->m_wqueue);
    DbUpdTask *tsk;

    for (;;) {
	if (!tqp->take(&tsk)) {
	    tqp->workerExit();
	    return (void*)1;
	}
	LOGDEB(("DbUpdWorker: got task, ql %d\n", int(tqp->size())));

	const char *fnc = tsk->udi.c_str();
	string ermsg;

	// Add db entry or update existing entry:
	try {
	    Xapian::docid did = 
		dbp->m_ndb->xwdb.replace_document(tsk->uniterm, 
						  tsk->doc);
	    if (did < dbp->updated.size()) {
		dbp->updated[did] = true;
		LOGINFO(("Db::add: docid %d updated [%s]\n", did, fnc));
	    } else {
		LOGINFO(("Db::add: docid %d added [%s]\n", did, fnc));
	    }
	} XCATCHERROR(ermsg);

	if (!ermsg.empty()) {
	    LOGERR(("Db::add: replace_document failed: %s\n", ermsg.c_str()));
	    ermsg.erase();
	    // FIXME: is this ever actually needed?
	    try {
		dbp->m_ndb->xwdb.add_document(tsk->doc);
		LOGDEB(("Db::add: %s added (failed re-seek for duplicate)\n", 
			fnc));
	    } XCATCHERROR(ermsg);
	    if (!ermsg.empty()) {
		LOGERR(("Db::add: add_document failed: %s\n", ermsg.c_str()));
		tqp->workerExit();
		return (void*)0;
	    }
	}
	dbp->maybeflush(tsk->txtlen);

	delete tsk;
    }
}
#endif // IDX_THREADS

// Add document in internal form to the database: index the terms in
// the title abstract and body and add special terms for file name,
// date, mime type etc. , create the document data record (more
// metadata), and update database
bool Db::addOrUpdate(const string &udi, const string &parent_udi,
		     Doc &doc)
{
    LOGDEB(("Db::add: udi [%s] parent [%s]\n", 
	     udi.c_str(), parent_udi.c_str()));
    if (m_ndb == 0)
	return false;
    // Check file system full every mbyte of indexed text.
    if (m_maxFsOccupPc > 0 && 
	(m_occFirstCheck || (m_curtxtsz - m_occtxtsz) / MB >= 1)) {
	LOGDEB(("Db::add: checking file system usage\n"));
	int pc;
	m_occFirstCheck = 0;
	if (fsocc(m_basedir, &pc) && pc >= m_maxFsOccupPc) {
	    LOGERR(("Db::add: stop indexing: file system "
		     "%d%% full > max %d%%\n", pc, m_maxFsOccupPc));
	    return false;
	}
	m_occtxtsz = m_curtxtsz;
    }

    Xapian::Document newdocument;

    // The term processing pipeline:
    TermProcIdx tpidx;
    TermProc *nxt = &tpidx;
    TermProcStop tpstop(nxt, m_stops);nxt = &tpstop;
    //TermProcCommongrams tpcommon(nxt, m_stops); nxt = &tpcommon;

    TermProcPrep tpprep(nxt);
#ifndef RCL_INDEX_STRIPCHARS
    if (o_index_stripchars)
#endif
	nxt = &tpprep;

    TextSplitDb splitter(newdocument, nxt);
    tpidx.setTSD(&splitter);

    // If the ipath is like a path, index the last element. This is
    // for compound documents like zip and chm for which the filter
    // uses the file path as ipath. 
    if (!doc.ipath.empty() && 
	doc.ipath.find_first_not_of("0123456789") != string::npos) {
	string utf8ipathlast;
	// There is no way in hell we could have an idea of the
	// charset here, so let's hope it's ascii or utf-8. We call
	// transcode to strip the bad chars and pray
	if (transcode(path_getsimple(doc.ipath), utf8ipathlast,
		      "UTF-8", "UTF-8")) {
	    splitter.text_to_words(utf8ipathlast);
	}
    }

    // Split and index the path from the url for path-based filtering
    {
	string path = url_gpath(doc.url);
	vector<string> vpath;
	stringToTokens(path, vpath, "/");
	splitter.curpos = 0;
	newdocument.add_posting(wrap_prefix(pathelt_prefix),
				splitter.basepos + splitter.curpos++);
	for (vector<string>::iterator it = vpath.begin(); 
	     it != vpath.end(); it++){
	    if (it->length() > 230) {
		// Just truncate it. May still be useful because of wildcards
		*it = it->substr(0, 230);
	    }
	    newdocument.add_posting(wrap_prefix(pathelt_prefix) + *it, 
				    splitter.basepos + splitter.curpos++);
	}
    }

    // Index textual metadata.  These are all indexed as text with
    // positions, as we may want to do phrase searches with them (this
    // makes no sense for keywords by the way).
    //
    // The order has no importance, and we set a position gap of 100
    // between fields to avoid false proximity matches.
    map<string, string>::iterator meta_it;
    for (meta_it = doc.meta.begin(); meta_it != doc.meta.end(); meta_it++) {
	if (!meta_it->second.empty()) {
	    const FieldTraits *ftp;
	    // We don't test for an empty prefix here. Some fields are part
	    // of the internal conf with an empty prefix (ie: abstract).
	    if (!fieldToTraits(meta_it->first, &ftp)) {
		LOGDEB0(("Db::add: no prefix for field [%s], no indexing\n",
			 meta_it->first.c_str()));
		continue;
	    }
	    LOGDEB0(("Db::add: field [%s] pfx [%s] inc %d: [%s]\n", 
		     meta_it->first.c_str(), ftp->pfx.c_str(), ftp->wdfinc,
		     meta_it->second.c_str()));
	    splitter.setprefix(ftp->pfx);
	    splitter.setwdfinc(ftp->wdfinc);
	    if (!splitter.text_to_words(meta_it->second))
                LOGDEB(("Db::addOrUpdate: split failed for %s\n", 
                        meta_it->first.c_str()));
	}
    }
    splitter.setprefix(string());
    splitter.setwdfinc(1);

    if (splitter.curpos < baseTextPosition)
	splitter.basepos = baseTextPosition;

    // Split and index body text
    LOGDEB2(("Db::add: split body: [%s]\n", doc.text.c_str()));
    if (!splitter.text_to_words(doc.text))
        LOGDEB(("Db::addOrUpdate: split failed for main text\n"));

    ////// Special terms for other metadata. No positions for these.
    // Mime type
    newdocument.add_term(wrap_prefix(mimetype_prefix) + doc.mimetype);

    // Simple file name indexed unsplit for specific "file name"
    // searches. This is not the same as a filename: clause inside the
    // query language.
    // We also add a term for the filename extension if any.
    string utf8fn;
    if (doc.getmeta(Doc::keyfn, &utf8fn) && !utf8fn.empty()) {
	string fn;
	if (unacmaybefold(utf8fn, fn, "UTF-8", UNACOP_UNACFOLD)) {
	    // We should truncate after extracting the extension, but this is
	    // a pathological case anyway
	    if (fn.size() > 230)
		utf8truncate(fn, 230);
	    string::size_type pos = fn.rfind('.');
	    if (pos != string::npos && pos != fn.length() - 1) {
		newdocument.add_term(wrap_prefix(fileext_prefix) + 
				     fn.substr(pos + 1));
	    }
	    newdocument.add_term(wrap_prefix(unsplitfilename_prefix) + fn);
	}
    }

    // Udi unique term: this is used for file existence/uptodate
    // checks, and unique id for the replace_document() call.
    string uniterm = make_uniterm(udi);
    newdocument.add_term(uniterm);
    // Parent term. This is used to find all descendents, mostly to delete them 
    // when the parent goes away
    if (!parent_udi.empty()) {
	newdocument.add_term(make_parentterm(parent_udi));
    }
    // Dates etc.
    time_t mtime = atol(doc.dmtime.empty() ? doc.fmtime.c_str() : 
			doc.dmtime.c_str());
    struct tm *tm = localtime(&mtime);
    char buf[9];
    snprintf(buf, 9, "%04d%02d%02d",
	    tm->tm_year+1900, tm->tm_mon + 1, tm->tm_mday);
    // Date (YYYYMMDD)
    newdocument.add_term(wrap_prefix(xapday_prefix) + string(buf)); 
    // Month (YYYYMM)
    buf[6] = '\0';
    newdocument.add_term(wrap_prefix(xapmonth_prefix) + string(buf));
    // Year (YYYY)
    buf[4] = '\0';
    newdocument.add_term(wrap_prefix(xapyear_prefix) + string(buf)); 


    //////////////////////////////////////////////////////////////////
    // Document data record. omindex has the following nl separated fields:
    // - url
    // - sample
    // - caption (title limited to 100 chars)
    // - mime type 
    //
    // The title, author, abstract and keywords fields are special,
    // they always get stored in the document data
    // record. Configurable other fields can be, too.
    //
    // We truncate stored fields abstract, title and keywords to
    // reasonable lengths and suppress newlines (so that the data
    // record can keep a simple syntax)

    string record;
    RECORD_APPEND(record, Doc::keyurl, doc.url);
    RECORD_APPEND(record, Doc::keytp, doc.mimetype);
    // We left-zero-pad the times so that they are lexico-sortable
    leftzeropad(doc.fmtime, 11);
    RECORD_APPEND(record, Doc::keyfmt, doc.fmtime);
    if (!doc.dmtime.empty()) {
	leftzeropad(doc.dmtime, 11);
	RECORD_APPEND(record, Doc::keydmt, doc.dmtime);
    }
    RECORD_APPEND(record, Doc::keyoc, doc.origcharset);

    if (doc.fbytes.empty())
	doc.fbytes = doc.pcbytes;

    if (!doc.fbytes.empty()) {
	RECORD_APPEND(record, Doc::keyfs, doc.fbytes);
	leftzeropad(doc.fbytes, 12);
	newdocument.add_value(VALUE_SIZE, doc.fbytes);
    }

    if (!doc.pcbytes.empty())
	RECORD_APPEND(record, Doc::keypcs, doc.pcbytes);
    char sizebuf[30]; 
    sprintf(sizebuf, "%u", (unsigned int)doc.text.length());
    RECORD_APPEND(record, Doc::keyds, sizebuf);

    // Note that we add the signature both as a value and in the data record
    if (!doc.sig.empty())
	RECORD_APPEND(record, Doc::keysig, doc.sig);
    newdocument.add_value(VALUE_SIG, doc.sig);

    if (!doc.ipath.empty())
	RECORD_APPEND(record, Doc::keyipt, doc.ipath);

    doc.meta[Doc::keytt] = 
	neutchars(truncate_to_word(doc.meta[Doc::keytt], 150), cstr_nc);
    if (!doc.meta[Doc::keytt].empty())
	RECORD_APPEND(record, cstr_caption, doc.meta[Doc::keytt]);

    trimstring(doc.meta[Doc::keykw], " \t\r\n");
    doc.meta[Doc::keykw] = 
	neutchars(truncate_to_word(doc.meta[Doc::keykw], 300), cstr_nc);
    if (!doc.meta[Doc::keykw].empty())
	RECORD_APPEND(record, Doc::keykw, doc.meta[Doc::keykw]);

    // If abstract is empty, we make up one with the beginning of the
    // document. This is then not indexed, but part of the doc data so
    // that we can return it to a query without having to decode the
    // original file.
    bool syntabs = false;
    // Note that the map accesses by operator[] create empty entries if they
    // don't exist yet.
    trimstring(doc.meta[Doc::keyabs], " \t\r\n");
    if (doc.meta[Doc::keyabs].empty()) {
	syntabs = true;
	if (!doc.text.empty())
	    doc.meta[Doc::keyabs] = cstr_syntAbs + 
		neutchars(truncate_to_word(doc.text, m_idxAbsTruncLen), cstr_nc);
    } else {
	doc.meta[Doc::keyabs] = 
	    neutchars(truncate_to_word(doc.meta[Doc::keyabs], m_idxAbsTruncLen),
		      cstr_nc);
    }
    if (!doc.meta[Doc::keyabs].empty())
	RECORD_APPEND(record, Doc::keyabs, doc.meta[Doc::keyabs]);

    const set<string>& stored = m_config->getStoredFields();
    for (set<string>::const_iterator it = stored.begin();
	 it != stored.end(); it++) {
	string nm = m_config->fieldCanon(*it);
	if (!doc.meta[*it].empty()) {
	    string value = 
		neutchars(truncate_to_word(doc.meta[*it], 150), cstr_nc);
	    RECORD_APPEND(record, nm, value);
	}
    }

    // If empty pages (multiple break at same pos) were recorded, save
    // them (this is because we have no way to record them in the
    // Xapian list
    if (!tpidx.m_pageincrvec.empty()) {
	ostringstream multibreaks;
	for (unsigned int i = 0; i < tpidx.m_pageincrvec.size(); i++) {
	    if (i != 0)
		multibreaks << ",";
	    multibreaks << tpidx.m_pageincrvec[i].first << "," << 
		tpidx.m_pageincrvec[i].second;
	}
	RECORD_APPEND(record, string(cstr_mbreaks), multibreaks.str());
    }
    
    // If the file's md5 was computed, add value. This is optionally
    // used for query result duplicate elimination.
    string& md5 = doc.meta[Doc::keymd5];
    if (!md5.empty()) {
	string digest;
	MD5HexScan(md5, digest);
	newdocument.add_value(VALUE_MD5, digest);
    }

    LOGDEB0(("Rcl::Db::add: new doc record:\n%s\n", record.c_str()));
    newdocument.set_data(record);

#ifdef IDX_THREADS
    DbUpdTask *tp = new DbUpdTask(udi, uniterm, newdocument, doc.text.length());
    if (!m_ndb->m_wqueue.put(tp)) {
	LOGERR(("Db::addOrUpdate:Cant queue task\n"));
	return false;
    }
#else
    const char *fnc = udi.c_str();
    string ermsg;

    // Add db entry or update existing entry:
    try {
	Xapian::docid did = 
	    m_ndb->xwdb.replace_document(uniterm, newdocument);
	if (did < updated.size()) {
	    updated[did] = true;
	    LOGINFO(("Db::add: docid %d updated [%s]\n", did, fnc));
	} else {
	    LOGINFO(("Db::add: docid %d added [%s]\n", did, fnc));
	}
    } XCATCHERROR(ermsg);

    if (!ermsg.empty()) {
	LOGERR(("Db::add: replace_document failed: %s\n", ermsg.c_str()));
	ermsg.erase();
	// FIXME: is this ever actually needed?
	try {
	    m_ndb->xwdb.add_document(newdocument);
	    LOGDEB(("Db::add: %s added (failed re-seek for duplicate)\n", 
		    fnc));
	} XCATCHERROR(ermsg);
	if (!ermsg.empty()) {
	    LOGERR(("Db::add: add_document failed: %s\n", ermsg.c_str()));
	    return false;
	}
    }

    // Test if we're over the flush threshold (limit memory usage):
    maybeflush(doc.text.length());
#endif // IDX_THREADS
    return true;
}

// Flush when idxflushmbs is reached
bool Db::maybeflush(off_t moretext)
{
    if (m_flushMb > 0) {
	m_curtxtsz += moretext;
	if ((m_curtxtsz - m_flushtxtsz) / MB >= m_flushMb) {
	    LOGDEB(("Db::add/delete: txt size >= %d Mb, flushing\n", 
		    m_flushMb));
	    string ermsg;
	    try {
		m_ndb->xwdb.flush();
	    } XCATCHERROR(ermsg);
	    if (!ermsg.empty()) {
		LOGERR(("Db::add: flush() failed: %s\n", ermsg.c_str()));
		return false;
	    }
	    m_flushtxtsz = m_curtxtsz;
	}
    }
    return true;
}

// Test if doc given by udi has changed since last indexed (test sigs)
bool Db::needUpdate(const string &udi, const string& sig)
{
    if (m_ndb == 0)
	return false;

    // If we are doing an in place reset, no need to test. Note that there is
    // no need to update the existence map either, it will be done while 
    // indexing
    if (o_inPlaceReset)
	return true;

    string uniterm = make_uniterm(udi);
    string ermsg;

    // We look up the document indexed by the uniterm. This is either
    // the actual document file, or, for a multi-document file, the
    // pseudo-doc we create to stand for the file itself.

    // We try twice in case database needs to be reopened.
    for (int tries = 0; tries < 2; tries++) {
	try {
	    // Get the doc or pseudo-doc
	    Xapian::PostingIterator docid =m_ndb->xrdb.postlist_begin(uniterm);
	    if (docid == m_ndb->xrdb.postlist_end(uniterm)) {
		// If no document exist with this path, we do need update
		LOGDEB(("Db::needUpdate:yes (new): [%s]\n", uniterm.c_str()));
		return true;
	    }
	    Xapian::Document doc = m_ndb->xrdb.get_document(*docid);

	    // Retrieve old file/doc signature from value
	    string osig = doc.get_value(VALUE_SIG);
	    LOGDEB2(("Db::needUpdate: oldsig [%s] new [%s]\n",
		     osig.c_str(), sig.c_str()));
	    // Compare new/old sig
	    if (sig != osig) {
		LOGDEB(("Db::needUpdate:yes: olsig [%s] new [%s] [%s]\n",
			osig.c_str(), sig.c_str(), uniterm.c_str()));
		// Db is not up to date. Let's index the file
		return true;
	    }

	    LOGDEB(("Db::needUpdate:no: [%s]\n", uniterm.c_str()));

	    // Up to date. 

	    // Set the uptodate flag for doc / pseudo doc
	    if (m_mode 	!= DbRO) {
		updated[*docid] = true;

		// Set the existence flag for all the subdocs (if any)
		vector<Xapian::docid> docids;
		if (!m_ndb->subDocs(udi, docids)) {
		    LOGERR(("Rcl::Db::needUpdate: can't get subdocs\n"));
		    return true;
		}
		for (vector<Xapian::docid>::iterator it = docids.begin();
		     it != docids.end(); it++) {
		    if (*it < updated.size()) {
			LOGDEB2(("Db::needUpdate: set flag for docid %d\n", *it));
			updated[*it] = true;
		    }
		}
	    }
	    return false;
	} catch (const Xapian::DatabaseModifiedError &e) {
	    LOGDEB(("Db::needUpdate: got modified error. reopen/retry\n"));
            m_reason = e.get_msg();
	    m_ndb->xrdb.reopen();
            continue;
	} XCATCHERROR(m_reason);
        break;
    }
    LOGERR(("Db::needUpdate: error while checking existence: %s\n", 
	    m_reason.c_str()));
    return true;
}


// Return existing stem db languages
vector<string> Db::getStemLangs()
{
    LOGDEB(("Db::getStemLang\n"));
    vector<string> langs;
    if (m_ndb == 0 || m_ndb->m_isopen == false)
	return langs;
    StemDb db(m_ndb->xrdb);
    db.getMembers(langs);
    return langs;
}

/**
 * Delete stem db for given language
 */
bool Db::deleteStemDb(const string& lang)
{
    LOGDEB(("Db::deleteStemDb(%s)\n", lang.c_str()));
    if (m_ndb == 0 || m_ndb->m_isopen == false || !m_ndb->m_iswritable)
	return false;
    XapWritableSynFamily db(m_ndb->xwdb, synFamStem);
    return db.deleteMember(lang);
}

/**
 * Create database of stem to parents associations for a given language.
 * We walk the list of all terms, stem them, and create another Xapian db
 * with documents indexed by a single term (the stem), and with the list of
 * parent terms in the document data.
 */
bool Db::createStemDbs(const vector<string>& langs)
{
    LOGDEB(("Db::createStemDbs\n"));
    if (m_ndb == 0 || m_ndb->m_isopen == false || !m_ndb->m_iswritable) {
	LOGERR(("createStemDb: db not open or not writable\n"));
	return false;
    }

    return createExpansionDbs(m_ndb->xwdb, langs);
}

/**
 * This is called at the end of an indexing session, to delete the
 * documents for files that are no longer there. This can ONLY be called
 * after a full file-system tree walk, else the file existence flags will 
 * be wrong.
 */
bool Db::purge()
{
    LOGDEB(("Db::purge\n"));
    if (m_ndb == 0)
	return false;
    LOGDEB(("Db::purge: m_isopen %d m_iswritable %d\n", m_ndb->m_isopen, 
	    m_ndb->m_iswritable));
    if (m_ndb->m_isopen == false || m_ndb->m_iswritable == false) 
	return false;

#ifdef IDX_THREADS
    m_ndb->m_wqueue.waitIdle();
#endif // IDX_THREADS

    // For xapian versions up to 1.0.1, deleting a non-existant
    // document would trigger an exception that would discard any
    // pending update. This could lose both previous added documents
    // or deletions. Adding the flush before the delete pass ensured
    // that any added document would go to the index. Kept here
    // because it doesn't really hurt.
    try {
	m_ndb->xwdb.flush();
    } catch (...) {
	LOGERR(("Db::purge: 1st flush failed\n"));

    }

    // Walk the document array and delete any xapian document whose
    // flag is not set (we did not see its source during indexing).
    int purgecount = 0;
    for (Xapian::docid docid = 1; docid < updated.size(); ++docid) {
	if (!updated[docid]) {
	    if ((purgecount+1) % 100 == 0) {
		try {
		    CancelCheck::instance().checkCancel();
		} catch(CancelExcept) {
		    LOGINFO(("Db::purge: partially cancelled\n"));
		    break;
		}
	    }

	    try {
		if (m_flushMb > 0) {
		    // We use an average term length of 5 for
		    // estimating the doc sizes which is probably not
		    // accurate but gives rough consistency with what
		    // we do for add/update. I should fetch the doc
		    // size from the data record, but this would be
		    // bad for performance.
		    Xapian::termcount trms = m_ndb->xwdb.get_doclength(docid);
		    maybeflush(trms * 5);
		}
		m_ndb->xwdb.delete_document(docid);
		LOGDEB(("Db::purge: deleted document #%d\n", docid));
	    } catch (const Xapian::DocNotFoundError &) {
		LOGDEB0(("Db::purge: document #%d not found\n", docid));
	    } catch (const Xapian::Error &e) {
		LOGERR(("Db::purge: document #%d: %s\n", docid, e.get_msg().c_str()));
	    } catch (...) {
		LOGERR(("Db::purge: document #%d: unknown error\n", docid));
	    }
	    purgecount++;
	}
    }

    try {
	m_ndb->xwdb.flush();
    } catch (...) {
	LOGERR(("Db::purge: 2nd flush failed\n"));
    }
    return true;
}

/* Delete document(s) for given unique identifier (doc and descendents) */
bool Db::purgeFile(const string &udi, bool *existed)
{
    LOGDEB(("Db:purgeFile: [%s]\n", udi.c_str()));
    if (m_ndb == 0 || !m_ndb->m_iswritable)
	return false;

#ifdef IDX_THREADS
    m_ndb->m_wqueue.waitIdle();
#endif // IDX_THREADS

    Xapian::WritableDatabase db = m_ndb->xwdb;
    string uniterm = make_uniterm(udi);
    string ermsg;
    try {
	Xapian::PostingIterator docid = db.postlist_begin(uniterm);
	if (docid == db.postlist_end(uniterm)) {
            if (existed)
                *existed = false;
	    return true;
        }
        *existed = true;
	LOGDEB(("purgeFile: delete docid %d\n", *docid));
	if (m_flushMb > 0) {
	    Xapian::termcount trms = m_ndb->xwdb.get_doclength(*docid);
	    maybeflush(trms * 5);
	}
	db.delete_document(*docid);
	vector<Xapian::docid> docids;
	m_ndb->subDocs(udi, docids);
	LOGDEB(("purgeFile: subdocs cnt %d\n", docids.size()));
	for (vector<Xapian::docid>::iterator it = docids.begin();
	     it != docids.end(); it++) {
	    LOGDEB(("Db::purgeFile: delete subdoc %d\n", *it));
	    if (m_flushMb > 0) {
		Xapian::termcount trms = m_ndb->xwdb.get_doclength(*it);
		maybeflush(trms * 5);
	    }
	    db.delete_document(*it);
	}
	return true;
    } XCATCHERROR(ermsg);
    if (!ermsg.empty()) {
	LOGERR(("Db::purgeFile: %s\n", ermsg.c_str()));
    }
    return false;
}

// File name wild card expansion. This is a specialisation ot termMatch
bool Db::filenameWildExp(const string& fnexp, vector<string>& names, int max)
{
    string pattern = fnexp;
    names.clear();

    // If pattern is not capitalized, not quoted (quoted pattern can't
    // get here currently anyway), and has no wildcards, we add * at
    // each end: match any substring
    if (pattern[0] == '"' && pattern[pattern.size()-1] == '"') {
	pattern = pattern.substr(1, pattern.size() -2);
    } else if (pattern.find_first_of(cstr_minwilds) == string::npos && 
	       !unaciscapital(pattern)) {
	pattern = "*" + pattern + "*";
    } // else let it be

    LOGDEB(("Rcl::Db::filenameWildExp: pattern: [%s]\n", pattern.c_str()));

    TermMatchResult result;
    if (!termMatch(ET_WILD, string(), pattern, result, max,
		   unsplitFilenameFieldName))
	return false;
    for (vector<TermMatchEntry>::const_iterator it = result.entries.begin();
	 it != result.entries.end(); it++) 
	names.push_back(it->term);

    if (names.empty()) {
	// Build an impossible query: we know its impossible because we
	// control the prefixes!
	names.push_back(wrap_prefix("XNONE") + "NoMatchingTerms");
    }
    return true;
}

// Walk the Y terms and return min/max
bool Db::maxYearSpan(int *minyear, int *maxyear)
{
    *minyear = 1000000; 
    *maxyear = -1000000;
    TermMatchResult result;
    if (!termMatch(ET_WILD, string(), "*", result, -1, "xapyear"))
	return false;
    for (vector<TermMatchEntry>::const_iterator it = result.entries.begin();
	 it != result.entries.end(); it++) {
        if (!it->term.empty()) {
            int year = atoi(it->term.c_str()+1);
            if (year < *minyear)
                *minyear = year;
            if (year > *maxyear)
                *maxyear = year;
        }
    }
    return true;
}


class TermMatchCmpByWcf {
public:
    int operator()(const TermMatchEntry& l, const TermMatchEntry& r) {
	return r.wcf - l.wcf < 0;
    }
};
class TermMatchCmpByTerm {
public:
    int operator()(const TermMatchEntry& l, const TermMatchEntry& r) {
	return l.term.compare(r.term) > 0;
    }
};
class TermMatchTermEqual {
public:
    int operator()(const TermMatchEntry& l, const TermMatchEntry& r) {
	return !l.term.compare(r.term);
    }
};

bool Db::stemExpand(const string &langs, const string &term, 
		    TermMatchResult& result)
{
    if (m_ndb == 0 || m_ndb->m_isopen == false)
	return false;
    vector<string> exp;
    StemDb db(m_ndb->xrdb);
    if (!db.stemExpand(langs, term, exp))
	return false;
    result.entries.insert(result.entries.end(), exp.begin(), exp.end());
    return true;
}

/** Add prefix to all strings in list. 
 * @param prefix already wrapped prefix
 */
static void addPrefix(vector<TermMatchEntry>& terms, const string& prefix)
{
    if (prefix.empty())
	return;
    for (vector<TermMatchEntry>::iterator it = terms.begin(); 
         it != terms.end(); it++)
	it->term.insert(0, prefix);
}

// Characters that can begin a wildcard or regexp expression. We use skipto
// to begin the allterms search with terms that begin with the portion of
// the input string prior to these chars.
const string cstr_wildSpecChars = "*?[";
const string cstr_regSpecChars = "(.[{";

// Find all index terms that match a wildcard or regular expression
// If field is set, we return a list of appropriately prefixed terms (which 
// are going to be used to build a Xapian query).
bool Db::termMatch(MatchType typ, const string &lang,
		   const string &root, 
		   TermMatchResult& res,
		   int max, 
		   const string& field)
{
    if (!m_ndb || !m_ndb->m_isopen)
	return false;
    Xapian::Database xdb = m_ndb->xdb();

    XAPTRY(res.dbdoccount = xdb.get_doccount();
           res.dbavgdoclen = xdb.get_avlength(), xdb, m_reason);
    if (!m_reason.empty())
        return false;

    // Get rid of capitals and accents

    string droot = root;

#ifndef RCL_INDEX_STRIPCHARS
    if (o_index_stripchars)
#endif
	if (!unacmaybefold(root, droot, "UTF-8", UNACOP_UNACFOLD)) {
	    LOGERR(("Db::termMatch: unac failed for [%s]\n", root.c_str()));
	    return false;
	}

    string nochars = typ == ET_WILD ? cstr_wildSpecChars : cstr_regSpecChars;

    string prefix;
    if (!field.empty()) {
	const FieldTraits *ftp = 0;
	if (!fieldToTraits(field, &ftp) || ftp->pfx.empty()) {
            LOGDEB(("Db::termMatch: field is not indexed (no prefix): [%s]\n", 
                    field.c_str()));
        } else {
	    prefix = wrap_prefix(ftp->pfx);
	}
    }
    res.prefix = prefix;

    if (typ == ET_STEM) {
	if (!stemExpand(lang, root, res))
	    return false;
	for (vector<TermMatchEntry>::iterator it = res.entries.begin(); 
	     it != res.entries.end(); it++) {
	    XAPTRY(it->wcf = xdb.get_collection_freq(it->term);
                   it->docs = xdb.get_termfreq(it->term),
                   xdb, m_reason);
            if (!m_reason.empty())
                return false;
	    LOGDEB1(("termMatch: %d [%s]\n", it->wcf, it->term.c_str()));
	}
        if (!prefix.empty())
            addPrefix(res.entries, prefix);
    } else {
	regex_t reg;
	int errcode;
	if (typ == ET_REGEXP) {
	    string mroot = droot;
	    if ((errcode = regcomp(&reg, mroot.c_str(), 
				   REG_EXTENDED|REG_NOSUB))) {
		char errbuf[200];
		regerror(errcode, &reg, errbuf, 199);
		LOGERR(("termMatch: regcomp failed: %s\n", errbuf));
		res.entries.push_back(string(errbuf));
		regfree(&reg);
		return false;
	    }
	}

	// Find the initial section before any special char
	string::size_type es = droot.find_first_of(nochars);
	string is;
	switch (es) {
	case string::npos: is = prefix + droot; break;
	case 0: is = prefix; break;
	default: is = prefix + droot.substr(0, es); break;
	}
	LOGDEB1(("termMatch: initsec: [%s]\n", is.c_str()));

        for (int tries = 0; tries < 2; tries++) { 
            try {
                Xapian::TermIterator it = xdb.allterms_begin(); 
                if (!is.empty())
                    it.skip_to(is.c_str());
                for (int rcnt = 0; it != xdb.allterms_end(); it++) {
                    // If we're beyond the terms matching the initial
                    // string, end
                    if (!is.empty() && (*it).find(is) != 0)
                        break;
                    string term;
                    if (!prefix.empty())
                        term = (*it).substr(prefix.length());
                    else
                        term = *it;
                    if (typ == ET_WILD) {
                        if (fnmatch(droot.c_str(), term.c_str(), 0) == 
                            FNM_NOMATCH)
                            continue;
                    } else {
                        if (regexec(&reg, term.c_str(), 0, 0, 0))
                            continue;
                    }
                    // Do we want stem expansion here? We don't do it for now
                    res.entries.push_back(TermMatchEntry(*it, 
                                                   xdb.get_collection_freq(*it),
                                                   it.get_termfreq()));

		    // The problem with truncating here is that this is done
		    // alphabetically and we may not keep the most frequent 
		    // terms. OTOH, not doing it may stall the program if
		    // we are walking the whole term list. We compromise
		    // by cutting at 2*max
                    if (max > 0 && ++rcnt >= 2*max)
			break;
                }
                m_reason.erase();
                break;
            } catch (const Xapian::DatabaseModifiedError &e) {
                m_reason = e.get_msg();
                xdb.reopen();
                continue;
            } XCATCHERROR(m_reason);
            break;
        }
	if (!m_reason.empty()) {
	    LOGERR(("termMatch: %s\n", m_reason.c_str()));
	    return false;
	}

	if (typ == ET_REGEXP) {
	    regfree(&reg);
	}

    }

    TermMatchCmpByTerm tcmp;
    sort(res.entries.begin(), res.entries.end(), tcmp);
    TermMatchTermEqual teq;
    vector<TermMatchEntry>::iterator uit = 
	unique(res.entries.begin(), res.entries.end(), teq);
    res.entries.resize(uit - res.entries.begin());
    TermMatchCmpByWcf wcmp;
    sort(res.entries.begin(), res.entries.end(), wcmp);
    if (max > 0) {
	// Would need a small max and big stem expansion...
	res.entries.resize(MIN(res.entries.size(), (unsigned int)max));
    }
    return true;
}

/** Term list walking. */
class TermIter {
public:
    Xapian::TermIterator it;
    Xapian::Database db;
};
TermIter *Db::termWalkOpen()
{
    if (!m_ndb || !m_ndb->m_isopen)
	return 0;
    TermIter *tit = new TermIter;
    if (tit) {
	tit->db = m_ndb->xdb();
        XAPTRY(tit->it = tit->db.allterms_begin(), tit->db, m_reason);
	if (!m_reason.empty()) {
	    LOGERR(("Db::termWalkOpen: xapian error: %s\n", m_reason.c_str()));
	    return 0;
	}
    }
    return tit;
}
bool Db::termWalkNext(TermIter *tit, string &term)
{
    XAPTRY(
	if (tit && tit->it != tit->db.allterms_end()) {
	    term = *(tit->it)++;
	    return true;
	}
        , tit->db, m_reason);

    if (!m_reason.empty()) {
	LOGERR(("Db::termWalkOpen: xapian error: %s\n", m_reason.c_str()));
    }
    return false;
}
void Db::termWalkClose(TermIter *tit)
{
    try {
	delete tit;
    } catch (...) {}
}

bool Db::termExists(const string& word)
{
    if (!m_ndb || !m_ndb->m_isopen)
	return 0;

    XAPTRY(if (!m_ndb->xdb().term_exists(word)) return false,
           m_ndb->xrdb, m_reason);

    if (!m_reason.empty()) {
	LOGERR(("Db::termWalkOpen: xapian error: %s\n", m_reason.c_str()));
	return false;
    }
    return true;
}


bool Db::stemDiffers(const string& lang, const string& word, 
		     const string& base)
{
    Xapian::Stem stemmer(lang);
    if (!stemmer(word).compare(stemmer(base))) {
	LOGDEB2(("Rcl::Db::stemDiffers: same for %s and %s\n", 
		word.c_str(), base.c_str()));
	return false;
    }
    return true;
}

// Retrieve document defined by Unique doc identifier. This is used
// by the GUI history feature and by open parent/getenclosing
// ! The return value is always true except for fatal errors. Document
//  existence should be tested by looking at doc.pc
bool Db::getDoc(const string &udi, Doc &doc)
{
    LOGDEB(("Db:getDoc: [%s]\n", udi.c_str()));
    if (m_ndb == 0)
	return false;

    // Initialize what we can in any case. If this is history, caller
    // will make partial display in case of error
    doc.meta[Rcl::Doc::keyrr] = "100%";
    doc.pc = 100;

    string uniterm = make_uniterm(udi);
    for (int tries = 0; tries < 2; tries++) {
	try {
            if (!m_ndb->xrdb.term_exists(uniterm)) {
                // Document found in history no longer in the
                // database.  We return true (because their might be
                // other ok docs further) but indicate the error with
                // pc = -1
                doc.pc = -1;
                LOGINFO(("Db:getDoc: no such doc in index: [%s] (len %d)\n",
                         uniterm.c_str(), uniterm.length()));
                return true;
            }
            Xapian::PostingIterator docid = 
                m_ndb->xrdb.postlist_begin(uniterm);
            Xapian::Document xdoc = m_ndb->xrdb.get_document(*docid);
            string data = xdoc.get_data();
            doc.meta[Rcl::Doc::keyudi] = udi;
            return m_ndb->dbDataToRclDoc(*docid, data, doc);
	} catch (const Xapian::DatabaseModifiedError &e) {
            m_reason = e.get_msg();
	    m_ndb->xrdb.reopen();
            continue;
	} XCATCHERROR(m_reason);
        break;
    }

    LOGERR(("Db::getDoc: %s\n", m_reason.c_str()));
    return false;
}

#ifndef NO_NAMESPACES
}
#endif
