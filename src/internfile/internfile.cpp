#ifndef lint
static char rcsid[] = "@(#$Id: internfile.cpp,v 1.20 2006-12-15 16:33:15 dockes Exp $ (C) 2004 J.F.Dockes";
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

#ifndef TEST_INTERNFILE

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <string>
#include <iostream>
#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

#include "internfile.h"
#include "rcldoc.h"
#include "mimetype.h"
#include "debuglog.h"
#include "mimehandler.h"
#include "execmd.h"
#include "pathut.h"
#include "wipedir.h"
#include "rclconfig.h"

// Execute the command to uncompress a file into a temporary one.
static bool uncompressfile(RclConfig *conf, const string& ifn, 
			   const list<string>& cmdv, const string& tdir, 
			   string& tfile)
{
    // Make sure tmp dir is empty. we guarantee this to filters
    if (wipedir(tdir) != 0) {
	LOGERR(("uncompressfile: can't clear temp dir %s\n", tdir.c_str()));
	return false;
    }
    string cmd = cmdv.front();

    // Substitute file name and temp dir in command elements
    list<string>::const_iterator it = cmdv.begin();
    ++it;
    list<string> args;
    for (; it != cmdv.end(); it++) {
	string s = *it;
	string ns;
	string::const_iterator it1;
	for (it1 = s.begin(); it1 != s.end();it1++) {
	    if (*it1 == '%') {
		if (++it1 == s.end()) {
		    ns += '%';
		    break;
		}
		if (*it1 == '%')
		    ns += '%';
		if (*it1 == 'f')
		    ns += ifn;
		if (*it1 == 't')
		    ns += tdir;
	    } else {
		ns += *it1;
	    }
	}
	args.push_back(ns);
    }

    // Execute command and retrieve output file name, check that it exists
    ExecCmd ex;
    int status = ex.doexec(cmd, args, 0, &tfile);
    if (status) {
	LOGERR(("uncompressfile: doexec: status 0x%x\n", status));
	rmdir(tdir.c_str());
	return false;
    }
    if (!tfile.empty() && tfile[tfile.length() - 1] == '\n')
	tfile.erase(tfile.length() - 1, 1);
    return true;
}

void FileInterner::tmpcleanup()
{
    if (m_tdir.empty() || m_tfile.empty())
	return;
    if (unlink(m_tfile.c_str()) < 0) {
	LOGERR(("FileInterner::tmpcleanup: unlink(%s) errno %d\n", 
		m_tfile.c_str(), errno));
	return;
    }
}

// Handler==0 on return says we're in error, will be handled when calling
// internfile
FileInterner::FileInterner(const std::string &f, RclConfig *cnf, 
			   const string& td, const string *imime)
    : m_cfg(cnf), m_fn(f), m_forPreview(imime?true:false), m_tdir(td)
{
    bool usfci = false;
    cnf->getConfParam("usesystemfilecommand", &usfci);
    LOGDEB1(("FileInterner::FileInterner: usfci now %d\n", usfci));

    // We need to run mime type identification in any case to check
    // for a compressed file.
    string l_mime = mimetype(m_fn, m_cfg, usfci);

    // If identification fails, try to use the input parameter. This
    // is then normally not a compressed type (it's the mime type from
    // the db), and is only set when previewing, not for indexing
    if (l_mime.empty() && imime)
	l_mime = *imime;

    if (!l_mime.empty()) {
	// Has mime: check for a compressed file. If so, create a
	// temporary uncompressed file, and rerun the mime type
	// identification, then do the rest with the temp file.
	list<string>ucmd;
	if (m_cfg->getUncompressor(l_mime, ucmd)) {
	    if (!uncompressfile(m_cfg, m_fn, ucmd, m_tdir, m_tfile)) {
		return;
	    }
	    LOGDEB(("internfile: after ucomp: m_tdir %s, tfile %s\n", 
		    m_tdir.c_str(), m_tfile.c_str()));
	    m_fn = m_tfile;
	    l_mime = mimetype(m_fn, m_cfg, usfci);
	    if (l_mime.empty() && imime)
		l_mime = *imime;
	}
    }

    if (l_mime.empty()) {
	// No mime type. We let it through as config may warrant that
	// we index all file names
	LOGDEB(("internfile: (no mime) [%s]\n", m_fn.c_str()));
    }

    // Look for appropriate handler (might still return empty)
    m_mimetype = l_mime;
    Dijon::Filter *df = getMimeHandler(l_mime, m_cfg);

    if (!df) {
	// No handler for this type, for now :( if indexallfilenames
	// is set in the config, this normally wont happen (we get mh_unknown)
	LOGDEB(("FileInterner:: no handler for %s\n", l_mime.c_str()));
	return;
    }
    df->set_property(Dijon::Filter::OPERATING_MODE, 
			    m_forPreview ? "view" : "index");

    string charset = m_cfg->getDefCharset();
    df->set_property(Dijon::Filter::DEFAULT_CHARSET, charset);
    if (!df->set_document_file(m_fn)) {
	LOGERR(("FileInterner:: error parsing %s\n", m_fn.c_str()));
	return;
    }
    m_handlers.reserve(20);
    m_handlers.push_back(df);
    LOGDEB(("FileInterner::FileInterner: %s [%s]\n", l_mime.c_str(), 
	    m_fn.c_str()));
}

FileInterner::~FileInterner()
{
    while (!m_handlers.empty()) {
	delete m_handlers.back();
	m_handlers.pop_back(); 
    }
    tmpcleanup();
}

static const string string_empty;
static const string get_mimetype(Dijon::Filter* df)
{
    const std::map<std::string, std::string> *docdata = &df->get_meta_data();
    map<string,string>::const_iterator it;
    it = docdata->find("mimetype");
    if (it != docdata->end()) {
	return it->second;
    } else {
	return string_empty;
    }
}

bool FileInterner::dijontorcl(Rcl::Doc& doc)
{
    Dijon::Filter *df = m_handlers.back();
    const std::map<std::string, std::string> *docdata = &df->get_meta_data();
    map<string,string>::const_iterator it;

    it = docdata->find("origcharset");
    if (it != docdata->end())
	doc.origcharset = it->second;

    it = docdata->find("content");
    if (it != docdata->end())
	doc.text = it->second;

    it = docdata->find("title");
    if (it != docdata->end())
	doc.title = it->second;
 
    it = docdata->find("keywords");
    if (it != docdata->end())
	doc.keywords = it->second;

    it = docdata->find("modificationdate");
    if (it != docdata->end())
	doc.dmtime = it->second;

    it = docdata->find("abstract");
    if (it != docdata->end()) {
	doc.abstract = it->second;
    } else {
	it = docdata->find("sample");
	if (it != docdata->end()) 
	    doc.abstract = it->second;
    }
    return true;
}


static const unsigned int MAXHANDLERS = 20;

FileInterner::Status FileInterner::internfile(Rcl::Doc& doc, string& ipath)
{
    if (m_handlers.size() != 1) {
	LOGERR(("FileInterner::internfile: bad stack size %d !!\n", 
		m_handlers.size()));
	return FIError;
    }

    // Ipath vector.
    // Note that the vector is big enough for the maximum stack. All values
    // over the last significant one are ""
    // We set the ipath for the first handler here, others are set
    // when they're pushed on the stack
    vector<string> vipath(MAXHANDLERS);
    int vipathidx = 0;
    if (!ipath.empty()) {
	list<string> lipath;
	stringToTokens(ipath, lipath, "|", true);
	vipath.insert(vipath.begin(), lipath.begin(), lipath.end());
	if (!m_handlers.back()->skip_to_document(vipath[m_handlers.size()-1])){
	    LOGERR(("FileInterner::internfile: can't skip\n"));
	    return FIError;
	}
    }

    /* Try to get doc from the topmost filter */
    while (!m_handlers.empty()) {
	if (!m_handlers.back()->has_documents()) {
	    // No docs at the current top level. Pop and see if there
	    // is something at the previous one
	    delete m_handlers.back();
	    m_handlers.pop_back();
	    continue;
	}

	if (!m_handlers.back()->next_document()) {
	    LOGERR(("FileInterner::internfile: next_document failed\n"));
	    return FIError;
	}

	// Look at what we've got
	const std::map<std::string, std::string> *docdata = 
	    &m_handlers.back()->get_meta_data();
	map<string,string>::const_iterator it;
	string charset;
	it = docdata->find("charset");
	if (it != docdata->end())
	    charset = it->second;
	string mimetype;
	it = docdata->find("mimetype");
	if (it != docdata->end())
	    mimetype = it->second;

	LOGDEB(("FileInterner::internfile:next_doc is %s\n",mimetype.c_str()));
	// If we find a text/plain doc, we're done
	if (!strcmp(mimetype.c_str(), "text/plain"))
	    break;

	// Got a non text/plain doc. We need to stack another
	// filter. Check current size
	if (m_handlers.size() > MAXHANDLERS) {
	    // Stack too big. Skip this and go on to check if there is
	    // something else in the current back()
	    LOGDEB(("FileInterner::internfile: stack too high\n"));
	    continue;
	}

	Dijon::Filter *again = getMimeHandler(mimetype, m_cfg);
	if (!again) {
	    // If we can't find a filter, this doc can't be handled
	    // but there can be other ones so we go on
	    LOGERR(("FileInterner::internfile: no filter for [%s]\n",
		    mimetype.c_str()));
	    continue;
	}
	again->set_property(Dijon::Filter::OPERATING_MODE, 
			    m_forPreview ? "view" : "index");
	again->set_property(Dijon::Filter::DEFAULT_CHARSET, 
			    charset);
	string ns;
	const string *txt = &ns;
	it = docdata->find("content");
	if (it != docdata->end())
	    txt = &it->second;
	if (!again->set_document_string(*txt)) {
	    LOGERR(("FileInterner::internfile: error reparsing for %s\n", 
		    m_fn.c_str()));
	    delete again;
	    continue;
	}
	// add filter and go on
	m_handlers.push_back(again);
	if (!m_handlers.back()->skip_to_document(vipath[m_handlers.size()-1])){
	    LOGERR(("FileInterner::internfile: can't skip\n"));
	    return FIError;
	}
    }

    if (m_handlers.empty()) {
	LOGERR(("FileInterner::internfile: stack empty\n"));
	return FIError;
    }

    // If indexing, we have to collect the ipath stack. 

    // While we're at it, we also set the mimetype, which is a special 
    // property:we want to get it from the topmost doc
    // with an ipath, not the last one which is always text/html
    // Note that ipath is returned through the parameter not doc.ipath
    if (!m_forPreview) {
	bool hasipath = false;
	doc.mimetype = m_mimetype;
	LOGDEB2(("INITIAL mimetype: %s\n", doc.mimetype.c_str()));
	map<string,string>::const_iterator titi;

	for (vector<Dijon::Filter*>::const_iterator hit = m_handlers.begin();
	     hit != m_handlers.end(); hit++) {

	    const map<string, string>& docdata = (*hit)->get_meta_data();
	    map<string, string>::const_iterator iti = docdata.find("ipath");

	    if (iti != docdata.end()) {
		if (!iti->second.empty()) {
		    // We have a non-empty ipath
		    hasipath = true;
		    titi = docdata.find("mimetype");
		    if (titi != docdata.end())
			doc.mimetype = titi->second;
		}
		ipath += iti->second + "|";
	    } else {
		ipath += "|";
	    }
	}

	// Walk done, transform the list into a string
	if (hasipath) {
	    LOGDEB2(("IPATH [%s]\n", ipath.c_str()));
	    string::size_type sit = ipath.find_last_not_of("|");
	    if (sit == string::npos)
		ipath.erase();
	    else if (sit < ipath.length() -1)
		ipath.erase(sit+1);
	} else {
	    ipath.erase();
	}
    }

    dijontorcl(doc);

    // Destack what can be
    while (!m_handlers.empty() && !m_handlers.back()->has_documents()) {
	delete m_handlers.back();
	m_handlers.pop_back();
    }
    if (m_handlers.empty() || !m_handlers.back()->has_documents())
	return FIDone;
    else 
	return FIAgain;
}

#else

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
using namespace std;

#include "debuglog.h"
#include "rclinit.h"
#include "internfile.h"
#include "rclconfig.h"
#include "rcldoc.h"

static string thisprog;

static string usage =
    " internfile <filename> [ipath]\n"
    "  \n\n"
    ;

static void
Usage(void)
{
    cerr << thisprog  << ": usage:\n" << usage;
    exit(1);
}

static int        op_flags;
#define OPT_q	  0x1 

int main(int argc, char **argv)
{
    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
	(*argv)++;
	if (!(**argv))
	    /* Cas du "adb - core" */
	    Usage();
	while (**argv)
	    switch (*(*argv)++) {
	    default: Usage();	break;
	    }
	argc--; argv++;
    }
    DebugLog::getdbl()->setloglevel(DEBDEB1);
    DebugLog::setfilename("stderr");

    if (argc < 1)
	Usage();
    string fn(*argv++);
    argc--;
    string ipath;
    if (argc >= 1) {
	ipath.append(*argv++);
	argc--;
    }
    string reason;
    RclConfig *config = recollinit(0, 0, reason);

    if (config == 0 || !config->ok()) {
	string str = "Configuration problem: ";
	str += reason;
	fprintf(stderr, "%s\n", str.c_str());
	exit(1);
    }

    FileInterner interner(fn, config, "/tmp");
    Rcl::Doc doc;
    FileInterner::Status status = interner.internfile(doc, ipath);
    switch (status) {
    case FileInterner::FIDone:
    case FileInterner::FIAgain:
	break;
    case FileInterner::FIError:
    default:
	fprintf(stderr, "internfile failed\n");
	exit(1);
    }
    
    cout << "doc.url [[[[" << doc.url << 
	"]]]]\n-----------------------------------------------------\n" <<
	"doc.ipath [[[[" << doc.ipath <<
	"]]]]\n-----------------------------------------------------\n" <<
	"doc.mimetype [[[[" << doc.mimetype <<
	"]]]]\n-----------------------------------------------------\n" <<
	"doc.fmtime [[[[" << doc.fmtime <<
	"]]]]\n-----------------------------------------------------\n" <<
	"doc.dmtime [[[[" << doc.dmtime <<
	"]]]]\n-----------------------------------------------------\n" <<
	"doc.origcharset [[[[" << doc.origcharset <<
	"]]]]\n-----------------------------------------------------\n" <<
	"doc.title [[[[" << doc.title <<
	"]]]]\n-----------------------------------------------------\n" <<
	"doc.keywords [[[[" << doc.keywords <<
	"]]]]\n-----------------------------------------------------\n" <<
	"doc.abstract [[[[" << doc.abstract <<
	"]]]]\n-----------------------------------------------------\n" <<
	"doc.text [[[[" << doc.text << "]]]]\n";
}

#endif // TEST_INTERNFILE
