#ifndef lint
static char rcsid[] = "@(#$Id: kio_recoll.cpp,v 1.16 2008-11-26 15:03:41 dockes Exp $ (C) 2005 J.F.Dockes";
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h> 

#include <string>

using namespace std;
#include <qglobal.h>

#if (QT_VERSION < 0x040000)
#include <qfile.h>
#include <qtextstream.h>
#include <qstring.h>
#else
#include <QFile>
#include <QTextStream>
#include <QByteArray>
#endif

#include <kdebug.h>

//#include <kinstance.h>
#include <kcomponentdata.h>
#include <kstandarddirs.h>

#include "rclconfig.h"
#include "rcldb.h"
#include "rclinit.h"
#include "pathut.h"
#include "searchdata.h"
#include "rclquery.h"
#include "wasastringtoquery.h"
#include "wasatorcl.h"
#include "kio_recoll.h"
#include "docseqdb.h"
#include "readfile.h"
#include "smallut.h"

using namespace KIO;

RclConfig *RecollProtocol::o_rclconfig;
RclConfig *RclConfig::getMainConfig()
{
    return RecollProtocol::o_rclconfig;
}

RecollProtocol::RecollProtocol(const QByteArray &pool, const QByteArray &app) 
    : SlaveBase("recoll", pool, app), m_initok(false), m_rcldb(0)
{
    kDebug() << endl;
    if (o_rclconfig == 0) {
	o_rclconfig = recollinit(0, 0, m_reason);
	if (!o_rclconfig || !o_rclconfig->ok()) {
	    m_reason = string("Configuration problem: ") + m_reason;
	    return;
	}
    }
    if (o_rclconfig->getDbDir().empty()) {
	// Note: this will have to be replaced by a call to a
	// configuration buildin dialog for initial configuration
	m_reason = "No db directory in configuration ??";
	return;
    }

    m_rcldb = new Rcl::Db;
    if (!m_rcldb) {
	m_reason = "Could not build database object. (out of memory ?)";
	return;
    }
    m_pager.setParent(this);
    m_initok = true;
    return;
}

// There should be an object counter somewhere to delete the config when done.
// Doesn't seem needed in the kio context.
RecollProtocol::~RecollProtocol()
{
    kDebug() << endl;
    delete m_rcldb;
}

bool RecollProtocol::maybeOpenDb(string &reason)
{
    if (!m_rcldb) {
	reason = "Internal error: initialization error";
	return false;
    }
    if (!m_rcldb->isopen() && !m_rcldb->open(o_rclconfig->getDbDir(), 
					     o_rclconfig->getStopfile(),
					     Rcl::Db::DbRO)) {
	reason = "Could not open database in " + o_rclconfig->getDbDir();
	return false;
    }
    return true;
}

void RecollProtocol::mimetype(const KUrl &url)
{
    kDebug() << url << endl;
    if (0) 
	mimeType("text/html");
    else
	mimeType("inode/directory");
    finished();
}

bool RecollProtocol::URLToQuery(const KUrl &url, QString& q, QString& opt)
{
    if (url.host().isEmpty()) {
	q = url.path();
	opt = "l";
    } else {
	// Decode the forms' arguments
	q = url.queryItem("q");
	opt = url.queryItem("qtp");
	if (opt.isEmpty()) {
	    opt = "l";
	} 
    }
    return true;
}

void RecollProtocol::get(const KUrl & url)
{
    kDebug() << url << endl;

    if (!m_initok || !maybeOpenDb(m_reason)) {
	error(KIO::ERR_SLAVE_DEFINED, "Init error");
	return;
    }
    error(KIO::ERR_IS_DIRECTORY, QString::null);
    return;

    QString host = url.host();
    QString path = url.path();

    kDebug() << "host:" << host << " path:" << path;

    if (host.isEmpty() && !path.compare("/")) {
	// recoll:/
	welcomePage();
	goto out;
    } else if ((!host.compare("search") && !path.compare("/query"))
	       || host.isEmpty()) {
	// Either "recoll://search/query?query args"
	// Or "recoll: some search string"
	QString query, opt;
	URLToQuery(url, query, opt);
	if (!query.isEmpty()) {
	    htmlDoSearch(query, opt.toUtf8().at(0));
	    goto out;
	}
    } else if (!host.compare("command")) {
	if (!path.isEmpty()) {
	    if (path.indexOf("/Page") == 0) {
		int newpage = 0;
		sscanf(path.toUtf8(), "/Page%d", &newpage);
		if (newpage > m_pager.pageNumber()) {
		    int npages = newpage - m_pager.pageNumber();
		    for (int i = 0; i < npages; i++)
			m_pager.resultPageNext();
		} else if (newpage < m_pager.pageNumber()) {
		    int npages = m_pager.pageNumber() - newpage;
		    for (int i = 0; i < npages; i++) 
			m_pager.resultPageBack();
		}
		mimeType("text/html");
		m_pager.displayPage();
		goto out;
	    } else if (path.indexOf("/QueryDetails") == 0) {
		queryDetails();
		goto out;
	    } 
	}
    }
    kDebug() << "Unrecognized URL" << endl;
    outputError("unrecognized URL");
 out:
    finished();
    kDebug() << "done" << endl;
}

void RecollProtocol::doSearch(const QString& q, char opt)
{
    kDebug() << q << endl;
    string qs = (const char *)q.toUtf8();
    Rcl::SearchData *sd = 0;
    if (opt != 'l') {
	Rcl::SearchDataClause *clp = 0;
	if (opt == 'f') {
	    clp = new Rcl::SearchDataClauseFilename(qs);
	} else {
	    // If there is no white space inside the query, then the user
	    // certainly means it as a phrase.
	    bool isreallyaphrase = false;
	    if (qs.find_first_of(" \t") == string::npos)
		isreallyaphrase = true;
	    clp = isreallyaphrase ? 
		new Rcl::SearchDataClauseDist(Rcl::SCLT_PHRASE, qs, 0) :
		new Rcl::SearchDataClauseSimple(opt == 'o' ?
						Rcl::SCLT_OR : Rcl::SCLT_AND, 
						qs);
	}
	sd = new Rcl::SearchData(Rcl::SCLT_OR);
	if (sd && clp)
	    sd->addClause(clp);
    } else {
	kDebug() << "Parsing query: " << qs.c_str();
	sd = wasaStringToRcl(qs, m_reason);
    }
    if (!sd) {
	m_reason = "Internal Error: cant allocate new query";
	outputError(m_reason.c_str());
	finished();
	return;
    }

    RefCntr<Rcl::SearchData> sdata(sd);
    sdata->setStemlang("english");
    kDebug() << "Building query";
    RefCntr<Rcl::Query>query(new Rcl::Query(m_rcldb));
    if (!query->setQuery(sdata)) {
	m_reason = "Internal Error: setQuery failed";
	outputError(m_reason.c_str());
	finished();
	return;
    }
    kDebug() << "Building docsequence";
    DocSequenceDb *src = 
	new DocSequenceDb(RefCntr<Rcl::Query>(query), "Query results", sdata);
    if (src == 0) {
	kDebug() << "Cant' build result sequence";
	error(-1, QString::null);
	finished();
	return;
    }
    kDebug() << "Setting source";
    m_source = RefCntr<DocSequence>(src);
}

// Note: KDE_EXPORT is actually needed on Unix when building with
// cmake. Says something like __attribute__(visibility(defautl))
// (cmake apparently sets all symbols to not exported)
extern "C" {KDE_EXPORT int kdemain(int argc, char **argv);}

int kdemain(int argc, char **argv)
{
#ifdef KDE_VERSION_3
    KInstance instance("kio_recoll");
#else
    KComponentData instance("kio_recoll");
#endif
    kDebug() << "*** starting kio_recoll " << endl;

    if (argc != 4)  {
	kDebug() << "Usage: kio_recoll proto dom-socket1 dom-socket2\n" << endl;
	exit(-1);
    }

    RecollProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    kDebug() << "kio_recoll Done" << endl;
    return 0;
}
