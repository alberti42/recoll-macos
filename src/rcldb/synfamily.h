/* Copyright (C) 2012 J.F.Dockes
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
#ifndef _SYNFAMILY_H_INCLUDED_
#define _SYNFAMILY_H_INCLUDED_

/**
 * The Xapian synonyms mechanism can be used for many things beyond actual
 * synonyms, anything that would turn a string into a group of equivalents.
 * Unfortunately, it has only one keyspace. 
 * This class partitions the Xapian synonyms keyspace by using prefixes and
 * can provide different applications each with a family of keyspaces.
 * Two characters are reserved by the class and should not be used inside 
 * either family or member names: ':' and ';'
 * A synonym key for family "stemdb", member "french", key "somestem" 
 * looks like:
 *  :stemdb:french:somestem  -> somestem expansions
 * A special entry is used to list all the members for a family, e.g.:
 *  :stemdb;members  -> french, english ...
 */

#include <string>
#include <vector>

#include "xapian.h"

namespace Rcl {

class XapSynFamily {
public:
    /** 
     * Construct from readable xapian database and family name (ie: Stm)
     */
    XapSynFamily(Xapian::Database xdb, const std::string& familyname)
	: m_rdb(xdb)
    {
	m_prefix1 = std::string(":") + familyname;
    }

    /** Expand one term (e.g.: familier) inside one family number (e.g: french)
     */
    virtual bool synExpand(const std::string& fammember,
		   const std::string& key,
		   std::vector<std::string>& result);

    /** Retrieve all members of this family (e.g: french english german...) */
    virtual bool getMembers(std::vector<std::string>&);

    /** debug: list map for one member to stdout */
    virtual bool listMap(const std::string& fam); 

protected:
    Xapian::Database m_rdb;
    std::string m_prefix1;

    virtual std::string entryprefix(const std::string& member)
    {
	return m_prefix1 + ":" + member + ":";
    }
    virtual std::string memberskey()
    {
	return m_prefix1 + ";" + "members";
    }

};

class XapWritableSynFamily : public XapSynFamily {
public:
    /** Construct with Xapian db open for r/w */
    XapWritableSynFamily(Xapian::WritableDatabase db, const std::string& pfx)
	: XapSynFamily(db, pfx),  m_wdb(db)
    {
    }

    /** Delete all entries for one member (e.g. french), and remove from list
     * of members */
    virtual bool deleteMember(const std::string& membername);

    /** Add to list of members. Idempotent, does not affect actual expansions */
    virtual bool createMember(const std::string& membername);

    /** Add expansion list for term inside family member (e.g., inside
     *  the french member, add expansion for familier -> familier,
     * familierement, ... */
    virtual bool addSynonyms(const std::string& membername, 
			     const std::string& term, 
			     const std::vector<std::string>& trans);

protected:
    Xapian::WritableDatabase m_wdb;
};


//
// Prefixes are centrally defined here to avoid collisions
//
// Stem expansion family prefix. The family member name is the language
static const std::string synFamStem("Stm");
static const std::string synFamDiac("Dia");
static const std::string synFamCase("Cse");


}

#endif /* _SYNFAMILY_H_INCLUDED_ */
