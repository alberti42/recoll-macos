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
#ifndef TEST_SYNFAMILY

#include "autoconfig.h"

#include "debuglog.h"
#include "rcldb.h"
#include "rcldb_p.h"
#include "synfamily.h"

#include <iostream>

using namespace std;

namespace Rcl {

bool XapSynFamily::synExpand(const string& member, const string& term,
			     vector<string>& result)
{
    string key = entryprefix(member) + term;
    string ermsg;
    try {
	for (Xapian::TermIterator xit = m_rdb.synonyms_begin(key);
	     xit != m_rdb.synonyms_end(key); xit++) {
	    result.push_back(*xit);
	}
    } XCATCHERROR(ermsg);
    if (!ermsg.empty()) {
	LOGERR(("synFamily::synExpand: error for member [%s] term [%s]\n",
		member.c_str(), term.c_str()));
	return false;
    }
#if 0
    string out;
    stringsToString(result, out);
    LOGDEB0(("XapSynFamily::synExpand:%s: [%s] -> %s\n", member.c_str(), 
	     term.c_str(), out.c_str()));
#endif
    return true;
}

bool XapSynFamily::getMembers(vector<string>& members)
{
    string key = memberskey();
    string ermsg;
    try {
	for (Xapian::TermIterator xit = m_rdb.synonyms_begin(key);
	     xit != m_rdb.synonyms_end(key); xit++) {
	    members.push_back(*xit);
	}
    } XCATCHERROR(ermsg);
    if (!ermsg.empty()) {
	LOGERR(("XapSynFamily::getMembers: xapian error %s\n", ermsg.c_str()));
	return false;
    }
    return true;
}

bool XapSynFamily::listMap(const string& membername)
{
    string key = entryprefix(membername);
    string ermsg;
    try {
	for (Xapian::TermIterator kit = m_rdb.synonym_keys_begin(key);
	     kit != m_rdb.synonym_keys_end(key); kit++) {
	    cout << "[" << *kit << "] -> ";
	    for (Xapian::TermIterator xit = m_rdb.synonyms_begin(*kit);
		 xit != m_rdb.synonyms_end(*kit); xit++) {
		cout << *xit << " ";
	    }
	    cout << endl;
	}
    } XCATCHERROR(ermsg);
    if (!ermsg.empty()) {
	LOGERR(("XapSynFamily::listMap: xapian error %s\n", ermsg.c_str()));
	return false;
    }
    vector<string>members;
    getMembers(members);
    cout << "All family members: ";
    for (vector<string>::const_iterator it = members.begin();
	 it != members.end(); it++) {
	cout << *it << " ";
    }
    cout << endl;
    return true;
}

bool XapWritableSynFamily::deleteMember(const string& membername)
{
    string key = entryprefix(membername);

    for (Xapian::TermIterator xit = m_wdb.synonym_keys_begin(key);
	 xit != m_wdb.synonym_keys_end(key); xit++) {
	m_wdb.clear_synonyms(*xit);
    }
    m_wdb.remove_synonym(memberskey(), membername);
    return true;
}

bool XapWritableSynFamily::createMember(const string& membername)
{
    string ermsg;
    try {
	m_wdb.add_synonym(memberskey(), membername);
    } XCATCHERROR(ermsg);
    if (!ermsg.empty()) {
	LOGERR(("XapSynFamily::createMember: xapian error %s\n", ermsg.c_str()));
	return false;
    }
    return true;
}

bool XapWritableSynFamily::addSynonyms(const string& membername, 
				       const string& term, 
				       const vector<string>& trans)
{
    string key = entryprefix(membername) + term;
    string ermsg;
    try {
	for (vector<string>::const_iterator it = trans.begin();
	     it != trans.end(); it++) {
	    m_wdb.add_synonym(key, *it);
	}
    } XCATCHERROR(ermsg);
    if (!ermsg.empty()) {
	LOGERR(("XapSynFamily::addSynonyms: xapian error %s\n", ermsg.c_str()));
	return false;
    }
    return true;
}

}

#else  // TEST_SYNFAMILY 
#endif // TEST_SYNFAMILY
