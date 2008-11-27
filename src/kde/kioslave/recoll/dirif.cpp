#ifndef lint
static char rcsid[] = "@(#$Id: dirif.cpp,v 1.2 2008-11-27 17:48:43 dockes Exp $ (C) 2008 J.F.Dockes";
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
    
    entry.insert(KIO::UDSEntry::UDS_TARGET_URL, doc.url.c_str());
    //    entry.insert(KIO::UDSEntry::UDS_URL, "recoll://search/query/1");
    return entry;
}


void RecollProtocol::stat(const KUrl & url)
{
    kDebug() << url << endl ;

    QString path = url.path();
    KIO::UDSEntry entry;
    if (!path.compare("/"))
	entry.insert(KIO::UDSEntry::UDS_NAME, "/welcome");
    else
	entry.insert(KIO::UDSEntry::UDS_NAME, url.path());
    entry.insert(KIO::UDSEntry::UDS_TARGET_URL, url.url());
    entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
    statEntry(entry);
    finished();
}

// From kio_beagle
void RecollProtocol::createRootEntry(KIO::UDSEntry& entry)
{
    // home directory
    entry.clear();
    entry.insert( KIO::UDSEntry::UDS_NAME, ".");
    entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.insert( KIO::UDSEntry::UDS_ACCESS, 0700);
    entry.insert( KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory");
}

void RecollProtocol::createGoHomeEntry(KIO::UDSEntry& entry)
{
    // status file
    entry.clear();
    entry.insert(KIO::UDSEntry::UDS_NAME, "Recoll home (click me)");
    entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
    entry.insert(KIO::UDSEntry::UDS_TARGET_URL, "recoll:///welcome");
    entry.insert(KIO::UDSEntry::UDS_ACCESS, 0500);
    entry.insert(KIO::UDSEntry::UDS_MIME_TYPE, "text/html");
    entry.insert(KIO::UDSEntry::UDS_ICON_NAME, "recoll");
}


void RecollProtocol::listDir(const KUrl& url)
{
    kDebug() << url << endl;

    // It seems that when the request is from konqueror
    // autocompletion it comes with a / at the end, which offers
    // an opportunity to not perform it.
    if (url.path() != "/" && url.path().endsWith("/")) {
	kDebug() << "Endswith/" << endl;
	error(-1, "");
	return;
    }

    if (!m_initok || !maybeOpenDb(m_reason)) {
	string reason = "RecollProtocol::listDir: Init error:" + m_reason;
	error(KIO::ERR_SLAVE_DEFINED, reason.c_str());
	return;
    }
    if (url.path().isEmpty() || url.path() == "/") {
	kDebug() << "list /" << endl;

	UDSEntryList entries;
	KIO::UDSEntry entry;

	// entry for '/'
	createRootEntry(entry);
	//	listEntry(entry, false);
	entries.append(entry);

	// entry for 'Information'
	createGoHomeEntry(entry);
	//	listEntry(entry, false);
	entries.append(entry);

	listEntries(entries);
	finished();
	return;
    }

    QString query, opt;
    URLToQuery(url, query, opt);
    kDebug() << "Query: " << query;
    if (!query.isEmpty()) {
	if (!doSearch(query, opt.toUtf8().at(0)))
	    return;
    } else {
	finished();
	return;
    }

    vector<ResListEntry> page;
    int pagelen = m_source->getSeqSlice(0, 100, page);
    kDebug() << "Got " << pagelen << " results.";
    UDSEntryList entries;
    for (int i = 0; i < pagelen; i++) {
	entries.append(resultToUDSEntry(page[i].doc));
    }
    listEntries(entries);
    finished();
}
