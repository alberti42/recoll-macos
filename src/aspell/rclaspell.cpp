#ifndef TEST_RCLASPELL
#ifndef lint
static char rcsid[] = "@(#$Id: rclaspell.cpp,v 1.2 2006-10-09 16:37:08 dockes Exp $ (C) 2006 J.F.Dockes";
#endif
#include <unistd.h>
#include <dlfcn.h>
#include <iostream>

#include "aspell.h"

#include "pathut.h"
#include "execmd.h"
#include "rclaspell.h"

class AspellData {
public:
    AspellData() : m_handle(0) {}
    ~AspellData() {
	if (m_handle) 
	    dlclose(m_handle);
    }

    void  *m_handle;
    string m_exec;
};

Aspell::~Aspell()
{
    if (m_data)
	delete m_data;
}

// Just a place where we keep the Aspell library entry points together
class AspellApi {
public:
    struct AspellConfig *(*new_aspell_config)();
    int (*aspell_config_replace)(struct AspellConfig *, const char * key, 
				 const char * value);
    struct AspellCanHaveError *(*new_aspell_speller)(struct AspellConfig *);
    void (*delete_aspell_config)(struct AspellConfig *);
    void (*delete_aspell_can_have_error)(struct AspellCanHaveError *);
    struct AspellSpeller * (*to_aspell_speller)(struct AspellCanHaveError *);
    struct AspellConfig * (*aspell_speller_config)(struct AspellSpeller *);
    const struct AspellWordList * (*aspell_speller_suggest)
	(struct AspellSpeller *, const char *, int);
    struct AspellStringEnumeration * (*aspell_word_list_elements)
	(const struct AspellWordList * ths);
    const char * (*aspell_string_enumeration_next)
	(struct AspellStringEnumeration * ths);
    void (*delete_aspell_string_enumeration)(struct AspellStringEnumeration *);
    const struct AspellError *(*aspell_error)
	(const struct AspellCanHaveError *);
    const char *(*aspell_error_message)(const struct AspellCanHaveError *);
    const char *(*aspell_speller_error_message)(const struct AspellSpeller *);
    void (*delete_aspell_speller)(struct AspellSpeller *);

};
static AspellApi aapi;

#define NMTOPTR(NM, TP)							\
    if ((aapi.NM = TP dlsym(m_data->m_handle, #NM)) == 0) {		\
	badnames += #NM + string(" ");					\
    }

bool Aspell::init(const string &basedir, string &reason)
{
    if (m_data == 0)
	m_data = new AspellData;
    if (m_data->m_handle) {
	dlclose(m_data->m_handle);
	m_data->m_handle = 0;
    }

    m_data->m_exec = path_cat(basedir, "bin");
    m_data->m_exec = path_cat(m_data->m_exec, "aspell");
    if (access(m_data->m_exec.c_str(), X_OK) != 0) {
	reason = m_data->m_exec + " not found or not executable";
	return false;
    }
    string lib = path_cat(basedir, "lib");
    lib = path_cat(lib, "libaspell.so");
    if ((m_data->m_handle = dlopen(lib.c_str(), RTLD_LAZY)) == 0) {
	reason = "Could not open shared library [";
	reason += lib + "] : " + dlerror();
	return false;
    }
    string badnames;
    NMTOPTR(new_aspell_config, (struct AspellConfig *(*)()));
    NMTOPTR(aspell_config_replace, (int (*)(struct AspellConfig *, 
					    const char *, const char *)));
    NMTOPTR(new_aspell_speller, 
	    (struct AspellCanHaveError *(*)(struct AspellConfig *)));
    NMTOPTR(delete_aspell_config, 
	    (void (*)(struct AspellConfig *)));
    NMTOPTR(delete_aspell_can_have_error, 
	    (void (*)(struct AspellCanHaveError *)));
    NMTOPTR(to_aspell_speller, 
	    (struct AspellSpeller *(*)(struct AspellCanHaveError *)));
    NMTOPTR(aspell_speller_config, 
	    (struct AspellConfig *(*)(struct AspellSpeller *)));
    NMTOPTR(aspell_speller_suggest, 
	    (const struct AspellWordList *(*)(struct AspellSpeller *, 
					       const char *, int)));
    NMTOPTR(aspell_word_list_elements, 
	    (struct AspellStringEnumeration *(*)
	     (const struct AspellWordList *)));
    NMTOPTR(aspell_string_enumeration_next, 
	    (const char * (*)(struct AspellStringEnumeration *)));
    NMTOPTR(delete_aspell_string_enumeration, 
	    (void (*)(struct AspellStringEnumeration *)));
    NMTOPTR(aspell_error, 
	    (const struct AspellError*(*)(const struct AspellCanHaveError *)));
    NMTOPTR(aspell_error_message,
	    (const char *(*)(const struct AspellCanHaveError *)));
    NMTOPTR(aspell_speller_error_message, 
	    (const char *(*)(const struct AspellSpeller *)));
    NMTOPTR(delete_aspell_speller, (void (*)(struct AspellSpeller *)));

    if (!badnames.empty()) {
	reason = string("Aspell::init: symbols not found:") + badnames;
	return false;
    }
    return true;
}

bool Aspell::ok()
{
    return m_data != 0 && m_data->m_handle != 0;
}

string Aspell::dicPath()
{
    return path_cat(m_conf->getConfDir(), 
		    string("aspdict.") + m_lang + string(".rws"));
}

bool Aspell::buildDict(Rcl::Db &db, string &reason)
{
  string term;

  //  Il faut nettoyer la liste, peut-etre faire un tri unique (verifier), 
  //      puis construire le dico 
  Rcl::TermIter *tit = db.termWalkOpen();
  if (tit == 0) {
      reason = "termWalkOpen failed\n";
      return false;
  }
  ExecCmd aspell;
  list<string> args;
  // aspell --lang=[lang] create master [dictApath]
  args.push_back(string("--lang=")+ m_lang);
  args.push_back("create");
  args.push_back("master");
  args.push_back(dicPath());
  //  aspell.setStderr("/dev/null");
  string allterms;
  while (db.termWalkNext(tit, term)) {
      // Filter out terms beginning with upper case (special stuff) and 
      // containing numbers
      if (term.empty())
	  continue;
      if ('A' <= term.at(0) && term.at(0) <= 'Z')
	  continue;
      if (term.find_first_of("0123456789+-._@") != string::npos)
	  continue;
      allterms += term + "\n";
      //      std::cout << "[" << term << "]" << std::endl;
  }
  db.termWalkClose(tit);
  aspell.doexec(m_data->m_exec, args, &allterms);
  return true;
}


bool Aspell::suggest(Rcl::Db &db,
		     string &term, list<string> &suggestions, string &reason)
{
    AspellCanHaveError *ret;
    AspellSpeller *speller;
    AspellConfig *config;

    config = aapi.new_aspell_config();

    aapi.aspell_config_replace(config, "lang", m_lang.c_str());
    aapi.aspell_config_replace(config, "encoding", "utf-8");
    aapi.aspell_config_replace(config, "master", dicPath().c_str());
    aapi.aspell_config_replace(config, "sug-mode", "fast");
    //    aapi.aspell_config_replace(config, "sug-edit-dist", "2");
    ret = aapi.new_aspell_speller(config);
    aapi.delete_aspell_config(config);

    if (aapi.aspell_error(ret) != 0) {
	reason = aapi.aspell_error_message(ret);
	aapi.delete_aspell_can_have_error(ret);
	return false;
    }
    speller = aapi.to_aspell_speller(ret);
    config = aapi.aspell_speller_config(speller);
    const AspellWordList *wl = 
	aapi.aspell_speller_suggest(speller, term.c_str(), term.length());
    if (wl == 0) {
	reason = aapi.aspell_speller_error_message(speller);
	return false;
    }
    AspellStringEnumeration *els = aapi.aspell_word_list_elements(wl);
    const char *word;
    while ((word = aapi.aspell_string_enumeration_next(els)) != 0) {
	// stemDiffers checks that the word exists (we don't want
	// aspell computed stuff, only exact terms from the dictionary),
	// and that it stems differently to the base word (else it's not 
	// useful to expand the search). Or is it ? 
        // ******** This should depend if
	// stemming is turned on or not for querying  *******
	string sw(word);
	if (db.termExists(sw) && db.stemDiffers("english", sw, term))
	    suggestions.push_back(word);
    }
    aapi.delete_aspell_string_enumeration(els);
    aapi.delete_aspell_speller(speller);
    // Config belongs to speller here? aapi.delete_aspell_config(config);
    return true;
}


#else // TEST_RCLASPELL test driver ->

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <iostream>
using namespace std;

#include "rclinit.h"
#include "rclconfig.h"
#include "rcldb.h"
#include "rclaspell.h"

static char *thisprog;
RclConfig *rclconfig;
Rcl::Db rcldb;

static char usage [] =
" -b : build dictionary\n"
" -s <term>: suggestions for term\n"
"\n\n"
;
static void
Usage(void)
{
    fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_s	  0x2 
#define OPT_b	  0x4 

int main(int argc, char **argv)
{
    string word;
    
    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
	(*argv)++;
	if (!(**argv))
	    /* Cas du "adb - core" */
	    Usage();
	while (**argv)
	    switch (*(*argv)++) {
	    case 'b':	op_flags |= OPT_b; break;
	    case 's':	op_flags |= OPT_s; if (argc < 2)  Usage();
		word = *(++argv);
		argc--; 
		goto b1;
	    default: Usage();	break;
	    }
    b1: argc--; argv++;
    }

    if (argc != 0 || op_flags == 0)
	Usage();

    string reason;
    rclconfig = recollinit(0, 0, reason);
    if (!rclconfig || !rclconfig->ok()) {
	fprintf(stderr, "Configuration problem: %s\n", reason.c_str());
	exit(1);
    }

    string dbdir = rclconfig->getDbDir();
    if (dbdir.empty()) {
	fprintf(stderr, "No db directory in configuration");
	exit(1);
    }

    if (!rcldb.open(dbdir, Rcl::Db::DbRO, 0)) {
	fprintf(stderr, "Could not open database in %s\n", dbdir.c_str());
	exit(1);
    }

    string lang = "en";

    Aspell aspell(rclconfig, lang);

    if (!aspell.init("/usr/local", reason)) {
	cerr << "Init failed: " << reason << endl;
	exit(1);
    }
    if (op_flags & OPT_b) {
	if (!aspell.buildDict(rcldb, reason)) {
	    cerr << "buildDict failed: " << reason << endl;
	    exit(1);
	}
    } else {
	list<string> suggs;
	if (!aspell.suggest(rcldb, word, suggs, reason)) {
	    cerr << "suggest failed: " << reason << endl;
	    exit(1);
	}
	cout << "Suggestions for " << word << ":" << endl;
	for (list<string>::iterator it = suggs.begin(); 
	     it != suggs.end(); it++) {
	    cout << *it << endl;
	}
    }
    exit(0);
}

#endif // TEST_RCLASPELL test driver
