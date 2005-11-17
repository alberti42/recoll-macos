#ifndef lint
static char rcsid [] = "@(#$Id: conftree.cpp,v 1.2 2005-11-17 12:47:03 dockes Exp $  (C) 2003 J.F.Dockes";
#endif
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef TEST_CONFTREE

#include <unistd.h> // for access(2)
#include <ctype.h>

#include <fstream>
#include <sstream>

#include "conftree.h"
#include "pathut.h"

#ifndef NO_NAMESPACES
using namespace std;
using std::list;
#endif // NO_NAMESPACES

#ifndef MIN
#define MIN(A,B) ((A)<(B) ? (A) : (B))
#endif

static void trimstring(string &s, const char *ws = " \t")
{
    string::size_type pos = s.find_first_not_of(ws);
    if (pos == string::npos) {
	s = "";
	return;
    }
    s.replace(0, pos, "");

    pos = s.find_last_not_of(ws);
    if (pos != string::npos && pos != s.length()-1)
	s.replace(pos+1, string::npos, "");
}

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
	// This avoids invisible problems
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
    if (status == STATUS_ERROR)
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
 
const char *ConfSimple::get(const char *nm, const char *sk)
{
    if (status == STATUS_ERROR)
	return 0;
    if (sk == 0)
	sk = "";
    // Find submap
    map<string, map<string, string> >::iterator ss;
    if ((ss = submaps.find(sk)) == submaps.end()) 
	return 0;

    // Find named value
    map<string, string>::iterator s;
    if ((s = ss->second.find(nm)) == ss->second.end()) 
	return 0;
    return (s->second).c_str();
}

static ConfSimple::WalkerCode swalker(void *f, const char *nm, 
				      const char *value)
{
    ostream *output = (ostream *)f;
    if (!nm || !strcmp(nm, ""))
	*output << "\n[" << value << "]\n";
    else
	*output << nm << " = " << value << "\n";
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

    string value1;
    string::size_type pos = 0;
    if (value.length() < 60) {
	value1 = value;
    } else {
	while (pos < value.length()) {
	    string::size_type len = MIN(60, value.length() - pos);
	    value1 += value.substr(pos, len);
	    pos += len;
	    if (pos < value.length())
		value1 += "\\\n";
	}
    }

    map<string, map<string, string> >::iterator ss;
    if ((ss = submaps.find(sk)) == submaps.end()) {
	map<string, string> submap;
	submap[nm] = value1;
	submaps[sk] = submap;

    } else {
	ss->second[nm] = value1;
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
	// ??
	return 0;
    }
}

// Add parameter to file
int ConfSimple::set(const char *nm, const char *value, const char *sk)
{
    string ssk = (sk == 0) ? string("") : string(sk);
    return set(string(nm), string(value), ssk);
}

ConfSimple::WalkerCode 
ConfSimple::sortwalk(WalkerCode (*walker)(void *,const char *,const char *),
		     void *clidata)
{
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
	    if (walker(clidata, it->first.c_str(), it->second.c_str()) 
		== WALK_STOP)
		return WALK_STOP;
	}
    }
    return WALK_CONTINUE;
}

#include <iostream>
void ConfSimple::list()
{
    sortwalk(swalker, &std::cout);
}

static ConfSimple::WalkerCode lwalker(void *l, const char *nm, const char *)
{
    list<string> *lst = (list<string> *)l;
    if (nm && *nm)
	lst->push_back(nm);
    return ConfSimple::WALK_CONTINUE;
}

list<string> ConfSimple::getKeys()
{
    std::list<string> mylist;
    sortwalk(lwalker, &mylist);
    return mylist;
}

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

bool ConfTree::stringToStrings(const string &s, std::list<string> &tokens)
{
    string current;
    tokens.clear();
    enum states {SPACE, TOKEN, INQUOTE, ESCAPE};
    states state = SPACE;
    for (unsigned int i = 0; i < s.length(); i++) {
	switch (s[i]) {
	    case '"': 
	    switch(state) {
	      case SPACE: 
		state=INQUOTE; continue;
	      case TOKEN: 
	        current += '"';
		continue;
	      case INQUOTE: 
		tokens.push_back(current);
		current = "";
		state = SPACE;
		continue;
	      case ESCAPE:
	        current += '"';
	        state = INQUOTE;
	      continue;
	    }
	    break;
	    case '\\': 
	    switch(state) {
	      case SPACE: 
	      case TOKEN: 
		  current += '\\';
		  state=TOKEN; 
		  continue;
	      case INQUOTE: 
		  state = ESCAPE;
		  continue;
	      case ESCAPE:
		  current += '\\';
		  state = INQUOTE;
		  continue;
	    }
	    break;

	    case ' ': 
	    case '\t': 
	    switch(state) {
	      case SPACE: 
		  continue;
	      case TOKEN: 
		tokens.push_back(current);
		current = "";
		state = SPACE;
		continue;
	      case INQUOTE: 
	      case ESCAPE:
		  current += s[i];
		  continue;
	    }
	    break;

	    default:
	    switch(state) {
	      case ESCAPE:
		  state = INQUOTE;
		  break;
	      case SPACE: 
		  state = TOKEN;
		  break;
	      case TOKEN: 
	      case INQUOTE: 
		  break;
	    }
	    current += s[i];
	}
    }
    switch(state) {
    case SPACE: 
	break;
    case TOKEN: 
	tokens.push_back(current);
	break;
    case INQUOTE: 
    case ESCAPE:
	return false;
    }
    return true;
}

bool ConfTree::stringToBool(const string &s)
{
    if (isdigit(s[0])) {
	int val = atoi(s.c_str());
	return val ? true : false;
    }
    if (strchr("yYoOtT", s[0]))
	return true;
    return false;
}


#else // TEST_CONFTREE

#include <stdio.h>
#include <unistd.h>

#include <sstream>
#include <iostream>
#include <list>

#include "conftree.h"

using namespace std;

static char *thisprog;

ConfSimple::WalkerCode mywalker(void *, const char *nm, const char *value)
{
    if (!nm || nm[0] == 0)
	printf("\n[%s]\n", value);
    else 
	printf("'%s' -> '%s'\n", nm, value);
    return ConfSimple::WALK_CONTINUE;
}

const char *longvalue = 
"Donnees012345678901234567890123456789012345678901234567890123456789AA"
"0123456789012345678901234567890123456789012345678901234567890123456789FIN"
    ;

void stringtest() 
{
    string s;
    ConfSimple c(&s);
    cout << "Initial:" << endl;
    c.list();
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
    c.list();
}

static char usage [] =
    "testconftree [opts] filename\n"
    "[-w]  : read/write test.\n"
    "[-s]  : string parsing test. Filename must hold parm 'strings'\n"
    "[-q] nm sect : subsection test: look for nm in 'sect' which can be ''\n"
    "[-S] : string io test. No filename in this case\n"
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
	    case 'w':	op_flags |= OPT_w; break;
	    case 's':   op_flags |= OPT_s; break;
	    case 'S':   op_flags |= OPT_S; break;
	    case 'q':
		op_flags |= OPT_q;
		if (argc < 3)  
		    Usage();
		nm = *(++argv);argc--;
		sub = *(++argv);argc--;		  
		goto b1;

	    default: Usage();	break;
	    }
    b1: argc--; argv++;
    }

    if ((op_flags & OPT_S)) {
	if (argc != 0)
	    Usage();
	stringtest();
    } else {
	if (argc < 1)
	    Usage();

	const char *filename = *argv++;argc--;

	if (op_flags & OPT_w) {
	    ConfSimple parms(filename);
	    if (parms.getStatus() != ConfSimple::STATUS_ERROR) {
		// It's ok for the file to not exist here
	    
		const char *cp = parms.get("mypid");
		if (cp) {
		    printf("Value for mypid is '%s'\n", cp);
		} else {
		    printf("mypid not set\n");
		}
		cp = parms.get("unstring");
		if (cp) {
		    printf("Value for unstring is '%s'\n", cp);
		} else {
		    printf("unstring not set\n");
		}
		string myval;
		if (parms.get(string("unstring"), myval, "")) {
		    printf("std::string value for 'unstring' is '%s'\n", 
			   myval.c_str());
		} else {
		    printf("unstring not set (std::string)\n");
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
	    if (!ConfTree::stringToStrings(source, strings)) {
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
	    printf("LIST\n");parms.list();
	    printf("KEYS\n");
	    list<string> keys = parms.getKeys();
	    for (list<string>::iterator it = keys.begin();it!=keys.end();it++) 
		printf("%s\n", (*it).c_str());
	    //printf("WALK\n");parms.sortwalk(mywalker, 0);
	}
    }
}

#endif
