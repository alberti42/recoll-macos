#ifndef lint
static char rcsid[] = "@(#$Id: qxtry.cpp,v 1.3 2006-01-23 13:32:28 dockes Exp $ (C) 2004 J.F.Dockes";
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
// Tests with direct xapian questions

#include <strings.h>

#include <iostream>
#include <string>
#include <vector>

#include "transcode.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

#include "xapian.h"

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
#define OPT_d	  0x1 
#define OPT_e     0x2

Xapian::Database db;

int main(int argc, char **argv)
{
    string dbdir = "/home/dockes/tmp/xapiandb";
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
	    case 'd':	op_flags |= OPT_e; if (argc < 2)  Usage();
		dbdir = *(++argv);
		argc--; 
		goto b1;
	    case 'e':	op_flags |= OPT_d; if (argc < 2)  Usage();
		encoding = *(++argv);
		argc--; 
		goto b1;
	    default: Usage();	break;
	    }
    b1: argc--; argv++;
    }
    if (argc < 1)
	Usage();

    vector<string> qterms;
    while (argc--) {
	qterms.push_back(*argv++);
    }

    try {
	db = Xapian::Auto::open(dbdir, Xapian::DB_OPEN);

	cout << "DB: ndocs " << db.get_doccount() << " lastdocid " <<
	    db.get_lastdocid() << " avglength " << db.get_avlength() << endl;
	    
	Xapian::Enquire enquire(db);

	Xapian::Query query(Xapian::Query::OP_OR, qterms.begin(), 
			    qterms.end());
	cout << "Performing query `" <<
	    query.get_description() << "'" << endl;
	enquire.set_query(query);

	Xapian::MSet matches = enquire.get_mset(0, 10);
	cout << "Estimated results: " << matches.get_matches_lower_bound() <<
	    endl;
	Xapian::MSetIterator i;
	for (i = matches.begin(); i != matches.end(); ++i) {
	    cout << "Document ID " << *i << "\t";
	    cout << i.get_percent() << "% ";
	    Xapian::Document doc = i.get_document();
	    cout << "[" << doc.get_data() << "]" << endl;
	}

    } catch (const Xapian::Error &e) {
	cout << "Exception: " << e.get_msg() << endl;
    } catch (const string &s) {
	cout << "Exception: " << s << endl;
    } catch (const char *s) {
	cout << "Exception: " << s << endl;
    } catch (...) {
	cout << "Caught unknown exception" << endl;
    }

    exit(0);
}
