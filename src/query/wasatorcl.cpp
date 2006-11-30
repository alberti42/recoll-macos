#ifndef lint
static char rcsid[] = "@(#$Id: wasatorcl.cpp,v 1.1 2006-11-30 18:12:16 dockes Exp $ (C) 2006 J.F.Dockes";
#endif
#ifndef TEST_WASATORCL

#include "wasastringtoquery.h"
#include "rcldb.h"
#include "searchdata.h"
#include "wasatorcl.h"

Rcl::SearchData *wasatorcl(WasaQuery *wasa)
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
						     (*it)->m_value, 0));
	    } else {
		sdata->addClause
		    (new Rcl::SearchDataClauseSimple(Rcl::SCLT_AND, 
						     (*it)->m_value));
	    }
	    break;
	case WasaQuery::OP_EXCL:
	    // Note: have to add dquotes which will be translated to
	    // phrase if there are several words in there. Not pretty
	    // but should work
	    sdata->addClause
		(new Rcl::SearchDataClauseSimple(Rcl::SCLT_EXCL, 
						 string("\"") + 
						 (*it)->m_value + "\""));
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
						     orvalue));
	    }
	}
    }

    return sdata;
}


#else // TEST

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>

#include <iostream>
#include <list>
#include <string>

using namespace std;

#include "debuglog.h"
#include "rclinit.h"
#include "rclconfig.h"
#include "rcldb.h"
#include "searchdata.h"
#include "refcntr.h"
#include "wasastringtoquery.h"
#include "wasatorcl.h"

static char *thisprog;

int main(int argc, char *argv[])
{
    thisprog = argv[0];
    argc--; argv++;

    if (argc != 1) {
	fprintf(stderr, "need one arg\n");
	exit(1);
    }
    const string str = *argv++;argc--;
    string reason;

    RclConfig *config = recollinit(RCLINIT_NONE, 0, 0, reason, 0);
    if (config == 0 || !config->ok()) {
        cerr << "Configuration problem: " << reason << endl;
        exit(1);
    }
    string dbdir = config->getDbDir();
    if (dbdir.empty()) {
	// Note: this will have to be replaced by a call to a
	// configuration buildin dialog for initial configuration
        cerr << "Configuration problem: " << "No dbdir" << endl;
	exit(1);
    }
    Rcl::Db rcldb;
    if (!rcldb.open(dbdir, Rcl::Db::DbRO, 0)) {
        cerr << "Could not open database in " << dbdir << endl;
	return 1;
    }
    
    StringToWasaQuery qparser;
    WasaQuery *wq = qparser.stringToQuery(str, reason);
    if (wq == 0) {
	fprintf(stderr, "wasastringtoquery failed: %s\n", reason.c_str());
	return 1;
    }
    string desc;
    wq->describe(desc);
    cout << endl << "Wasabi query description: " << desc << endl << endl;

    Rcl::SearchData *sdata = wasatorcl(wq);
    RefCntr<Rcl::SearchData> rq(sdata);
    if (!rcldb.setQuery(rq)) {
	cerr << "setQuery failed" << endl;
	return 1;
    }
    int maxi = rcldb.getResCnt() > 10 ? 10 : rcldb.getResCnt();

    cout << endl << "Rcl Query description: " << sdata->getDescription() 
	 << endl << endl << "Results: " << endl;

    for (int i = 0; i < maxi ; i++) {
	Rcl::Doc doc;
	if (!rcldb.getDoc(i, doc)) {
	    cerr << "getDoc failed" << endl;
	    return 1;
	}
	cout << i << ": " << doc.url << endl;
    }
    return 0;
}
#endif // TEST_WASATORCL
