#ifndef _RCLASPELL_H_INCLUDED_
#define _RCLASPELL_H_INCLUDED_
/* @(#$Id: rclaspell.h,v 1.1 2006-10-09 14:05:35 dockes Exp $  (C) 2006 J.F.Dockes */

/// Class to interface an aspell speller.

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
    /** Get hold of the aspell command and shared library */
    bool init(const string &basedir, string &reason); 
    /**  Build dictionary out of index term list. This is done at the end
     * of an indexing pass. */
    bool buildDict(Rcl::Db &db, string &reason);
    /** Return a list of possible expansions for user term */
    bool suggest(Rcl::Db &db, string &term, list<string>suggestions, 
		 string &reason);
    string dicPath();

 private:
    RclConfig  *m_conf;
    string      m_lang;
    AspellData *m_data;
};

#endif /* _RCLASPELL_H_INCLUDED_ */
