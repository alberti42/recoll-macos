#ifndef lint
static char rcsid[] = "@(#$Id: mimehandler.cpp,v 1.2 2005-01-26 11:47:27 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <iostream>
#include <string>
using namespace std;

#include "mimehandler.h"
#include "readfile.h"
#include "csguess.h"
#include "transcode.h"
#include "debuglog.h"

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
    LOGDEB1(("textPlainToDoc: transcod from %s to %s\n", charset, "UTF-8"));

    if (!transcode(otext, utf8, charset, "UTF-8")) {
	cerr << "textPlainToDoc: transcode failed: charset '" << charset
	     << "' to UTF-8: "<< utf8 << endl;
	otext.erase();
	return 0;
    }

    Rcl::Doc out;
    out.origcharset = charset;
    out.text = utf8;
    docout = out;
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
	ihandlers["text/html"] = textHtmlToDoc;
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
	LOGERR(("getMimeHandler: bad mimeconf line for %s\n", mtype.c_str()));
	return 0;
    }

    // Retrieve handler function according to type
    if (!strcasecmp(toks[0].c_str(), "internal")) {
	cerr << "Internal Handler" << endl;
	map<string, MimeHandlerFunc>::const_iterator it = 
	    ihandlers.find(mtype);
	if (it == ihandlers.end()) {
	    LOGERR(("getMimeHandler: internal handler not found for %s\n",
		    mtype.c_str()));
	    return 0;
	}
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
