#ifndef lint
static char rcsid[] = "@(#$Id: kio_recoll.cpp,v 1.15 2008-11-20 13:10:23 dockes Exp $ (C) 2005 J.F.Dockes";
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

bool RecollKioPager::append(const string& data)
{
    if (!m_parent) return false;
    m_parent->data(QByteArray(data.c_str()));
    return true;
}

string RecollKioPager::detailsLink()
{
    string chunk = "<a href=\"recoll://command/QueryDetails\">";
    chunk += tr("(show query)") + "</a>";
    return chunk;
}

const static string parformat = 
    "<a href=\"%U\"><img src=\"%I\" align=\"left\"></a>"
    "%R %S "
    "<a href=\"%U\">Open</a>&nbsp;&nbsp;<b>%T</b><br>"
    "%M&nbsp;%D&nbsp;&nbsp;&nbsp;<i>%U</i><br>"
    "%A %K";
const string &RecollKioPager::parFormat()
{
    return parformat;
}

string RecollKioPager::pageTop()
{
    return "<p align=\"center\"><a href=\"recoll://welcome\">New Search</a></p>";
}

string RecollKioPager::nextUrl()
{
    int pagenum = pageNumber();
    if (pagenum < 0)
	pagenum = 0;
    else
	pagenum++;
    char buf[100];
    sprintf(buf, "recoll://command/Page%d", pagenum);
    return buf;
}
string RecollKioPager::prevUrl()
{
    int pagenum = pageNumber();
    if (pagenum <= 0)
	pagenum = 0;
    else
	pagenum--;
    char buf[100];
    sprintf(buf, "recoll://command/Page%d", pagenum);
    return buf;
}

static RclConfig *rclconfig;
RclConfig *RclConfig::getMainConfig()
{
  return rclconfig;
}

RecollProtocol::RecollProtocol(const QByteArray &pool, const QByteArray &app) 
    : SlaveBase("recoll", pool, app), m_initok(false), 
      m_rclconfig(0), m_rcldb(0)
{
    m_pager.setParent(this);
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

static string welcomedata;

void RecollProtocol::welcomePage()
{
    kDebug() << endl;
    if (welcomedata.empty()) {
	QString location = 
	    KStandardDirs::locate("data", "kio_recoll/welcome.html");
	string reason;
	if (location.isEmpty() || 
	    !file_to_string((const char *)location.toUtf8(), 
			    welcomedata, &reason)) {
	    welcomedata = "<html><head><title>Recoll Error</title></head>"
		"<body><p>Could not locate Recoll welcome.html file: ";
	    welcomedata += reason;
	    welcomedata += "</p></body></html>";
	}
    }    
    string tmp;
    map<char, string> subs;
    subs['Q'] = "";
    pcSubst(welcomedata, tmp, subs);
    data(tmp.c_str());
    kDebug() << "WelcomePage done" << endl;
}

void RecollProtocol::doSearch(const QString& q, char opt)
{
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
	kDebug() << "Parsing query";
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
    RefCntr<Rcl::Query>query(new Rcl::Query(m_rcldb));
    if (!query->setQuery(sdata)) {
	m_reason = "Internal Error: setQuery failed";
	outputError(m_reason.c_str());
	finished();
	return;
    }
    DocSequenceDb *src = 
	new DocSequenceDb(RefCntr<Rcl::Query>(query), "Query results", 
			  sdata);

    m_pager.setDocSource(RefCntr<DocSequence>(src));
    m_pager.resultPageNext();
}

void RecollProtocol::get(const KUrl & url)
{
    kDebug() << url << endl;

    mimeType("text/html");

    if (!m_initok || !maybeOpenDb(m_reason)) {
	outputError(m_reason.c_str());
	finished();
	return;
    }

    QString host = url.host();
    QString path = url.path();
    kDebug() << "host:" << host << " path:" << path;
    if (host.isEmpty() || !host.compare("welcome")) {
	kDebug() << "Host is empty";
	if (path.isEmpty() || !path.compare("/")) {
	    kDebug() << "Path is empty or strange";
	    // Display welcome page
	    welcomePage();
	    finished();
	    return;
	}
	// Ie: "recoll: some search string"
	doSearch(path);
    } else if (!host.compare("search")) {
	if (path.compare("/query")) {
	    finished(); return;
	}
	// Decode the forms' arguments
	QString query = url.queryItem("q");
	if (query.isEmpty()) {
	    finished(); return;
	}
	QString opt = url.queryItem("qtp");
	if (opt.isEmpty()) {
	    opt = "l";
	}
	doSearch(query, opt.toUtf8().at(0));
    } else if (!host.compare("command")) {
	if (path.isEmpty()) {
	    finished();return;
	} else if (path.indexOf("/Page") == 0) {
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
	} else if (path.indexOf("/QueryDetails") == 0) {
	    QByteArray array;
	    QTextStream os(&array, QIODevice::WriteOnly);

	    os << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << endl;
	    os << "<title>" << "Recoll query details" << "</title>\n" << endl;
	    os << "</head>" << endl;
	    os << "<body><h3>Query details:</h3>" << endl;
	    os << "<p>" << m_pager.queryDescription().c_str() <<"</p>"<< endl;
	    os << "<p><a href=\"recoll://command/Page" << 
		m_pager.pageNumber() << "\">Return to results</a>" << endl;
	    os << "</body></html>" << endl;
	    data(array);
	    finished();
	    return;
	} else {
	    // Unknown //command/???
	    finished(); return;
	}
    } else {
	// Unknown 'host' //??? value
	finished(); return;
    }

    m_pager.displayPage();

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

