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
#ifndef _DOCSEQDB_H_INCLUDED_
#define _DOCSEQDB_H_INCLUDED_
/* @(#$Id: docseqdb.h,v 1.1 2007-01-19 10:32:39 dockes Exp $  (C) 2004 J.F.Dockes */
#include <string>
#include <list>
#ifndef NO_NAMESPACES
using std::string;
using std::list;
#endif

#include "docseq.h"
namespace Rcl {
class Db;
}

/** A DocSequence from a Db query (there should be one active for this
    to make sense */
class DocSequenceDb : public DocSequence {
 public:
    DocSequenceDb(Rcl::Db *d, const string &t) : 
	DocSequence(t), m_db(d), m_rescnt(-1) 
	{}
    virtual ~DocSequenceDb() {}
    virtual bool getDoc(int num, Rcl::Doc &doc, int *percent, string * = 0);
    virtual int getResCnt();
    virtual void getTerms(list<string>&);
    virtual string getAbstract(Rcl::Doc &doc);

 private:
    Rcl::Db *m_db;
    int      m_rescnt;
};

#endif /* _DOCSEQDB_H_INCLUDED_ */
