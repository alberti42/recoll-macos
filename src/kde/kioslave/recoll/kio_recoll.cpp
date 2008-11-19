#ifndef lint
static char rcsid[] = "@(#$Id: kio_recoll.cpp,v 1.14 2008-11-19 12:28:59 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

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

#include "rclconfig.h"
#include "rcldb.h"
#include "rclinit.h"
#include "pathut.h"
#include "searchdata.h"
#include "rclquery.h"
#include "wasastringtoquery.h"
#include "wasatorcl.h"
#include "kio_recoll.h"

using namespace KIO;

static RclConfig *rclconfig;
RclConfig *RclConfig::getMainConfig()
{
  return rclconfig;
}

RecollProtocol::RecollProtocol(const QByteArray &pool, const QByteArray &app) 
    : SlaveBase("recoll", pool, app), m_initok(false), 
      m_rclconfig(0), m_rcldb(0)
{
    string reason;
    rclconfig = m_rclconfig = recollinit(0, 0, m_reason);
    if (!m_rclconfig || !m_rclconfig->ok()) {
	m_reason = string("Configuration problem: ") + reason;
	return;
    }
    m_dbdir = m_rclconfig->getDbDir();
    if (m_dbdir.empty()) {
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
    m_initok = true;
    return;
}

RecollProtocol::~RecollProtocol()
{
    delete m_rcldb;
    delete m_rclconfig;
}

void RecollProtocol::mimetype(const KUrl & /*url*/)
{
  mimeType("text/html");
  finished();
}

bool RecollProtocol::maybeOpenDb(string &reason)
{
    if (!m_rcldb) {
	reason = "Internal error: initialization error";
	return false;
    }
    if (!m_rcldb->isopen() && !m_rcldb->open(m_dbdir, 
					     m_rclconfig->getStopfile(),
					     Rcl::Db::DbRO)) {
	reason = "Could not open database in " + m_dbdir;
	return false;
    }
    return true;
}

void RecollProtocol::get(const KUrl & url)
{
    kDebug() << "RecollProtocol::get:" << url << endl;

    mimeType("text/html");

    if (!m_initok || !maybeOpenDb(m_reason)) {
	outputError(m_reason.c_str());
	finished();
	return;
    }

    string iconsdir;
    m_rclconfig->getConfParam("iconsdir", iconsdir);
    if (iconsdir.empty()) {
	iconsdir = path_cat("/usr/local/share/recoll", "images");
    } else {
	iconsdir = path_tildexpand(iconsdir);
    }

    QString path = url.path();
    kDebug() << "RecollProtocol::get:path:" << path << endl;
    QByteArray u8 =  path.toUtf8();

    RefCntr<Rcl::SearchData> sdata = wasaStringToRcl((const char*)u8, m_reason);
    sdata->setStemlang("english");

    RefCntr<Rcl::Query>query(new Rcl::Query(m_rcldb));
    if (!query->setQuery(sdata)) {
	m_reason = "Internal Error: setQuery failed";
	outputError(m_reason.c_str());
	finished();
	return;
    }

    string explain = sdata->getDescription();

    QByteArray output;
    QTextStream os(&output, QIODevice::ReadWrite);

    os << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">" << endl;
    os << "<html><head>" << endl;
    os << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << endl;
    os << "<meta http-equiv=\"Pragma\" content=\"no-cache\">" << endl;
    os << "<title>Recoll: query results</title>" << endl;
    os << "</head><body>" << endl;

    os << "<p><b>Actual query performed: </b>" << endl;
    os << explain.c_str() << "</p>";

    Rcl::Doc doc;
    int cnt = query->getResCnt();
    for (int i = 0; i < cnt; i++) {
	string sh;
	doc.erase();

	if (!query->getDoc(i, doc)) {
	    // This may very well happen for history if the doc has
	    // been removed since. So don't treat it as fatal.
	    doc.meta[Rcl::Doc::keykw] = string("Unavailable document");
	}

	string iconname = m_rclconfig->getMimeIconName(doc.mimetype);
	if (iconname.empty())
	    iconname = "document";
	string imgfile = iconsdir + "/" + iconname + ".png";

	string result;
	if (!sh.empty())
	    result += string("<p><b>") + sh + "</p>\n<p>";
	else
	    result = "<p>";

	char perbuf[10];
	sprintf(perbuf, "%3d%%", doc.pc);
	if (doc.meta[Rcl::Doc::keytt].empty()) 
	    doc.meta[Rcl::Doc::keytt] = path_getsimple(doc.url);
	char datebuf[100];
	datebuf[0] = 0;
	if (!doc.dmtime.empty() || !doc.fmtime.empty()) {
	    time_t mtime = doc.dmtime.empty() ?
		atol(doc.fmtime.c_str()) : atol(doc.dmtime.c_str());
	    struct tm *tm = localtime(&mtime);
	    strftime(datebuf, 99, 
		     "<i>Modified:</i>&nbsp;%Y-%m-%d&nbsp;%H:%M:%S", tm);
	}
	result += "<a href=\"" + doc.url + "\">" +
	    "<img src=\"file://" + imgfile + "\" align=\"left\">" + "</a>";
	string abst = escapeHtml(doc.meta[Rcl::Doc::keyabs]);
	result += string(perbuf) + " <b>" + doc.meta[Rcl::Doc::keytt] + "</b><br>" +
	    doc.mimetype + "&nbsp;" +
	    (datebuf[0] ? string(datebuf) + "<br>" : string("<br>")) +
	    (!abst.empty() ? abst + "<br>" : string("")) +
	    (!doc.meta[Rcl::Doc::keytt].empty() ? doc.meta[Rcl::Doc::keykw] + 
	     "<br>" : string("")) +
	    "<a href=\"" + doc.url + "\">" + doc.url + "</a><br></p>\n";

	QString str = QString::fromUtf8(result.c_str(), result.length());
	os << str;
    }

    os << "</body></html>";
    
    data(output);
    kDebug() << "call finished" << endl;
    finished();
}

void RecollProtocol::outputError(const QString& errmsg)
{
    QByteArray array;
    QTextStream os(&array, QIODevice::WriteOnly);

    os << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Strict//EN\">" << endl;
    os << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << endl;
    os << "<title>" << "Recoll output" << "</title>\n" << endl;
    os << "</head>" << endl;
    os << "<body><h1>Recoll Error</h1>" << errmsg << "</body>" << endl;
    os << "</html>" << endl;
    data(array);
}

// Note: KDE_EXPORT is actually needed on Unix when building with
// cmake. Says something like __attribute__(visibility(defautl))
// (cmake apparently sets all symbols to not exported)
extern "C" {KDE_EXPORT int kdemain(int argc, char **argv);}

int kdemain(int argc, char **argv)
{
    FILE*mf = fopen("/tmp/toto","w");
    fprintf(mf, "KIORECOLL\n");
    fclose(mf);
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

