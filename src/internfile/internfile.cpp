#ifndef lint
static char rcsid[] = "@(#$Id: internfile.cpp,v 1.11 2005-11-24 07:16:15 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
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
#include "mimetype.h"
#include "debuglog.h"
#include "mimehandler.h"
#include "execmd.h"
#include "pathut.h"
#include "wipedir.h"

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
    string cmd = find_filter(conf, cmdv.front());

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
    : m_fn(f), m_cfg(cnf), m_tdir(td), m_handler(0)
{
    // We are actually going to access the file, so it's ok
    // performancewise to check this config variable at every call
    // even if it can only change when we change directories
    string usfc;
    int usfci;
    if (!cnf->getConfParam("usesystemfilecommand", usfc)) 
	usfci = 0;
    else 
	usfci = atoi(usfc.c_str()) ? 1 : 0;
    LOGDEB1(("FileInterner::FileInterner: usfci now %d\n", usfci));

    bool forPreview = imime ? true : false;

    // We need to run mime type identification in any case to check
    // for a compressed file.
    m_mime = mimetype(m_fn, m_cfg, usfci);

    // If identification fails, try to use the input parameter. Note that this 
    // is normally not a compressed type (it's the mime type from the db)
    if (m_mime.empty() && imime)
	m_mime = *imime;
    if (m_mime.empty()) {
	// No mime type: not listed in our map, or present in stop list
	LOGDEB(("FileInterner::FileInterner: (no mime) [%s]\n", m_fn.c_str()));
	return;
    }

    // First check for a compressed file. If so, create a temporary
    // uncompressed file, and rerun the mime type identification, then do the
    // rest with the temp file.
    list<string>ucmd;
    if (m_cfg->getUncompressor(m_mime, ucmd)) {
	if (!uncompressfile(m_cfg, m_fn, ucmd, m_tdir, m_tfile)) {
	    return;
	}
	LOGDEB(("internfile: after ucomp: m_tdir %s, tfile %s\n", 
		m_tdir.c_str(), m_tfile.c_str()));
	m_fn = m_tfile;
	m_mime = mimetype(m_fn, m_cfg, usfci);
	if (m_mime.empty() && imime)
	    m_mime = *imime;
	if (m_mime.empty()) {
	    // No mime type ?? pass on.
	    LOGDEB(("internfile: (no mime) [%s]\n", m_fn.c_str()));
	    return;
	}
    }

    // Look for appropriate handler
    m_handler = getMimeHandler(m_mime, m_cfg);
    if (!m_handler) {
	// No handler for this type, for now :(
	LOGDEB(("FileInterner::FileInterner: %s: no handler\n", 
		m_mime.c_str()));
	return;
    }
    m_handler->setForPreview(forPreview);
    LOGDEB(("FileInterner::FileInterner: %s [%s]\n", m_mime.c_str(), 
	    m_fn.c_str()));
}

FileInterner::Status FileInterner::internfile(Rcl::Doc& doc, string& ipath)
{
    if (!m_handler)
	return FIError;

    // Turn file into a document. The document has fields for title, body 
    // etc.,  all text converted to utf8
    MimeHandler::Status mhs = 
	m_handler->mkDoc(m_cfg, m_fn, m_mime, doc, ipath);
    FileInterner::Status ret = FIError;
    switch (mhs) {
    case MimeHandler::MHError: break;
    case MimeHandler::MHDone: ret = FIDone;break;
    case MimeHandler::MHAgain: ret = FIAgain;break;
    }

    doc.mimetype = m_mime;
    return ret;
}

FileInterner::~FileInterner()
{
    delete m_handler; 
    m_handler = 0;
    tmpcleanup();
}
