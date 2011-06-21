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
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <strings.h>

#include <iostream>
#include <string>
#include <vector>

#include "pathut.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

#include "utf8iter.h"

#include "xapian.h"

static string thisprog;

static string usage =
    " -d <dbdir> -e <output encoding>\n"
    " -i docid -D : get document data for docid\n"
    " -i docid -X : delete document docid\n"
    " -i docid -b : 'rebuild' document from term positions\n"
    " -t term -E  : term existence test\n"
    " -t term -F  : retrieve term frequency data for given term\n"
    " -t term -P  : retrieve postings for term\n"
    " -i docid -T : term list for doc docid\n"
    " -x          : separate each output char with a space\n"
    " -T          : list all terms\n"
    "    -f       : precede each term in the list with its occurrence count\n"
    "    -n : raw data (no [])\n"
    " -s          : special mode to dump recoll stem db\n"
    " -q term [term ...] : perform AND query\n"
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
#define OPT_i     0x4
#define OPT_T     0x8
#define OPT_D     0x10
#define OPT_t     0x20
#define OPT_P     0x40
#define OPT_F     0x80
#define OPT_E     0x100
#define OPT_b     0x200
#define OPT_s     0x400
#define OPT_f     0x800
#define OPT_q     0x1000
#define OPT_n     0x2000
#define OPT_X     0x4000
#define OPT_x     0x8000

// Compute an exploded version of string, inserting a space between each char.
// (no character combining possible)
static string detailstring(const string& in)
{
    if (!(op_flags & OPT_x))
	return in;
    string out;
    Utf8Iter  it(in);
    for (; !it.eof(); it++) {
	it.appendchartostring(out);
	out += ' ';
    }
    // Strip last space
    if (!out.empty())
	out.resize(out.size()-1);
    return out;
}

Xapian::Database *db;

static void cleanup()
{
    delete db;
}

static void sigcleanup(int sig)
{
    fprintf(stderr, "sigcleanup\n");
    cleanup();
    exit(1);
}

int main(int argc, char **argv)
{
    string dbdir = path_cat(path_home(), ".recoll/xapiandb");
    string outencoding = "ISO8859-1";
    int docid = 1;
    string aterm;

    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
	(*argv)++;
	if (!(**argv))
	    /* Cas du "adb - core" */
	    Usage();
	while (**argv)
	    switch (*(*argv)++) {
	    case 'b':   op_flags |= OPT_b; break;
	    case 'D':	op_flags |= OPT_D; break;
	    case 'd':	op_flags |= OPT_d; if (argc < 2)  Usage();
		dbdir = *(++argv);
		argc--; 
		goto b1;
	    case 'E':	op_flags |= OPT_E; break;
	    case 'e':	op_flags |= OPT_d; if (argc < 2)  Usage();
		outencoding = *(++argv);
		argc--; 
		goto b1;
	    case 'F':   op_flags |= OPT_F; break;
	    case 'f':   op_flags |= OPT_f; break;
	    case 'i':	op_flags |= OPT_i; if (argc < 2)  Usage();
		if (sscanf(*(++argv), "%d", &docid) != 1) Usage();
		argc--; 
		goto b1;
	    case 'n':	op_flags |= OPT_n; break;
	    case 'P':	op_flags |= OPT_P; break;
	    case 'q':	op_flags |= OPT_q; break;
	    case 's':	op_flags |= OPT_s; break;
	    case 'T':	op_flags |= OPT_T; break;
	    case 't':	op_flags |= OPT_t; if (argc < 2)  Usage();
		aterm = *(++argv);
		argc--; 
		goto b1;
	    case 'X':	op_flags |= OPT_X; break;
	    case 'x':	op_flags |= OPT_x; break;
	    default: Usage();	break;
	    }
    b1: argc--; argv++;
    }

    vector<string> qterms;
    if (op_flags & OPT_q) {
	fprintf(stderr, "q argc %d\n", argc);
	if (argc < 1)
	    Usage();
	while (argc > 0) {
	    qterms.push_back(*argv++); argc--;
	}
    }

    if (argc != 0)
	Usage();

    atexit(cleanup);
    if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
	signal(SIGHUP, sigcleanup);
    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	signal(SIGINT, sigcleanup);
    if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
	signal(SIGQUIT, sigcleanup);
    if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
	signal(SIGTERM, sigcleanup);

    try {
	db = new Xapian::Database(dbdir);

	cout << "DB: ndocs " << db->get_doccount() << " lastdocid " <<
	    db->get_lastdocid() << " avglength " << db->get_avlength() << endl;
	    
	if (op_flags & OPT_T) {
	    Xapian::TermIterator term;
	    string printable;
	    string op = (op_flags & OPT_n) ? string(): "[";
	    string cl = (op_flags & OPT_n) ? string(): "]";
	    if (op_flags & OPT_i) {
		for (term = db->termlist_begin(docid); 
		     term != db->termlist_end(docid);term++) {
		    cout << op << detailstring(*term) << cl << endl;
		}
	    } else {
		for (term = db->allterms_begin(); 
		     term != db->allterms_end();term++) {
		    if (op_flags & OPT_f)
			cout << term.get_termfreq() << " ";
		    cout << op << detailstring(*term) << cl << endl;
		}
	    }
	} else if (op_flags & OPT_s) {
	    for (unsigned int docid = 1;
		 docid < db->get_lastdocid(); docid++) {
		// cout << docid << ": ";
		Xapian::TermIterator term;
		for (term = db->termlist_begin(docid); 
		     term != db->termlist_end(docid);term++) {
		    cout << detailstring(*term) << " ";
		    Xapian::Document doc = db->get_document(docid);
		    string data = doc.get_data();
		    cout << data;
		}
	    }
	} else if (op_flags & OPT_D) {
	    Xapian::Document doc = db->get_document(docid);
	    string data = doc.get_data();
	    cout << data << endl;
	} else if (op_flags & OPT_X) {
	    Xapian::Document doc = db->get_document(docid);
	    string data = doc.get_data();
	    cout << data << endl;
	    cout << "Really delete xapian document ?" << endl;
	    string rep;
	    cin >> rep;
	    if (!rep.empty() && (rep[0] == 'y' || rep[0] == 'Y')) {
		Xapian::WritableDatabase wdb(dbdir,  Xapian::DB_OPEN);
		cout << "Deleting" << endl;
		wdb.delete_document(docid);
	    }
	} else if (op_flags & OPT_b) {
	    if (!(op_flags & OPT_i))
		Usage();
	    vector<string> buf;
	    Xapian::TermIterator term;
	    for (term = db->termlist_begin(docid);
		 term != db->termlist_end(docid); term++) {
		Xapian::PositionIterator pos;
		for (pos = db->positionlist_begin(docid, *term); 
		     pos != db->positionlist_end(docid, *term); pos++) {
		    if (buf.size() <= *pos)
			buf.resize((*pos)+100);
		    buf[(*pos)] = detailstring(*term);
		}
	    }
	    for (vector<string>::iterator it = buf.begin(); it != buf.end();
		 it++) {
		cout << *it << " ";
	    }
	} else if (op_flags & OPT_P) {
	    Xapian::PostingIterator doc;
	    for (doc = db->postlist_begin(aterm);
		 doc != db->postlist_end(aterm); doc++) {
		cout << *doc << " : " ;
		Xapian::PositionIterator pos;
		for (pos = doc.positionlist_begin(); 
		     pos != doc.positionlist_end(); pos++) {
		    cout << *pos << " " ;
		}
		cout << endl;
	    }
		
	} else if (op_flags & OPT_F) {
	    cout << "FreqFor " << aterm << " : " <<
		db->get_termfreq(aterm) << endl;
	} else if (op_flags & OPT_E) {
	    cout << "Exists [" << aterm << "] : " <<
		db->term_exists(aterm) << endl;
	}  else if (op_flags & OPT_q) {
	    Xapian::Enquire enquire(*db);

	    Xapian::Query query(Xapian::Query::OP_AND, qterms.begin(), 
				qterms.end());
	    cout << "Performing query `" <<
		query.get_description() << "'" << endl;
	    enquire.set_query(query);

	    Xapian::MSet matches = enquire.get_mset(0, 10);
	    cout << "Estimated results: " << 
		matches.get_matches_lower_bound() << endl;
	    Xapian::MSetIterator i;
	    for (i = matches.begin(); i != matches.end(); ++i) {
		cout << "Document ID " << *i << "\t";
		cout << i.get_percent() << "% ";
		Xapian::Document doc = i.get_document();
		cout << "[" << doc.get_data() << "]" << endl;
	    }
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
