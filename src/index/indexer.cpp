#ifndef lint
static char rcsid[] = "@(#$Id: indexer.cpp,v 1.71 2008-12-17 08:01:40 dockes Exp $ (C) 2004 J.F.Dockes";
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
#include "autoconfig.h"
#endif

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "debuglog.h"
#include "indexer.h"

ConfIndexer::~ConfIndexer()
{
     deleteZ(m_fsindexer);
}

bool ConfIndexer::index(bool resetbefore)
{
    list<string> tdl = m_config->getTopdirs();
    if (tdl.empty()) {
	m_reason = "Top directory list (topdirs param.) not found in config"
	    "or Directory list parse error";
	return false;
    }
    
    // (In theory) Each top level directory to be indexed can be
    // associated with a different database. We first group the
    // directories by database: it is important that all directories
    // for a database be indexed at once so that deleted file cleanup
    // works.
    // In practise we have a single db per configuration, but this
    // code doesn't hurt anyway
    list<string>::iterator dirit;
    map<string, list<string> > dbmap;
    map<string, list<string> >::iterator dbit;
    for (dirit = tdl.begin(); dirit != tdl.end(); dirit++) {
	string dbdir;
	string doctopdir = *dirit;
	m_config->setKeyDir(doctopdir);
	dbdir = m_config->getDbDir();
	if (dbdir.empty()) {
	    LOGERR(("ConfIndexer::index: no database directory in "
		    "configuration for %s\n", doctopdir.c_str()));
	    m_reason = "No database directory set for " + doctopdir;
	    return false;
	}
	dbit = dbmap.find(dbdir);
	if (dbit == dbmap.end()) {
	    list<string> l;
	    l.push_back(doctopdir);
	    dbmap[dbdir] = l;
	} else {
	    dbit->second.push_back(doctopdir);
	}
    }
    m_config->setKeyDir("");

    // The dbmap now has dbdir as key and directory lists as values.
    // Index each directory group in turn
    for (dbit = dbmap.begin(); dbit != dbmap.end(); dbit++) {
	m_fsindexer = new FsIndexer(m_config, m_updater);
	if (!m_fsindexer->indexTrees(resetbefore, &dbit->second)) {
	    deleteZ(m_fsindexer);
	    m_reason = "Failed indexing in " + dbit->first;
	    return false;
	}
	deleteZ(m_fsindexer);
    }
    return true;
}
