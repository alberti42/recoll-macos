#ifndef lint
static char rcsid[] = "@(#$Id: qtry.cpp,v 1.4 2005-01-31 14:31:10 dockes Exp $ (C) 2004 J.F.Dockes";
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
#include "mimehandler.h"
#include "pathut.h"

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

    RclConfig *rclconfig = new RclConfig;

    if (!rclconfig->ok())
	cerr << "Config could not be built" << endl;

    string dbdir;
    if (rclconfig->getConfParam(string("dbdir"), dbdir) == 0) {
	cerr << "No database directory in configuration" << endl;
	exit(1);
    }
    dbdir = path_tildexpand(dbdir);
    Rcl::Db *rcldb = new Rcl::Db;

    if (!rcldb->open(dbdir, Rcl::Db::DbRO)) {
	fprintf(stderr, "Could not open database\n");
	exit(1);
    }

    // TOBEDONE: query syntax. Yes it's mighty stupid to cat terms.
    string query;
    while (argc--)
	query += string(*argv++) + " " ;
    rcldb->setQuery(query);
    int i = 0;
    Rcl::Doc doc;
    for (i=0;;i++) {
	doc.erase();
	if (!rcldb->getDoc(i, doc))
	    break;

	cout << "Url: " << doc.url << endl;
	cout << "Mimetype: " << doc.mimetype << endl;
	cout << "Mtime: " << doc.mtime << endl;
	cout << "Origcharset: " << doc.origcharset << endl;
	cout << "Title: " << doc.title << endl;
	cout << "Text: " << doc.text << endl;
	cout << "Keywords: " << doc.keywords << endl;
	cout << "Abstract: " << doc.abstract << endl;
	cout << endl;

	// Go to the file system to retrieve / convert the document text
	// for preview:

	// Look for appropriate handler
	MimeHandlerFunc fun = getMimeHandler(doc.mimetype, 
					     rclconfig->getMimeConf());
	if (!fun) {
	    cout << "No mime handler !" << endl;
	    continue;
	}
	string fn = doc.url.substr(6, string::npos);
	cout << "Filename: "  << fn << endl;

	Rcl::Doc fdoc;
	if (!fun(rclconfig, fn,  doc.mimetype, fdoc)) {
	    cout << "Failed to convert/preview document!" << endl;
	    continue;
	}
	string outencoding = "iso8859-1";
	string printable;
	transcode(fdoc.text, printable, "UTF-8", outencoding);
	cout << printable << endl;
    }
    delete rcldb;
    cerr << "Exiting" << endl;
    exit(0);
}
