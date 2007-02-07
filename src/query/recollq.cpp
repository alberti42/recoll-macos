#ifndef lint
static char rcsid[] = "@(#$Id: recollq.cpp,v 1.1 2007-02-07 12:00:17 dockes Exp $ (C) 2006 J.F.Dockes";
#endif
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
" [-o|-a] <query string>\n"
" Runs a recoll query and displays result lines. \n"
"  Default: will interpret the argument(s) as a wasabi query string\n"
"    query may be like: \n"
"    implicit AND, Exclusion, field spec:    t1 -t2 title:t3\n"
"    OR has priority: t1 OR t2 t3 OR t4 means (t1 OR t2) AND (t3 OR t4)\n"
"    Phrase: \"t1 t2\" (needs additional quoting on cmd line)\n"
"  -o Emulate the gui simple search in ANY TERM mode\n"
"  -a Emulate the gui simple search in ALL TERMS mode\n"
"Common options:\n"
"    -c <configdir> : specify config directory, overriding $RECOLL_CONFDIR\n"

;
static void
Usage(void)
{
    fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_o     0x2 
#define OPT_a     0x4 
#define OPT_c     0x8

int main(int argc, char **argv)
{
    string a_config;
    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
        (*argv)++;
        if (!(**argv))
            /* Cas du "adb - core" */
            Usage();
        while (**argv)
            switch (*(*argv)++) {
            case 'a':   op_flags |= OPT_a; break;
            case 'o':   op_flags |= OPT_o; break;
	    case 'c':	op_flags |= OPT_c; if (argc < 2)  Usage();
		a_config = *(++argv);
		argc--; goto b1;
            default: Usage();   break;
            }
    b1: argc--; argv++;
    }

    if (argc < 1) {
	Usage();
    }
    string qs = *argv++;argc--;
    while (argc > 0) {
	qs += string(" ") + *argv++;argc--;
    }

    Rcl::Db rcldb;
    RclConfig *rclconfig;
    string dbdir;
    string reason;

    rclconfig = recollinit(0, 0, reason, &a_config);
    if (!rclconfig || !rclconfig->ok()) {
	fprintf(stderr, "Recoll init failed: %s\n", reason.c_str());
	exit(1);
    }
    dbdir = rclconfig->getDbDir();
    rcldb.open(dbdir,  Rcl::Db::DbRO, Rcl::Db::QO_STEM);

    Rcl::SearchData *sd = 0;

    if ((op_flags & OPT_a) || (op_flags & OPT_o)) {
	sd = new Rcl::SearchData(Rcl::SCLT_OR);
	// If there is no white space inside the query, then the user
	// certainly means it as a phrase.
	bool isreallyaphrase = false;
	if (qs.find_first_of(" \t") == string::npos)
	    isreallyaphrase = true;
	Rcl::SearchDataClause *clp = isreallyaphrase ? 
	    new Rcl::SearchDataClauseDist(Rcl::SCLT_PHRASE, qs, 0) :
	    new Rcl::SearchDataClauseSimple((op_flags & OPT_o)?
					    Rcl::SCLT_OR : Rcl::SCLT_AND, qs);
	if (sd)
	    sd->addClause(clp);
    } else {
	sd = wasaStringToRcl(qs, reason);
    }

    if (!sd) {
	cerr << "Query string interpretation failed: " << reason << endl;
	return 1;
    }

    RefCntr<Rcl::SearchData> rq(sd);
    rcldb.setQuery(rq, Rcl::Db::QO_STEM);
    int offset = 0;
    int limit = 1000;
    cout << "Recoll query: " << rq->getDescription() << endl;
    int cnt = rcldb.getResCnt();
    cout << cnt << " results " << 
	(cnt <= limit ? "" : "(printing 100 max):") << endl;
    for (int i = offset; i < offset + limit; i++) {
	int pc;
	Rcl::Doc doc;
	if (!rcldb.getDoc(i, doc, &pc))
	    break;

	char cpc[20];
	sprintf(cpc, "%d", pc);
	cout << cpc << "%"           << "\t"
	     << doc.mimetype.c_str() << "\t"
	     << doc.dmtime.c_str()   << "\t"
	     << doc.fbytes.c_str()   << "\tbytes" << "\t"
	     << "[" << doc.url.c_str() << "]" << "\t" 
	     << "[" << doc.title.c_str() << "]"
	     <<  endl;
    }

    return 0;
}
