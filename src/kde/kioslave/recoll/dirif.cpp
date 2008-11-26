#ifndef lint
static char rcsid[] = "@(#$Id: dirif.cpp,v 1.1 2008-11-26 15:03:41 dockes Exp $ (C) 2008 J.F.Dockes";
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

/*
 * A lot of code in this file was copied from kio_beagle 0.4.0,
 * which is a GPL program. The authors listed are:
 * Debajyoti Bera <dbera.web@gmail.com>
 * 
 * KDE4 port:
 * Stephan Binner <binner@kde.org>
 */

#include <sys/stat.h>

#include <kurl.h>
#include <kio/global.h>

#include <kdebug.h>


#include "kio_recoll.h"

using namespace KIO;

const UDSEntry resultToUDSEntry(Rcl::Doc doc)
{
    
    UDSEntry entry;
    
    KUrl url(doc.url.c_str());
    kDebug() << doc.url.c_str();

    entry.insert(KIO::UDSEntry::UDS_NAME, url.fileName());
    if (!doc.mimetype.compare("application/x-fsdirectory")) {
	entry.insert(KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory");
    	entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    } else {
	entry.insert(KIO::UDSEntry::UDS_MIME_TYPE, doc.mimetype.c_str());
    	entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
    }
    entry.insert(KIO::UDSEntry::UDS_LOCAL_PATH, url.path());
    // For local files, supply the usual file stat information
    struct stat info;
    if (lstat(url.path().toAscii(), &info) >= 0) {
	entry.insert( KIO::UDSEntry::UDS_SIZE, info.st_size);
	entry.insert( KIO::UDSEntry::UDS_ACCESS, info.st_mode);
	entry.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, info.st_mtime);
    	entry.insert( KIO::UDSEntry::UDS_ACCESS_TIME, info.st_atime);
    	entry.insert( KIO::UDSEntry::UDS_CREATION_TIME, info.st_ctime);
    }
    
    //    entry.insert(KIO::UDSEntry::UDS_URL, doc.url.c_str());
    //    entry.insert(KIO::UDSEntry::UDS_URL, "recoll://search/query/1");
    return entry;
}


// Don't know that we really need this. It's used by beagle to return
// info on the virtual entries used to execute commands, and on the
// saved searches (appearing as directories)
void RecollProtocol::stat(const KUrl & url)
{
    kDebug() << url << endl ;

    QString path = url.path();
    KIO::UDSEntry entry;
    entry.insert(KIO::UDSEntry::UDS_NAME, url.path());
    entry.insert(KIO::UDSEntry::UDS_URL, url.url());
    entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
    statEntry(entry);
    finished();
}

void RecollProtocol::listDir(const KUrl& url)
{
    kDebug() << url << endl;
    if (!m_initok || !maybeOpenDb(m_reason)) {
	error(KIO::ERR_SLAVE_DEFINED, "Init error");
	return;
    }

    QString query, opt;
    URLToQuery(url, query, opt);
    kDebug() << "Query: " << query;
    if (!query.isEmpty()) {
	doSearch(query, opt.toUtf8().at(0));
    } else {
	finished();
	return;
    }

    vector<ResListEntry> page;
    int pagelen = m_source->getSeqSlice(0, 20, page);
    kDebug() << "Got " << pagelen << " results.";
    UDSEntryList entries;
    for (int i = 0; i < pagelen; i++) {
	entries.append(resultToUDSEntry(page[i].doc));
    }
    listEntries(entries);
    //    finished();
}
