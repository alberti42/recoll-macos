#ifndef lint
static char rcsid[] = "@(#$Id: recollindex.cpp,v 1.4 2004-12-17 13:01:01 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <sys/stat.h>

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
#include "csguess.h"
#include "transcode.h"

using namespace std;


bool textPlainToDoc(RclConfig *conf, const string &fn, 
			 const string &mtype, Rcl::Doc &docout)
{
    string otext;
    if (!file_to_string(fn, otext))
	return false;
	
    // Try to guess charset, then convert to utf-8, and fill document
    // fields The charset guesser really doesnt work well in general
    // and should be avoided (especially for short documents)
    string charset;
    if (conf->guesscharset) {
	charset = csguess(otext, conf->defcharset);
    } else
	charset = conf->defcharset;
    string utf8;
    cerr << "textPlainToDoc: transcod from " << charset << " to  UTF-8" 
	 << endl;

    if (!transcode(otext, utf8, charset, "UTF-8")) {
	cerr << "textPlainToDoc: transcode failed: charset '" << charset
	     << "' to UTF-8: "<< utf8 << endl;
	otext.erase();
	return 0;
    }

    Rcl::Doc out;
    out.origcharset = charset;
    out.text = utf8;
    //out.text = otext;
    docout = out;
    cerr << utf8 << endl;
    return true;
}

// Map of mime types to internal interner functions. This could just as well 
// be an if else if suite inside getMimeHandler(), but this is prettier ?
static map<string, MimeHandlerFunc> ihandlers;
// Static object to get the map to be initialized at program start.
class IHandler_Init {
 public:
    IHandler_Init() {
	ihandlers["text/plain"] = textPlainToDoc;
	// Add new associations here when needed
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

/**
 * Bunch holder for data used while indexing a directory tree
 */
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
    if (!db.open(dbdir, Rcl::Db::DbUpd)) {
	cerr << "Error opening database in " << dbdir << " for " <<
	    topdir << endl;
	return;
    }
    walker.walk(topdir, indexfile, this);
    if (!db.close()) {
	cerr << "Error closing database in " << dbdir << " for " <<
	    topdir << endl;
	return;
    }
}

/** 
 * This function gets called for every file and directory found by the
 * tree walker. Adjust parameters and index files if/when needed.
 */
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

    if (!me->db.needUpdate(fn, stp))
	return FsTreeWalker::FtwOk;

    // Turn file into a document. The document has fields for title, body 
    // etc.,  all text converted to utf8
    Rcl::Doc doc;
    if (!fun(me->config, fn,  mime, doc))
	return FsTreeWalker::FtwOk;

    // Set up common fields:
    doc.mimetype = mime;
    char ascdate[20];
    sprintf(ascdate, "%ld", long(stp->st_mtime));
    doc.mtime = ascdate;

    // Set up xapian document, add postings and misc fields, 
    // add to or update database.
    if (!me->db.add(fn, doc))
	return FsTreeWalker::FtwError;

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
	for (unsigned int i = 0; i < tdl.size(); i++) {
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
