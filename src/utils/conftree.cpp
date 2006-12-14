#ifndef lint
static char rcsid [] = "@(#$Id: conftree.cpp,v 1.8 2006-12-14 13:53:43 dockes Exp $  (C) 2003 J.F.Dockes";
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
	if (line.empty())
	    continue;
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
	    continue;
	}

	// Look for first equal sign
	string::size_type eqpos = line.find("=");
	if (eqpos == string::npos)
	    continue;

	// Compute name and value, trim white space
	string nm, val;
	nm = line.substr(0, eqpos);
	trimstring(nm);
	val = line.substr(eqpos+1, string::npos);
	trimstring(val);
	
	if (nm.length() == 0)
	    continue;

	map<string, map<string, string> >::iterator s;
	s = submaps.find(submapkey);
	if (s != submaps.end()) {
	    // submap already exists
	    map<string, string> &sm = s->second;
	    sm[nm] = val;
	} else {
	    map<string, string> newmap;
	    newmap[nm] = val;
	    submaps[submapkey] = newmap;
	}

    }
}


ConfSimple::ConfSimple(int readonly, bool tildexp)
{
    data = 0;
    dotildexpand = tildexp;
    status = readonly ? STATUS_RO : STATUS_RW;
}

ConfSimple::ConfSimple(string *d, int readonly, bool tildexp)
{
    data = d;
    dotildexpand = tildexp;
    status = readonly ? STATUS_RO : STATUS_RW;

    stringstream input(*d, ios::in);
    parseinput(input);
}


ConfSimple::ConfSimple(const char *fname, int readonly, bool tildexp)
{
    data = 0;
    filename = string(fname);
    dotildexpand = tildexp;
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

    // Parse
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
    if ((ss = submaps.find(sk)) == submaps.end()) 
	return 0;

    // Find named value
    map<string, string>::iterator s;
    if ((s = ss->second.find(nm)) == ss->second.end()) 
	return 0;
    value = s->second;
    return 1;
}

static ConfSimple::WalkerCode swalker(void *f, const string &nm, 
				      const string &value)
{
    ostream *output = (ostream *)f;
    if (nm.empty())
	*output << "\n[" << value << "]\n";
    else {
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

int ConfSimple::set(const std::string &nm, const std::string &value, 
		  const string &sk)
{
    if (status  != STATUS_RW)
	return 0;

    // Preprocess value: we don't want nl's in there, and we want to keep
    // lines to a reasonable length
    if (value.find_first_of("\n\r") != string::npos) {
	return 0;
    }

    map<string, map<string, string> >::iterator ss;
    if ((ss = submaps.find(sk)) == submaps.end()) {
	map<string, string> submap;
	submap[nm] = value;
	submaps[sk] = submap;

    } else {
	ss->second[nm] = value;
    }
  
    if (filename.length()) {
	ofstream output(filename.c_str(), ios::out|ios::trunc);
	if (!output.is_open())
	    return 0;
	if (sortwalk(swalker, &output) != WALK_CONTINUE) {
	    return 0;
	}
	return 1;
    } else if (data) {
	ostringstream output(*data, ios::out | ios::trunc);
	if (sortwalk(swalker, &output) != WALK_CONTINUE) {
	    return 0;
	}
	return 1;
    } else {
	// No backing store, no writing
	return 1;
    }
}

int ConfSimple::erase(const string &nm, const string &sk)
{
    if (status  != STATUS_RW)
	return 0;

    map<string, map<string, string> >::iterator ss;
    if ((ss = submaps.find(sk)) == submaps.end()) {
	return 0;

    }
    
    ss->second.erase(nm);
  
    if (filename.length()) {
	ofstream output(filename.c_str(), ios::out|ios::trunc);
	if (!output.is_open())
	    return 0;
	if (sortwalk(swalker, &output) != WALK_CONTINUE) {
	    return 0;
	}
	return 1;
    } else if (data) {
	ostringstream output(*data, ios::out | ios::trunc);
	if (sortwalk(swalker, &output) != WALK_CONTINUE) {
	    return 0;
	}
	return 1;
    } else {
	return 1;
    }
}

int ConfSimple::set(const char *nm, const char *value, const char *sk)
{
    string ssk = (sk == 0) ? string("") : string(sk);
    return set(string(nm), string(value), ssk);
}

ConfSimple::WalkerCode 
ConfSimple::sortwalk(WalkerCode (*walker)(void *,const string&,const string&),
		     void *clidata)
{
    if (!ok())
	return WALK_STOP;
    // For all submaps:
    for (map<string, map<string, string> >::iterator sit = submaps.begin();
	 sit != submaps.end(); sit++) {

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

#include <iostream>
void ConfSimple::listall()
{
    if (!ok())
	return;
    sortwalk(swalker, &std::cout);
}

list<string> ConfSimple::getNames(const string &sk)
{
    std::list<string> mylist;
    if (!ok())
	return mylist;
    map<string, map<string, string> >::iterator ss;
    if ((ss = submaps.find(sk)) == submaps.end()) {
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

static char usage [] =
    "testconftree [opts] filename\n"
    "[-w]  : read/write test.\n"
    "[-s]  : string parsing test. Filename must hold parm 'strings'\n"
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

int main(int argc, char **argv)
{
    const char *nm = 0;
    const char *sub = 0;

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
	    case 'q':
		op_flags |= OPT_q;
		if (argc < 3)  
		    Usage();
		nm = *(++argv);argc--;
		sub = *(++argv);argc--;		  
		goto b1;
	    case 's':   op_flags |= OPT_s; break;
	    case 'S':   op_flags |= OPT_S; break;
	    case 'V':   op_flags |= OPT_S; break;
	    case 'w':	op_flags |= OPT_w; break;

	    default: Usage();	break;
	    }
    b1: argc--; argv++;
    }

    if ((op_flags & OPT_S)) {
	if (argc != 0)
	    Usage();
	string s;
	ConfSimple c(&s);
	memtest(c);
    } else if  ((op_flags & OPT_V)) {
	if (argc != 0)
	    Usage();
	string s;
	ConfSimple c;
	memtest(c);
    } else {
	if (argc < 1)
	    Usage();

	const char *filename = *argv++;argc--;

	if (op_flags & OPT_w) {
	    ConfSimple parms(filename);
	    if (parms.getStatus() != ConfSimple::STATUS_ERROR) {
		// It's ok for the file to not exist here
		string value;
		
		if (parms.get("mypid", value)) {
		    printf("Value for mypid is '%s'\n", value.c_str());
		} else {
		    printf("mypid not set\n");
		}
		
		if (parms.get("unstring", value)) {
		    printf("Value for unstring is '%s'\n", value.c_str());
		} else {
		    printf("unstring not set\n");
		}
	    }
	    char spid[100];
	    sprintf(spid, "%d", getpid());
	    parms.set("mypid", spid);

	    ostringstream ost;;
	    ost << "mypid" << getpid();
	    parms.set(ost.str(), spid, "");

	    parms.set("unstring", "Une jolie phrase pour essayer");
	} else if (op_flags & OPT_q) {
	    ConfTree parms(filename, 0);
	    if (parms.getStatus() == ConfSimple::STATUS_ERROR) {
		fprintf(stderr, "Error opening or parsing file\n");
		exit(1);
	    }
	    string value;
	    if (!parms.get(nm, value, sub)) {
		fprintf(stderr, "name '%s' not found in '%s'\n", nm, sub);
		exit(1);
	    }
	    printf("%s : '%s' = '%s'\n", sub, nm, value.c_str());
	    exit(0);
	} else if (op_flags & OPT_d) {
	    ConfTree parms(filename, 0);
	    if (parms.getStatus() != ConfSimple::STATUS_RW) {
		fprintf(stderr, "Error opening or parsing file\n");
		exit(1);
	    }
	    if (!parms.erase(nm, sub)) {
		fprintf(stderr, "delete name '%s' in '%s' failed\n", nm, sub);
		exit(1);
	    }
	    printf("OK\n");
	    exit(0);
	} else if (op_flags & OPT_s) {
	    ConfSimple parms(filename, 1);
	    if (parms.getStatus() == ConfSimple::STATUS_ERROR) {
		cerr << "Cant open /parse conf file " << filename << endl;
		exit(1);
	    }
	    
	    string source;
	    if (!parms.get(string("strings"), source, "")) {
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
	    ConfTree parms(filename, 1);
	    if (parms.getStatus() == ConfSimple::STATUS_ERROR) {
		fprintf(stderr, "Open failed\n");
		exit(1);
	    }
	    printf("LIST\n");
	    parms.listall();
	    //printf("WALK\n");parms.sortwalk(mywalker, 0);
	    printf("\nNAMES in global space:\n");
	    list<string> names = parms.getNames("");
	    for (list<string>::iterator it = names.begin();it!=names.end();
		 it++) 
		printf("%s\n", (*it).c_str());

	}
    }
}

#endif
