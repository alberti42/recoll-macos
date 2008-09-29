#ifndef lint
static char rcsid[] = "@(#$Id: kio_recoll.cpp,v 1.10 2008-09-29 11:33:55 dockes Exp $ (C) 2005 J.F.Dockes";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h> 

#include <string>

using namespace std;

#include <qfile.h>

#include <kinstance.h>

#include "rclconfig.h"
#include "rcldb.h"
#include "rclinit.h"
#include "docseqdb.h"
#include "pathut.h"
#include "searchdata.h"

#include "plaintorich.h"

#include "kio_recoll.h"

using namespace KIO;
static RclConfig *rclconfig;
RclConfig *RclConfig::getMainConfig()
{
  return rclconfig;
}

RecollProtocol::RecollProtocol(const QCString &pool, const QCString &app) 
    : SlaveBase("recoll", pool, app), m_initok(false), 
      m_rclconfig(0), m_rcldb(0), m_docsource(0)
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
    delete m_docsource;
    delete m_rcldb;
    delete m_rclconfig;
}

bool RecollProtocol::maybeOpenDb(string &reason)
{
    if (!m_rcldb) {
	reason = "Internal error: initialization error";
	return false;
    }
    if (!m_rcldb->isopen() && !m_rcldb->open(m_dbdir, 
					     m_rclconfig->getStopfile(),
					     Rcl::Db::DbRO,
					     Rcl::Db::QO_STEM)) {
	reason = "Could not open database in " + m_dbdir;
	return false;
    }
    return true;
}

void RecollProtocol::get(const KURL & url)
{
    fprintf(stderr, "RecollProtocol::get %s\n", url.url().ascii());
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
    fprintf(stderr, "RecollProtocol::get:path [%s]\n", path.latin1());
    QCString u8 =  path.utf8();

    RefCntr<Rcl::SearchData> sdata(new Rcl::SearchData(Rcl::SCLT_OR));
    sdata->addClause(new Rcl::SearchDataClauseSimple(Rcl::SCLT_AND, 
						    (const char *)u8));
    Rcl::Query *query = new Rcl::Query(m_rcldb);
    if (!query->setQuery(sdata)) {
	m_reason = "Internal Error: setQuery failed";
	outputError(m_reason.c_str());
	finished();
	delete query;
	return;
    }

    if (m_docsource)
	delete m_docsource;

    m_docsource = new DocSequenceDb(RefCntr<Rcl::Query>(query), 
				    "Query results", sdata);

    QByteArray output;
    QTextStream os(output, IO_WriteOnly );
    os.setEncoding(QTextStream::UnicodeUTF8); 
    os << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">" << endl;
    os << "<html><head>" << endl;
    os << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << endl;
    os << "<meta http-equiv=\"Pragma\" content=\"no-cache\">" << endl;
    os << "<title>Recoll: query results</title>" << endl;
    os << "</head><body>" << endl;

    Rcl::Doc doc;
    for (int i = 0; i < 100; i++) {
	string sh;
	doc.erase();

	if (!m_docsource->getDoc(i, doc, &sh)) {
	    // This may very well happen for history if the doc has
	    // been removed since. So don't treat it as fatal.
	    doc.meta["abstract"] = string("Unavailable document");
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
	if (doc.meta["title"].empty()) 
	  doc.meta["title"] = path_getsimple(doc.url);
	char datebuf[100];
	datebuf[0] = 0;
	if (!doc.dmtime.empty() || !doc.fmtime.empty()) {
	    time_t mtime = doc.dmtime.empty() ?
		atol(doc.fmtime.c_str()) : atol(doc.dmtime.c_str());
	    struct tm *tm = localtime(&mtime);
	    strftime(datebuf, 99, 
		     "<i>Modified:</i>&nbsp;%Y-%m-%d&nbsp;%H:%M:%S", tm);
	}
	result += "<img src=\"file://" + imgfile + "\" align=\"left\">";
	string abst = escapeHtml(doc.meta["abstract"]);
	result += string(perbuf) + " <b>" + doc.meta["title"] + "</b><br>" +
	    doc.mimetype + "&nbsp;" +
	    (datebuf[0] ? string(datebuf) + "<br>" : string("<br>")) +
	    (!abst.empty() ? abst + "<br>" : string("")) +
	    (!doc.meta["keywords"].empty() ? doc.meta["keywords"] + 
	     "<br>" : string("")) +
	    "<a href=\"" + doc.url + "\">" + doc.url + "</a><br></p>\n";

	QString str = QString::fromUtf8(result.c_str(), result.length());
	os << str;
    }

    os << "</body></html>";
    
    data(output);
    data(QByteArray());

    fprintf(stderr, "RecollProtocol::get: calling finished\n");
    finished();
}

void RecollProtocol::outputError(const QString& errmsg)
{
    QByteArray array;
    QTextStream os(array, IO_WriteOnly);
    os.setEncoding(QTextStream::UnicodeUTF8);

    os << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Strict//EN\">" << endl;
    os << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << endl;
    os << "<title>" << "Recoll output" << "</title>\n" << endl;
    os << "</head>" << endl;
    os << "<body><h1>Recoll Error</h1>" << errmsg << "</body>" << endl;
    os << "</html>" << endl;

    data(array);
}


extern "C" { int KDE_EXPORT kdemain(int argc, char **argv); }

int kdemain(int argc, char **argv)
{
  fprintf(stderr, "KIO_RECOLL\n");
  KInstance instance("kio_recoll");

  if (argc != 4)  {
      fprintf(stderr, 
	      "Usage: kio_recoll protocol domain-socket1 domain-socket2\n");
      exit(-1);
  }

  RecollProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();

  return 0;
}
