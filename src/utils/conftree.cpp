#ifndef lint
static char rcsid [] = "@(#$Id: conftree.cpp,v 1.10 2007-08-04 07:22:43 dockes Exp $  (C) 2003 J.F.Dockes";
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef TEST_CONFTREE

#include <unistd.h> // for access(2)
#include <ctype.h>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

#include "conftree.h"
#include "pathut.h"
#include "smallut.h"

#ifndef NO_NAMESPACES
using namespace std;
using std::list;
#endif // NO_NAMESPACES

#ifndef MIN
#define MIN(A,B) ((A)<(B) ? (A) : (B))
#endif

#define LL 1024
void ConfSimple::parseinput(istream &input)
{
    string submapkey;
    char cline[LL];
    bool appending = false;
    string line;

    for (;;) {
	input.getline(cline, LL-1);
	// fprintf(stderr, "Line: '%s' status %d\n", cline, int(status));
	if (!input.good()) {
	    if (input.bad()) {
		status = STATUS_ERROR;
		return;
	    }
	    // Must be eof ?
	    break;
	}

	int ll = strlen(cline);
	while (ll > 0 && (cline[ll-1] == '\n' || cline[ll-1] == '\r')) {
	    cline[ll-1] = 0;
	    ll--;
	}

	if (appending)
	    line += cline;
	else
	    line = cline;

	// Note that we trim whitespace before checking for backslash-eol
	// This avoids invisible problems.
	trimstring(line);
	if (line.empty()) {
	    m_order.push_back(ConfLine(ConfLine::CFL_COMMENT, line));
	    continue;
	}
	if (line[line.length() - 1] == '\\') {
	    line.erase(line.length() - 1);
	    appending = true;
	    continue;
	}
	appending = false;

	if (line[0] == '[') {
	    trimstring(line, "[]");
	    if (dotildexpand)
		submapkey = path_tildexpand(line);
	    else 
		submapkey = line;
	    // No need for adding sk to order, will be done with first
	    // variable insert. Also means that empty section are
	    // expandable (won't be output when rewriting)
	    // Another option would be to add the subsec to m_order here
	    // and not do it inside i_set() if init is true
	    continue;
	}

	// Look for first equal sign
	string::size_type eqpos = line.find("=");
	if (eqpos == string::npos) {
	    m_order.push_back(ConfLine(ConfLine::CFL_COMMENT, line));
	    continue;
	}

	// Compute name and value, trim white space
	string nm, val;
	nm = line.substr(0, eqpos);
	trimstring(nm);
	val = line.substr(eqpos+1, string::npos);
	trimstring(val);
	
	if (nm.length() == 0) {
	    m_order.push_back(ConfLine(ConfLine::CFL_COMMENT, line));
	    continue;
	}
	i_set(nm, val, submapkey, true);
    }
}


ConfSimple::ConfSimple(int readonly, bool tildexp)
    : dotildexpand(tildexp), m_data(0)
{
    status = readonly ? STATUS_RO : STATUS_RW;
}

ConfSimple::ConfSimple(string *d, int readonly, bool tildexp)
    : dotildexpand(tildexp), m_data(d)
{
    status = readonly ? STATUS_RO : STATUS_RW;

    stringstream input(*d, ios::in);
    parseinput(input);
}

ConfSimple::ConfSimple(const char *fname, int readonly, bool tildexp)
    : dotildexpand(tildexp), m_filename(fname), m_data(0)
{
    status = readonly ? STATUS_RO : STATUS_RW;

    ifstream input;
    if (readonly) {
	input.open(fname, ios::in);
    } else {
	ios::openmode mode = ios::in|ios::out;
	// It seems that there is no separate 'create if not exists' 
	// open flag. Have to truncate to create, but dont want to do 
	// this to an existing file !
	if (access(fname, 0) < 0) {
	    mode |= ios::trunc;
	}
	input.open(fname, mode);
	if (input.is_open()) {
	    status = STATUS_RW;
	} else {
	    input.clear();
	    input.open(fname, ios::in);
	    if (input.is_open()) {
		status = STATUS_RO;
	    }
	}
    }

    if (!input.is_open()) {
	status = STATUS_ERROR;
	return;
    }	    

    parseinput(input);
}

ConfSimple::StatusCode ConfSimple::getStatus()
{
    switch (status) {
    case STATUS_RO: return STATUS_RO;
    case STATUS_RW: return STATUS_RW;
    default: return STATUS_ERROR;
    }
}

int ConfSimple::get(const string &nm, string &value, const string &sk)
{
    if (!ok())
	return 0;

    // Find submap
    map<string, map<string, string> >::iterator ss;
    if ((ss = m_submaps.find(sk)) == m_submaps.end()) 
	return 0;

    // Find named value
    map<string, string>::iterator s;
    if ((s = ss->second.find(nm)) == ss->second.end()) 
	return 0;
    value = s->second;
    return 1;
}

// Code to appropriately output a subkey (nm=="") or variable line
// Splits long lines
static ConfSimple::WalkerCode varprinter(void *f, const string &nm, 
				      const string &value)
{
    ostream *output = (ostream *)f;
    if (nm.empty()) {
	*output << "\n[" << value << "]\n";
    } else {
	string value1;
	if (value.length() < 60) {
	    value1 = value;
	} else {
	    string::size_type pos = 0;
	    while (pos < value.length()) {
		string::size_type len = MIN(60, value.length() - pos);
		value1 += value.substr(pos, len);
		pos += len;
		if (pos < value.length())
		    value1 += "\\\n";
	    }
	}
	*output << nm << " = " << value1 << "\n";
    }
    return ConfSimple::WALK_CONTINUE;
}

// Set variable and rewrite data
int ConfSimple::set(const std::string &nm, const std::string &value, 
		    const string &sk)
{
    if (status  != STATUS_RW)
	return 0;

    if (!i_set(nm, value, sk))
	return 0;
    return write();
}

// Internal set variable: no rw checking or file rewriting. If init is
// set, we're doing initial parsing, else we are changing a parsed
// tree (changes the way we update the order data)
int ConfSimple::i_set(const std::string &nm, const std::string &value, 
		      const string &sk, bool init)
{
    // Values must not have embedded newlines
    if (value.find_first_of("\n\r") != string::npos) {
	return 0;
    }
    bool existing = false;
    map<string, map<string, string> >::iterator ss;
    if ((ss = m_submaps.find(sk)) == m_submaps.end()) {
	map<string, string> submap;
	submap[nm] = value;
	m_submaps[sk] = submap;
	if (!sk.empty())
	    m_order.push_back(ConfLine(ConfLine::CFL_SK, sk));
	// The var insert will be at the end, need not search for the
	// right place
	init = true;
    } else {
	map<string, string>::iterator it;
	it = ss->second.find(nm);
	if (it == ss->second.end()) {
	    ss->second.insert(pair<string,string>(nm, value));
	} else {
	    it->second = value;
	    existing = true;
	}
    }

    // If the variable already existed, no need to change the order data
    if (existing)
	return 1;

    // Add the new variable at the end of its submap in the order data.

    if (init) {
	// During the initial construction, insert at end
	m_order.push_back(ConfLine(ConfLine::CFL_VAR, nm));
	return 1;
    } 

    list<ConfLine>::iterator start, fin;
    if (sk.empty()) {
	start = m_order.begin();
    } else {
	start = find(m_order.begin(), m_order.end(), 
		     ConfLine(ConfLine::CFL_SK, sk));
	if (start == m_order.end()) {
	    // This is not logically possible. The subkey must
	    // exist. We're doomed
	    std::cerr << "Logical failure during configuration variable " 
		"insertion" << endl;
	    abort();
	}
    }

    fin = m_order.end();
    if (start != m_order.end()) {
	start++;
	for (list<ConfLine>::iterator it = start; it != m_order.end(); it++) {
	    if (it->m_kind == ConfLine::CFL_SK) {
		fin = it;
		break;
	    }
	}
    }
    m_order.insert(fin, ConfLine(ConfLine::CFL_VAR, nm));

    return 1;
}

int ConfSimple::erase(const string &nm, const string &sk)
{
    if (status  != STATUS_RW)
	return 0;

    map<string, map<string, string> >::iterator ss;
    if ((ss = m_submaps.find(sk)) == m_submaps.end()) {
	return 0;
    }
    
    ss->second.erase(nm);
  
    return write();
}

ConfSimple::WalkerCode 
ConfSimple::sortwalk(WalkerCode (*walker)(void *,const string&,const string&),
		     void *clidata)
{
    if (!ok())
	return WALK_STOP;
    // For all submaps:
    for (map<string, map<string, string> >::iterator sit = m_submaps.begin();
	 sit != m_submaps.end(); sit++) {

	// Possibly emit submap name:
	if (!sit->first.empty() && walker(clidata, "", sit->first.c_str())
	    == WALK_STOP)
	    return WALK_STOP;

	// Walk submap
	map<string, string> &sm = sit->second;
	for (map<string, string>::iterator it = sm.begin();it != sm.end();
	     it++) {
	    if (walker(clidata, it->first, it->second) == WALK_STOP)
		return WALK_STOP;
	}
    }
    return WALK_CONTINUE;
}

bool ConfSimple::write()
{
    if (m_filename.length()) {
	ofstream output(m_filename.c_str(), ios::out|ios::trunc);
	if (!output.is_open())
	    return 0;
	return write(output);
    } else if (m_data) {
	ostringstream output(*m_data, ios::out | ios::trunc);
	return write(output);
    } else {
	// No backing store, no writing
	return 1;
    }
}

bool ConfSimple::write(ostream& out)
{
    if (!ok())
	return false;
    string sk;
    for (list<ConfLine>::const_iterator it = m_order.begin(); 
	 it != m_order.end(); it++) {
	switch(it->m_kind) {
	case ConfLine::CFL_COMMENT: 
	    out << it->m_data << endl; 
	    if (!out.good()) 
		return false;
	    break;
	case ConfLine::CFL_SK:      
	    sk = it->m_data;
	    out << "[" << it->m_data << "]" << endl;
	    if (!out.good()) 
		return false;
	    break;
	case ConfLine::CFL_VAR:
	    string value;
	    // As erase() doesnt update m_order we can find unexisting
	    // variables, and must not output anything for them
	    if (get(it->m_data, value, sk)) {
		varprinter(&out, it->m_data, value);
		if (!out.good()) 
		    return false;
	    }
	}
    }
    return true;
}

void ConfSimple::listall()
{
    if (!ok())
	return;
    write(std::cout);
}

list<string> ConfSimple::getNames(const string &sk)
{
    std::list<string> mylist;
    if (!ok())
	return mylist;
    map<string, map<string, string> >::iterator ss;
    if ((ss = m_submaps.find(sk)) == m_submaps.end()) {
	return mylist;
    }
    map<string, string>::const_iterator it;
    for (it = ss->second.begin();it != ss->second.end();it++) {
	mylist.push_back(it->first);
    }
    mylist.sort();
    mylist.unique();
    return mylist;
}

list<string> ConfSimple::getSubKeys()
{
    std::list<string> mylist;
    if (!ok())
	return mylist;
    map<string, map<string, string> >::iterator ss;
    for (ss = m_submaps.begin(); ss != m_submaps.end(); ss++) {
	mylist.push_back(ss->first);
    }
    return mylist;
}

// //////////////////////////////////////////////////////////////////////////
// ConfTree Methods: conftree interpret keys like a hierarchical file tree
// //////////////////////////////////////////////////////////////////////////

int ConfTree::get(const std::string &name, string &value, const string &sk)
{
    if (sk.empty() || sk[0] != '/') {
	//	fprintf(stderr, "Looking in global space");
	return ConfSimple::get(name, value, sk);
    }

    // Get writable copy of subkey path
    string msk = sk;

    // Handle the case where the config file path has an ending / and not
    // the input sk
    path_catslash(msk);

    // Look in subkey and up its parents until root ('')
    for (;;) {
	//fprintf(stderr,"Looking for '%s' in '%s'\n",
	//name.c_str(), msk.c_str());
	if (ConfSimple::get(name, value, msk))
	    return 1;
	string::size_type pos = msk.rfind("/");
	if (pos != string::npos) {
	    msk.replace(pos, string::npos, "");
	} else
	    break;
    }
    return 0;
}

#else // TEST_CONFTREE

#include <stdio.h>
#include <unistd.h>

#include <sstream>
#include <iostream>
#include <list>

#include "conftree.h"
#include "smallut.h"

using namespace std;

static char *thisprog;

ConfSimple::WalkerCode mywalker(void *, const string &nm, const string &value)
{
    if (nm.empty())
	printf("\n[%s]\n", value.c_str());
    else 
	printf("'%s' -> '%s'\n", nm.c_str(), value.c_str());
    return ConfSimple::WALK_CONTINUE;
}

const char *longvalue = 
"Donnees012345678901234567890123456789012345678901234567890123456789AA"
"0123456789012345678901234567890123456789012345678901234567890123456789FIN"
    ;

void memtest(ConfSimple &c) 
{
    cout << "Initial:" << endl;
    c.listall();
    if (c.set("nom", "avec nl \n 2eme ligne", "")) {
	fprintf(stderr, "set with embedded nl succeeded !\n");
	exit(1);
    }
    if (!c.set(string("parm1"), string("1"), string("subkey1"))) {
	fprintf(stderr, "Set error");
	exit(1);
    }
    if (!c.set("sparm", "Parametre \"string\" bla", "s2")) {
	fprintf(stderr, "Set error");
	exit(1);
    }
    if (!c.set("long", longvalue, "")) {
	fprintf(stderr, "Set error");
	exit(1);
    }

    cout << "Final:" << endl;
    c.listall();
}

bool readwrite(ConfNull *conf)
{
    if (conf->ok()) {
	// It's ok for the file to not exist here
	string value;
		
	if (conf->get("mypid", value)) {
	    cout << "Value for mypid is [" << value << "]" << endl;
	} else {
	    cout << "mypid not set" << endl;
	}
		
	if (conf->get("unstring", value)) {
	    cout << "Value for unstring is ["<< value << "]" << endl;
	} else {
	    cout << "unstring not set" << endl;
	}
    }
    char spid[100];
    sprintf(spid, "%d", getpid());
    if (!conf->set("mypid", spid)) {
	cerr << "Set mypid failed" << endl;
    }

    ostringstream ost;
    ost << "mypid" << getpid();
    if (!conf->set(ost.str(), spid, "")) {
	cerr << "Set mypid failed (2)" << endl;
    }
    if (!conf->set("unstring", "Une jolie phrase pour essayer")) {
	cerr << "Set unstring failed" << endl;
    }
    return true;
}

bool query(ConfNull *conf, const string& nm, const string& sub)
{
    if (!conf->ok()) {
	cerr <<  "Error opening or parsing file\n" << endl;
	return false;
    }
    string value;
    if (!conf->get(nm, value, sub)) {
	cerr << "name [" << nm << "] not found in [" << sub << "]" << endl;
	return false;
    }
    cout << "[" << sub << "] " << nm << " " << value << endl;
    return true;
}

bool erase(ConfNull *conf, const string& nm, const string& sub)
{
    if (!conf->ok()) {
	cerr <<  "Error opening or parsing file\n" << endl;
	return false;
    }

    if (!conf->erase(nm, sub)) {
	cerr <<  "delete name [" << nm <<  "] in ["<< sub << "] failed" << endl;
	return false;
    }
    return true;
}

bool setvar(ConfNull *conf, const string& nm, const string& value, 
	    const string& sub)
{
    if (!conf->ok()) {
	cerr <<  "Error opening or parsing file\n" << endl;
	return false;
    }
    if (!conf->set(nm, value, sub)) {
	cerr <<  "Set error\n" << endl;
	return false;
    }
    return true;
}

static char usage [] =
    "testconftree [opts] filename\n"
    "[-w]  : read/write test.\n"
    "[-s]  : string parsing test. Filename must hold parm 'strings'\n"
    "[-a] nm value sect : add/set nm,value in 'sect' which can be ''\n"
    "[-q] nm sect : subsection test: look for nm in 'sect' which can be ''\n"
    "[-d] nm sect : delete nm in 'sect' which can be ''\n"
    "[-S] : string io test. No filename in this case\n"
    "[-V] : volatile config test. No filename in this case\n"
    ;

void Usage() {
    fprintf(stderr, "%s:%s\n", thisprog, usage);
    exit(1);
}
static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_w	  0x2 
#define OPT_q     0x4
#define OPT_s     0x8
#define OPT_S     0x10
#define OPT_d     0x20
#define OPT_V     0x40
#define OPT_a     0x80
#define OPT_k     0x100

int main(int argc, char **argv)
{
    const char *nm = 0;
    const char *sub = 0;
    const char *value = 0;

    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
	(*argv)++;
	if (!(**argv))
	    /* Cas du "adb - core" */
	    Usage();
	while (**argv)
	    switch (*(*argv)++) {
	    case 'd':
		op_flags |= OPT_d;
		if (argc < 3)  
		    Usage();
		nm = *(++argv);argc--;
		sub = *(++argv);argc--;		  
		goto b1;
	    case 'a':
		op_flags |= OPT_a;
		if (argc < 4)  
		    Usage();
		nm = *(++argv);argc--;
		value = *(++argv);argc--;
		sub = *(++argv);argc--;		  
		goto b1;
	    case 'q':
		op_flags |= OPT_q;
		if (argc < 3)  
		    Usage();
		nm = *(++argv);argc--;
		sub = *(++argv);argc--;		  
		goto b1;
	    case 's':   op_flags |= OPT_s; break;
	    case 'k':   op_flags |= OPT_k; break;
	    case 'S':   op_flags |= OPT_S; break;
	    case 'V':   op_flags |= OPT_S; break;
	    case 'w':	op_flags |= OPT_w; break;

	    default: Usage();	break;
	    }
    b1: argc--; argv++;
    }

    if ((op_flags & OPT_S)) {
	// String storage test
	if (argc != 0)
	    Usage();
	string s;
	ConfSimple c(&s);
	memtest(c);
	exit(0);
    } else if  ((op_flags & OPT_V)) {
	// No storage test
	if (argc != 0)
	    Usage();
	ConfSimple c;
	memtest(c);
	exit(0);
    } 

    // Other tests use file(s) as backing store
    if (argc < 1)
	Usage();

    list<string> flist;
    while (argc--) {
	flist.push_back(*argv++);
    }
    bool ro = !(op_flags & (OPT_w|OPT_a|OPT_d));
    ConfNull *conf = 0;
    switch (flist.size()) {
    case 0:
	Usage();
	break;
    case 1:
	conf = new ConfTree(flist.front().c_str(), ro);
	break;
    default:
	conf = new ConfStack<ConfTree>(flist, ro);
	break;
    }

    if (op_flags & OPT_w) {
	exit(!readwrite(conf));
    } else if (op_flags & OPT_q) {
	exit(!query(conf, nm, sub));
    } else if (op_flags & OPT_k) {
	if (!conf->ok()) {
	    cerr << "conf init error" << endl;
	    exit(1);
	}
	list<string>lst = conf->getSubKeys();
	for (list<string>::const_iterator it = lst.begin(); 
	     it != lst.end(); it++) {
	    cout << *it << endl;
	}
	exit(0);
    } else if (op_flags & OPT_a) {
	exit(!setvar(conf, nm, value, sub));
    } else if (op_flags & OPT_d) {
	exit(!erase(conf, nm, sub));
    } else if (op_flags & OPT_s) {
	if (!conf->ok()) {
	    cerr << "Cant open /parse conf file " << endl;
	    exit(1);
	}
	    
	string source;
	if (!conf->get(string("strings"), source, "")) {
	    cerr << "Cant get param 'strings'" << endl;
	    exit(1);
	}
	cout << "source: [" << source << "]" << endl;
	list<string> strings;
	if (!stringToStrings(source, strings)) {
	    cerr << "parse failed" << endl;
	    exit(1);
	}
	    
	for (list<string>::iterator it = strings.begin(); 
	     it != strings.end(); it++) {
	    cout << "[" << *it << "]" << endl;
	}
	     
    } else {
	if (!conf->ok()) {
	    fprintf(stderr, "Open failed\n");
	    exit(1);
	}
	printf("LIST\n");
	conf->listall();
	//printf("WALK\n");conf->sortwalk(mywalker, 0);
	printf("\nNAMES in global space:\n");
	list<string> names = conf->getNames("");
	for (list<string>::iterator it = names.begin();it!=names.end();
	     it++) 
	    printf("%s\n", (*it).c_str());

    }
}

#endif
