#ifndef lint
static char rcsid[] = "@(#$Id: dirif.cpp,v 1.4 2008-12-01 15:36:52 dockes Exp $ (C) 2008 J.F.Dockes";
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

#include <kdeversion.h>

#if KDE_IS_VERSION(4,1,0)
// Couldn't get listDir() to work with kde 4.0, konqueror keeps
// crashing because of kdirmodel, couldn't find a workaround (not
// saying it's impossible)...

#include <sys/stat.h>

#include <kurl.h>
#include <kio/global.h>
#include <kstandarddirs.h>

#include <kdebug.h>

#include "kio_recoll.h"
#include "pathut.h"

using namespace KIO;

static const QString resultBaseName("recollResult");

//
bool RecollProtocol::isRecollResult(const KUrl &url, int *num)
{
    *num = -1;
    kDebug() << "url" << url << "m_srchStr" << m_srchStr;
    // Does the url look like a recoll search result ??
    if (!url.host().isEmpty() || url.path().isEmpty() || 
	url.protocol().compare("recoll")) 
	return false;
    QString path = url.path();
    if (!path.startsWith("/")) 
	return false;
    // Look for the last '/' and check if it is followed by
    // resultBaseName (riiiight...)
    int slashpos = path.lastIndexOf("/");
    if (slashpos == -1 || slashpos == 0 || slashpos == path.length() -1)
	return false;
    slashpos++;
    kDebug() << "Comparing " << path.mid(slashpos, resultBaseName.length()) <<
	"and " << resultBaseName;
    if (path.mid(slashpos, resultBaseName.length()).compare(resultBaseName))
	return false;

    QString snum = path.mid(slashpos + resultBaseName.length());
    sscanf(snum.toAscii(), "%d", num);
    if (*num == -1)
	return false;

    kDebug() << "URL analysis ok, num:" << *num;

    // We do have something that ressembles a recoll result locator. Check if
    // this matches the current search, else have to run the requested one
    QString searchstring = path.mid(1, slashpos-2);
    kDebug() << "Comparing search strings" << m_srchStr << "and" << searchstring;
    if (searchstring.compare(m_srchStr)) {
	kDebug() << "Starting new search";
	if (!doSearch(searchstring))
	    return false;
    }

    return true;
}

static const UDSEntry resultToUDSEntry(const Rcl::Doc& doc, int num)
{
    UDSEntry entry;
    
    KUrl url(doc.url.c_str());
    kDebug() << doc.url.c_str();

    entry.insert(KIO::UDSEntry::UDS_DISPLAY_NAME, url.fileName());
    char cnum[30];sprintf(cnum, "%d", num);
    entry.insert(KIO::UDSEntry::UDS_NAME, resultBaseName + cnum);

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
    kDebug() << "entry URL: " << doc.url.c_str();
    entry.insert(KIO::UDSEntry::UDS_TARGET_URL, doc.url.c_str());

    return entry;
}


// From kio_beagle
static void createRootEntry(KIO::UDSEntry& entry)
{
    entry.clear();
    entry.insert( KIO::UDSEntry::UDS_NAME, ".");
    entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.insert( KIO::UDSEntry::UDS_ACCESS, 0700);
    entry.insert( KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory");
}

// Points to html query screen
static void createGoHomeEntry(KIO::UDSEntry& entry)
{
    entry.clear();
    entry.insert(KIO::UDSEntry::UDS_NAME, "search");
    entry.insert(KIO::UDSEntry::UDS_DISPLAY_NAME, "Recoll search (click me)");
    entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
    entry.insert(KIO::UDSEntry::UDS_TARGET_URL, "recoll:///welcome");
    entry.insert(KIO::UDSEntry::UDS_ACCESS, 0500);
    entry.insert(KIO::UDSEntry::UDS_MIME_TYPE, "text/html");
    entry.insert(KIO::UDSEntry::UDS_ICON_NAME, "recoll");
}

// Points to help file
static void createGoHelpEntry(KIO::UDSEntry& entry)
{
    QString location = 
	KStandardDirs::locate("data", "kio_recoll/help.html");
    entry.clear();
    entry.insert(KIO::UDSEntry::UDS_NAME, "help");
    entry.insert(KIO::UDSEntry::UDS_DISPLAY_NAME, "Recoll help (click me first)");
    entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
    entry.insert(KIO::UDSEntry::UDS_TARGET_URL, QString("file://") +
		 location);
    entry.insert(KIO::UDSEntry::UDS_ACCESS, 0500);
    entry.insert(KIO::UDSEntry::UDS_MIME_TYPE, "text/html");
    entry.insert(KIO::UDSEntry::UDS_ICON_NAME, "help");
}

void RecollProtocol::stat(const KUrl & url)
{
    kDebug() << url << endl ;
    int num = -1;
    QString path = url.path();
    KIO::UDSEntry entry;
    if (!path.compare("/")) {
	createRootEntry(entry);
    } else if (!path.compare("/help")) {
	createGoHelpEntry(entry);
    } else if (!path.compare("/search")) {
	createGoHomeEntry(entry);
    } else if (isRecollResult(url, &num)) {
	// If this url starts with the current search url appended with a 
	// result name appended, let's stat said result.
	Rcl::Doc doc;
	if (num >= 0 && !m_source.isNull()  && m_source->getDoc(num, doc)) {
	    entry = resultToUDSEntry(doc, num);
	}
    } else {
	// ?? 
    }

    entry.insert(KIO::UDSEntry::UDS_TARGET_URL, url.url());
    statEntry(entry);
    finished();
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
	entries.append(entry);

	createGoHomeEntry(entry);
	entries.append(entry);
	createGoHelpEntry(entry);
	entries.append(entry);

	listEntries(entries);
	finished();
	return;
    }

    
    QString query, opt;
    URLToQuery(url, query, opt);
    kDebug() << "Query: " << query;
    if (!query.isEmpty()) {
	if (!doSearch(query, opt))
	    return;
    } else {
	finished();
	return;
    }

    static int numentries = -1;
    if (numentries == -1) {
	if (o_rclconfig)
	    o_rclconfig->getConfParam("kio_max_direntries", &numentries);
	if (numentries == -1)
	    numentries = 100;
    }

    vector<ResListEntry> page;
    int pagelen = m_source->getSeqSlice(0, numentries, page);
    kDebug() << "Got " << pagelen << " results.";
    UDSEntryList entries;
    for (int i = 0; i < pagelen; i++) {
	entries.append(resultToUDSEntry(page[i].doc, i));
    }
    listEntries(entries);
    finished();
}
#endif // KDE 4.1+
