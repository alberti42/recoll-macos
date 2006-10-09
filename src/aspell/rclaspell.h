#ifndef _RCLASPELL_H_INCLUDED_
#define _RCLASPELL_H_INCLUDED_
/* @(#$Id: rclaspell.h,v 1.2 2006-10-09 16:37:08 dockes Exp $  (C) 2006 J.F.Dockes */

/**
 * Aspell speller interface class.
 *
 * Aspell is used to let the user find about spelling variations that may 
 * exist in the document set for a given word.
 * A specific aspell dictionary is created out of all the terms in the 
 * xapian index, and we then use it to expand a term to spelling neighbours.
 * We use the aspell C api for term expansion, but have 
 * to execute the program to create dictionaries.
 */

#include <string>
#include <list>

#include "rclconfig.h"
#include "rcldb.h"

#ifndef NO_NAMESPACES
using std::string;
using std::list;
#endif // NO_NAMESPACES

class AspellData;

class Aspell {
 public:
    Aspell(RclConfig *cnf, const string &lang) 
	: m_conf(cnf), m_lang(lang), m_data(0) {};
    ~Aspell();

    /** Check health */
    bool ok();

    /** Find the aspell command and shared library, init function pointers */
    bool init(const string &basedir, string &reason); 

    /**  Build dictionary out of index term list. This is done at the end
     * of an indexing pass. */
    bool buildDict(Rcl::Db &db, string &reason);

    /** Return a list of possible expansions for a given word */
    bool suggest(Rcl::Db &db, string &term, list<string> &suggestions, 
		 string &reason);

 private:
    string dicPath();
    RclConfig  *m_conf;
    string      m_lang;
    AspellData *m_data;
};

#endif /* _RCLASPELL_H_INCLUDED_ */
