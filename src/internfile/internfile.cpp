#ifndef lint
static char rcsid[] = "@(#$Id: internfile.cpp,v 1.2 2005-02-09 12:07:29 dockes Exp $ (C) 2004 J.F.Dockes";
#endif
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <string>
#include <iostream>
using namespace std;

#include "internfile.h"
#include "mimetype.h"
#include "debuglog.h"
#include "mimehandler.h"
#include "execmd.h"
#include "pathut.h"
#include "wipedir.h"

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

static void tmpcleanup(const string& tdir, const string& tfile)
{
    if (tdir.empty() || tfile.empty())
	return;
    if (unlink(tfile.c_str()) < 0) {
	LOGERR(("tmpcleanup: unlink(%s) errno %d\n", tfile.c_str(), 
		errno));
	return;
    }
}

bool internfile(const std::string &ifn, RclConfig *config, Rcl::Doc& doc,
		const string& tdir)
{
    string fn = ifn;
    string tfile;
    MimeHandler *handler = 0;
    bool ret = false;

    string mime = mimetype(fn, config->getMimeMap());
    if (mime.empty()) {
	// No mime type: not listed in our map.
	LOGDEB(("internfile: (no mime) [%s]\n", fn.c_str()));
	return false;
    }

    // First check for a compressed file
    list<string>ucmd;
    if (getUncompressor(mime, config->getMimeConf(), ucmd)) {
	if (!uncompressfile(config, fn, ucmd, tdir, tfile)) 
	    return false;
	LOGDEB(("internfile: after ucomp: tdir %s, tfile %s\n", 
		tdir.c_str(), tfile.c_str()));
	fn = tfile;
	mime = mimetype(fn, config->getMimeMap());
	if (mime.empty()) {
	    // No mime type ?? pass on.
	    LOGDEB(("internfile: (no mime) [%s]\n", fn.c_str()));
	    goto out;
	}

    }
    
    // Look for appropriate handler
    handler = getMimeHandler(mime, config->getMimeConf());
    if (!handler) {
	// No handler for this type, for now :(
	LOGDEB(("internfile: %s : no handler\n", mime.c_str()));
	goto out;
    }

    LOGDEB(("internfile: %s [%s]\n", mime.c_str(), fn.c_str()));

    // Turn file into a document. The document has fields for title, body 
    // etc.,  all text converted to utf8
    if (!handler->worker(config, fn,  mime, doc)) {
	goto out;
    }
    doc.mimetype = mime;

    // Clean up. We delete the temp file and its father directory
    ret = true;
 out:
    delete handler;
    tmpcleanup(tdir, tfile);
    return ret;
}
