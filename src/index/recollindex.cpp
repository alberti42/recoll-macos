#ifndef lint
static char rcsid[] = "@(#$Id: recollindex.cpp,v 1.2 2004-12-14 17:54:16 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <strings.h>

#include <iostream>

#include "pathut.h"
#include "conftree.h"
#include "rclconfig.h"
#include "fstreewalk.h"
#include "mimetype.h"
#include "rcldb.h"
#include "readfile.h"
#include "indexer.h"

using namespace std;


Rcl::Doc* textPlainToDoc(RclConfig *conf, const string &fn, 
			 const string &mtype)
{
    return 0;
}

static map<string, MimeHandlerFunc> ihandlers;
class IHandler_Init {
 public:
    IHandler_Init() {
	ihandlers["text/plain"] = textPlainToDoc;
    }
};
static IHandler_Init ihandleriniter;

/**
 * Return handler function for given mime type
 */
MimeHandlerFunc getMimeHandler(const std::string &mtype, ConfTree *mhandlers)
{
    // Return handler definition for mime type
    string hs;
    if (!mhandlers->get(mtype, hs, "")) 
	return 0;

    // Break definition into type and name 
    vector<string> toks;
    ConfTree::stringToStrings(hs, toks);
    if (toks.size() < 1) {
	cerr << "Bad mimeconf line for " << mtype << endl;
	return 0;
    }

    // Retrieve handler function according to type
    if (!strcasecmp(toks[0].c_str(), "internal")) {
	cerr << "Internal Handler" << endl;
	map<string, MimeHandlerFunc>::const_iterator it = 
	    ihandlers.find(mtype);
	if (it == ihandlers.end()) {
	    cerr << "Internal handler not found for " << mtype << endl;
	    return 0;
	}
	cerr << "Got handler" << endl;
	return it->second;
    } else if (!strcasecmp(toks[0].c_str(), "dll")) {
	if (toks.size() != 2)
	    return 0;
	return 0;
    } else if (!strcasecmp(toks[0].c_str(), "exec")) {
	if (toks.size() != 2)
	    return 0;
	return 0;
    } else {
	return 0;
    }
}

class DirIndexer {
    FsTreeWalker walker;
    RclConfig *config;
    string topdir;
    string dbdir;
    Rcl::Db db;
 public:
    DirIndexer(RclConfig *cnf, const string &dbd, const string &top) 
	: config(cnf), topdir(top), dbdir(dbd)
    { }

    friend FsTreeWalker::Status 
      indexfile(void *, const std::string &, const struct stat *, 
		FsTreeWalker::CbFlag);

    void index();
};

void DirIndexer::index()
{
#if 0
    if (!db.open(dbdir, Rcl::Db::DbUpd)) {
	cerr << "Error opening database in " << dbdir << " for " <<
	    topdir << endl;
	return;
    }
#endif
    walker.walk(topdir, indexfile, this);
#if 0
    if (!db.close()) {
	cerr << "Error closing database in " << dbdir << " for " <<
	    topdir << endl;
	return;
    }
#endif
}

FsTreeWalker::Status 
indexfile(void *cdata, const std::string &fn, const struct stat *stp, 
	  FsTreeWalker::CbFlag flg)
{
    DirIndexer *me = (DirIndexer *)cdata;

    if (flg == FsTreeWalker::FtwDirEnter || 
	flg == FsTreeWalker::FtwDirReturn) {
	me->config->setKeyDir(fn);
	cout << "indexfile: [" << fn << "]" << endl;
	cout << "   defcharset: " << me->config->getDefCharset()
	     << " deflang: " << me->config->getDefLang() << endl;

	return FsTreeWalker::FtwOk;
    }

    string mime = mimetype(fn, me->config->getMimeMap());
    if (mime.length() == 0) {
	cout << "indexfile: " << "(no mime)" << " " << fn << endl;
	// No mime type ?? pass on.
	return FsTreeWalker::FtwOk;
    }

    cout << "indexfile: " << mime << " " << fn << endl;

    // Look for appropriate handler
    MimeHandlerFunc fun = getMimeHandler(mime, me->config->getMimeConf());
    if (!fun) {
	// No handler for this type, for now :(
	return FsTreeWalker::FtwOk;
    }

    // Check if file has already been indexed, and has changed since
    // - Make path term, 
    // - query db: postlist_begin->docid
    // - fetch doc (get_document(docid)
    // - check date field, maybe skip

    // Turn file into a document. The document has fields for title, body 
    // etc.,  all text converted to utf8
    Rcl::Doc *doc = fun(me->config, fn,  mime);

#if 0
    // Set up xapian document, add postings and misc fields, 
    // add to or update database.
    dbadd(doc);
#endif

    return FsTreeWalker::FtwOk;
}


int main(int argc, const char **argv)
{
    RclConfig *config = new RclConfig;

    if (!config->ok())
	cerr << "Config could not be built" << endl;

    ConfTree *conf = config->getConfig();
    
    string topdirs;
    if (conf->get("topdirs", topdirs, "") == 0) {
	cerr << "No top directories in configuration" << endl;
	exit(1);
    }
    vector<string> tdl;
    if (ConfTree::stringToStrings(topdirs, tdl)) {
	for (int i = 0; i < tdl.size(); i++) {
	    string topdir = tdl[i];
	    cout << topdir << endl;
	    string dbdir;
	    if (conf->get("dbdir", dbdir, topdir) == 0) {
		cerr << "No database directory in configuration for " 
		     << topdir << endl;
		exit(1);
	    }
	    DirIndexer indexer(config, dbdir, topdir);
	    indexer.index();
	}
    }
}
