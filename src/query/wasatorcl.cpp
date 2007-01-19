#ifndef lint
static char rcsid[] = "@(#$Id: wasatorcl.cpp,v 1.5 2007-01-19 10:22:06 dockes Exp $ (C) 2006 J.F.Dockes";
#endif
#ifndef TEST_WASATORCL

#include "wasastringtoquery.h"
#include "rcldb.h"
#include "searchdata.h"
#include "wasatorcl.h"

Rcl::SearchData *wasaStringToRcl(const string &qs, string &reason)
{
    StringToWasaQuery parser;
    WasaQuery *wq = parser.stringToQuery(qs, reason);
    if (wq == 0) 
	return 0;
    Rcl::SearchData *rq = wasaQueryToRcl(wq);
    if (rq == 0) {
	reason = "Failed translating wasa query structure to recoll";
	return 0;
    }
    return rq;
}

Rcl::SearchData *wasaQueryToRcl(WasaQuery *wasa)
{
    if (wasa == 0)
	return 0;

    Rcl::SearchData *sdata = new Rcl::SearchData(Rcl::SCLT_AND);

    WasaQuery::subqlist_t::iterator it;
    for (it = wasa->m_subs.begin(); it != wasa->m_subs.end(); it++) {
	switch ((*it)->m_op) {
	case WasaQuery::OP_NULL:
	case WasaQuery::OP_AND:
	default:
	    // ??
	    continue;
	case WasaQuery::OP_LEAF:
	    if ((*it)->m_value.find_first_of(" \t\n\r") != string::npos) {
		sdata->addClause
		    (new Rcl::SearchDataClauseDist(Rcl::SCLT_PHRASE, 
						   (*it)->m_value, 0, 
						   (*it)->m_fieldspec));
	    } else {
		sdata->addClause
		    (new Rcl::SearchDataClauseSimple(Rcl::SCLT_AND, 
						     (*it)->m_value, 
						     (*it)->m_fieldspec));
	    }
	    break;
	case WasaQuery::OP_EXCL:
	    // Note: have to add dquotes which will be translated to
	    // phrase if there are several words in there. Not pretty
	    // but should work
	    sdata->addClause
		(new Rcl::SearchDataClauseSimple(Rcl::SCLT_EXCL, 
						 string("\"") + 
						 (*it)->m_value + "\"",
						 (*it)->m_fieldspec));
	    break;
	case WasaQuery::OP_OR:
	    // Concatenate all OR values as phrases. Hope there are no
	    // stray dquotes in there
	    {
		string orvalue;
		WasaQuery::subqlist_t::iterator orit;
		for (orit = (*it)->m_subs.begin(); 
		     orit != (*it)->m_subs.end(); orit++) {
		    orvalue += string("\"") + (*orit)->m_value + "\"";
		}
		sdata->addClause
		    (new Rcl::SearchDataClauseSimple(Rcl::SCLT_OR, 
						     orvalue,
						     (*it)->m_fieldspec));
	    }
	}
    }

    // File type and sort specs. We only know about mime types for now.
    if (wasa->m_typeKind == WasaQuery::WQTK_MIME) {
	for (vector<string>::const_iterator it = wasa->m_types.begin();
	     it != wasa->m_types.end(); it++) {
	    sdata->addFiletype(*it);
	}
    }
    return sdata;
}


#else // TEST

// Takes a query expressed in wasabi/recoll simple language and run it

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <iostream>
using namespace std;

#include "rcldb.h"
#include "rclconfig.h"
#include "pathut.h"
#include "rclinit.h"
#include "debuglog.h"
#include "wasastringtoquery.h"
#include "wasatorcl.h"

static char *thisprog;

static char usage [] =
"rclqlang <query>\n"
" query may be like: \n"
"  implicit AND, Exclusion, field spec:    t1 -t2 title:t3\n"
"  OR has priority: t1 OR t2 t3 OR t4 means (t1 OR t2) AND (t3 OR t4)\n"
"  Phrase: \"t1 t2\" (needs additional quoting on cmd line)\n"
;

static void
Usage(void)
{
    fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

extern bool runQuery(Rcl::Db&, const string& qs);

int main(int argc, char **argv)
{
    thisprog = argv[0];
    argc--; argv++;

    if (argc < 1)
	Usage();
    string qs;
    while (argc--) {
	qs += *argv++;
	if (argc) 
	    qs += " ";
    }

    Rcl::Db rcldb;
    RclConfig *rclconfig;
    string dbdir;
    string reason;
    rclconfig = recollinit(0, 0, reason, 0);
    if (!rclconfig || !rclconfig->ok()) {
	fprintf(stderr, "Recoll init failed: %s\n", reason.c_str());
	exit(1);
    }
    dbdir = rclconfig->getDbDir();

    rcldb.open(dbdir,  Rcl::Db::DbRO, Rcl::Db::QO_STEM);
    if (!runQuery(rcldb, qs))
	exit(1);
    exit(0);
}

bool runQuery(Rcl::Db &rcldb, const string &qs)
{
    string reason;
    RefCntr<Rcl::SearchData> rq(wasaStringToRcl(qs, reason));
    if (!rq.getptr()) {
	cerr << "Query string interpretation failed: " << reason << endl;
	return false;
    }

    rcldb.setQuery(rq, Rcl::Db::QO_STEM);
    int offset = 0;
    int limit = 100;
    cout << "Recoll query: " << rq->getDescription() << endl;
    cout << rcldb.getResCnt() << " results (printing 100 max):" << endl;
    for (int i = offset; i < offset + limit; i++) {
	int pc;
	Rcl::Doc doc;
	if (!rcldb.getDoc(i, doc, &pc))
	    break;

	char cpc[20];
	sprintf(cpc, "%d", pc);
	cout << cpc << "% " 
	     << doc.mimetype.c_str() << " "
	     << doc.dmtime.c_str() << " "
	     << doc.fbytes.c_str() << " bytes "
	     << "[" << doc.url.c_str() << "] " 
	     << "[" << doc.title.c_str() << "] "
	     <<  endl;
    }
    return true;
}

#endif // TEST_WASATORCL
