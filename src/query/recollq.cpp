#ifndef lint
static char rcsid[] = "@(#$Id: recollq.cpp,v 1.10 2007-11-08 09:35:47 dockes Exp $ (C) 2006 J.F.Dockes";
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
// Takes a query and run it, no gui, results to stdout

#ifndef TEST_RECOLLQ
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include <iostream>
using namespace std;

#include "rcldb.h"
#include "rclconfig.h"
#include "pathut.h"
#include "rclinit.h"
#include "debuglog.h"
#include "wasastringtoquery.h"
#include "wasatorcl.h"
#include "internfile.h"
#include "wipedir.h"

static char *thisprog;
static char usage [] =
" [-o|-a|-f] <query string>\n"
" Runs a recoll query and displays result lines. \n"
"  Default: will interpret the argument(s) as a wasabi query string\n"
"    query may be like: \n"
"    implicit AND, Exclusion, field spec:    t1 -t2 title:t3\n"
"    OR has priority: t1 OR t2 t3 OR t4 means (t1 OR t2) AND (t3 OR t4)\n"
"    Phrase: \"t1 t2\" (needs additional quoting on cmd line)\n"
"  -o Emulate the gui simple search in ANY TERM mode\n"
"  -a Emulate the gui simple search in ALL TERMS mode\n"
"  -f Emulate the gui simple search in filename mode\n"
"Common options:\n"
"    -c <configdir> : specify config directory, overriding $RECOLL_CONFDIR\n"
"    -d also dump file contents\n"
"    -n <cnt> limit the maximum number of results (0->no limit, default 2000)\n"
"    -b : basic. Just output urls, no mime types or titles\n"
;
static void
Usage(void)
{
    cerr << thisprog <<  ": usage:" << endl << usage;
    exit(1);
}

// ATTENTION A LA COMPATIBILITE AVEC LES OPTIONS DE recoll
// OPT_q and OPT_t are ignored
static int     op_flags;
#define OPT_o     0x2 
#define OPT_a     0x4 
#define OPT_c     0x8
#define OPT_d     0x10
#define OPT_n     0x20
#define OPT_b     0x40
#define OPT_f     0x80
#define OPT_l     0x100
#define OPT_q     0x200
#define OPT_t     0x400

int recollq(RclConfig **cfp, int argc, char **argv)
{
    string a_config;
    int limit = 2000;
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
            case 'b':   op_flags |= OPT_b; break;
	    case 'c':	op_flags |= OPT_c; if (argc < 2)  Usage();
		a_config = *(++argv);
		argc--; goto b1;
            case 'd':   op_flags |= OPT_d; break;
            case 'f':   op_flags |= OPT_f; break;
            case 'l':   op_flags |= OPT_l; break;
	    case 'n':	op_flags |= OPT_n; if (argc < 2)  Usage();
		limit = atoi(*(++argv));
		if (limit <= 0) limit = INT_MAX;
		argc--; goto b1;
            case 'o':   op_flags |= OPT_o; break;
            case 'q':   op_flags |= OPT_q; break;
            case 't':   op_flags |= OPT_t; break;
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
    string dbdir;
    string reason;
    *cfp = recollinit(0, 0, reason, &a_config);
    RclConfig *rclconfig = *cfp;
    if (!rclconfig || !rclconfig->ok()) {
	fprintf(stderr, "Recoll init failed: %s\n", reason.c_str());
	exit(1);
    }
    dbdir = rclconfig->getDbDir();
    rcldb.open(dbdir,  rclconfig->getStopfile(),
	       Rcl::Db::DbRO, Rcl::Db::QO_STEM);

    Rcl::SearchData *sd = 0;

    if (op_flags & (OPT_a|OPT_o|OPT_f)) {
	sd = new Rcl::SearchData(Rcl::SCLT_OR);
	Rcl::SearchDataClause *clp = 0;
	if (op_flags & OPT_f) {
	    clp = new Rcl::SearchDataClauseFilename(qs);
	} else {
	    // If there is no white space inside the query, then the user
	    // certainly means it as a phrase.
	    bool isreallyaphrase = false;
	    if (qs.find_first_of(" \t") == string::npos)
		isreallyaphrase = true;
	    clp = isreallyaphrase ? 
		new Rcl::SearchDataClauseDist(Rcl::SCLT_PHRASE, qs, 0) :
		new Rcl::SearchDataClauseSimple((op_flags & OPT_o)?
						Rcl::SCLT_OR : Rcl::SCLT_AND, 
						qs);
	}
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
    int cnt = rcldb.getResCnt();
    if (!(op_flags & OPT_b)) {
	cout << "Recoll query: " << rq->getDescription() << endl;
	if (cnt <= limit)
	    cout << cnt << " results:" << endl;
	else
	    cout << cnt << " results (printing  " << limit << " max):" << endl;
    }

    string tmpdir;
    for (int i = 0; i < limit; i++) {
	int pc;
	Rcl::Doc doc;
	if (!rcldb.getDoc(i, doc, &pc))
	    break;

	if (op_flags & OPT_b) {
	    cout << doc.url.c_str() << endl;
	} else {
	    char cpc[20];
	    sprintf(cpc, "%d", pc);
	    cout 
		<< doc.mimetype.c_str() << "\t"
		<< "[" << doc.url.c_str() << "]" << "\t" 
		<< "[" << doc.meta["title"].c_str() << "]" << "\t"
		<< doc.fbytes.c_str()   << "\tbytes" << "\t"
		<<  endl;
	}
	if (op_flags & OPT_d) {
	    string fn = doc.url.substr(7);
	    struct stat st;
	    if (stat(fn.c_str(), &st) != 0) {
		cout << "No such file: " << fn << endl;
		continue;
	    } 
	    if (tmpdir.empty() || access(tmpdir.c_str(), 0) < 0) {
		string reason;
		if (!maketmpdir(tmpdir, reason)) {
		    cerr << "Cannot create temporary directory: "
			 << reason << endl;
		    return 1;
		}
	    }
	    wipedir(tmpdir);
	    FileInterner interner(fn, &st, rclconfig, tmpdir, &doc.mimetype);
	    if (interner.internfile(doc, doc.ipath)) {
		cout << doc.text << endl;
	    } else {
		cout << "Cant intern: " << fn << endl;
	    }
	}
	
    }

    // Maybe clean up temporary directory
    if (tmpdir.length()) {
	wipedir(tmpdir);
	if (rmdir(tmpdir.c_str()) < 0) {
	    cerr << "Cannot clear temp dir " << tmpdir << endl;
	}
    }

    return 0;
}

#else // TEST_RECOLLQ The test driver is actually the useful program...
#include <stdlib.h>

#include "rclconfig.h"
#include "recollq.h"

static RclConfig *rclconfig;

RclConfig *RclConfig::getMainConfig() 
{
    return rclconfig;
}

int main(int argc, char **argv)
{
    exit(recollq(&rclconfig, argc, argv));
}
#endif // TEST_RECOLLQ

