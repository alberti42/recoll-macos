#ifndef lint
static char rcsid[] = "@(#$Id: qtry.cpp,v 1.1 2005-01-24 13:17:58 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

// Tests with the query interface

#include <strings.h>

#include <iostream>
#include <string>
#include <vector>

#include "conftree.h"
#include "rclconfig.h"
#include "rcldb.h"
#include "transcode.h"

using namespace std;

#include "rcldb.h"

static string thisprog;

static string usage =
    " -d <dbdir> -e <interface encoding> term [term] ..."
    "  \n\n"
    ;

static void
Usage(void)
{
    cerr << thisprog  << ": usage:\n" << usage;
    exit(1);
}

static int        op_flags;
#define OPT_e     0x2

int main(int argc, char **argv)
{
    string encoding = "ISO8859-1";

    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
	(*argv)++;
	if (!(**argv))
	    /* Cas du "adb - core" */
	    Usage();
	while (**argv)
	    switch (*(*argv)++) {
	    case 'e':	op_flags |= OPT_e; if (argc < 2)  Usage();
		encoding = *(++argv);
		argc--; 
		goto b1;
	    default: Usage();	break;
	    }
    b1: argc--; argv++;
    }
    if (argc < 1)
	Usage();

    RclConfig *config = new RclConfig;

    if (!config->ok())
	cerr << "Config could not be built" << endl;

    string dbdir;
    if (config->getConfParam(string("dbdir"), dbdir) == 0) {
	cerr << "No database directory in configuration" << endl;
	exit(1);
    }
    
    Rcl::Db *db = new Rcl::Db;

    if (!db->open(dbdir, Rcl::Db::DbRO)) {
	fprintf(stderr, "Could not open database\n");
	exit(1);
    }

    // TOBEDONE: query syntax. Yes it's mighty stupid to cat terms.
    string query;
    while (argc--)
	query += string(*argv++) + " " ;
    db->setQuery(query);
    int i = 0;
    Rcl::Doc doc;
    while (db->getDoc(i++, doc)) {
	cout << "Url: " << doc.url << endl;
	cout << "Mimetype: " << doc.mimetype << endl;
	cout << "Mtime: " << doc.mtime << endl;
	cout << "Origcharset: " << doc.origcharset << endl;
	cout << "Title: " << doc.title << endl;
	cout << "Text: " << doc.text << endl;
	cout << "Keywords: " << doc.keywords << endl;
	cout << "Abstract: " << doc.abstract << endl;
	cout << endl;
	
	doc.erase();
    }
    delete db;
    cerr << "Exiting" << endl;
    exit(0);
}
