/* htmlparse.cc: simple HTML parser for omega indexer
 *
 * ----START-LICENCE----
 * Copyright 1999,2000,2001 BrightStation PLC
 * Copyright 2001 Ananova Ltd
 * Copyright 2002 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 * -----END-LICENCE-----
 */

// This file has code from omindex + an adaptor function for recoll at the end

#include "htmlparse.h"
#include "mimehandler.h"
#include "debuglog.h"
#include "csguess.h"
#include "readfile.h"
#include "transcode.h"
#include "mimeparse.h"

class MyHtmlParser : public HtmlParser {
 public:
    bool in_script_tag;
    bool in_style_tag;
    string title, sample, keywords, dump;
    string ocharset; // This is the charset our user thinks the doc was
    string charset; // This is the charset it was supposedly converted to
    string doccharset; // Set this to value of charset parameter in header
    bool indexing_allowed;
    void process_text(const string &text);
    void opening_tag(const string &tag, const map<string,string> &p);
    void closing_tag(const string &tag);
    MyHtmlParser() :
	in_script_tag(false),
	in_style_tag(false),
	indexing_allowed(true) { }
};

void
MyHtmlParser::process_text(const string &text)
{
    // some tags are meaningful mid-word so this is simplistic at best...

    if (!in_script_tag && !in_style_tag) {
	string::size_type firstchar = text.find_first_not_of(" \t\n\r");
	if (firstchar != string::npos) {
	    dump += text.substr(firstchar);
	    dump += " ";
	}
    }
}

// lets hope that the charset includes ascii values...
static inline void
lowercase_term(string &term)
{
    string::iterator i = term.begin();
    while (i != term.end()) {
	if (*i >= 'A' && *i <= 'Z')
	    *i = *i + 'a' - 'A';
        i++;
    }
}

#include <iostream>
using namespace std;


void
MyHtmlParser::opening_tag(const string &tag, const map<string,string> &p)
{
#if 0
    cout << "TAG: " << tag << ": " << endl;
    map<string, string>::const_iterator x;
    for (x = p.begin(); x != p.end(); x++) {
	cout << "  " << x->first << " -> '" << x->second << "'" << endl;
    }
#endif
    
    if (tag == "meta") {
	map<string, string>::const_iterator i, j;
	if ((i = p.find("content")) != p.end()) {
	    if ((j = p.find("name")) != p.end()) {
		string name = j->second;
		lowercase_term(name);
		if (name == "description") {
		    if (sample.empty()) {
			sample = i->second;
			decode_entities(sample);
		    }
		} else if (name == "keywords") {
		    if (!keywords.empty()) keywords += ' ';
		    string tmp = i->second;
		    decode_entities(tmp);
		    keywords += tmp;
		} else if (name == "robots") {
		    string val = i->second;
		    decode_entities(val);
		    lowercase_term(val);
		    if (val.find("none") != string::npos ||
			val.find("noindex") != string::npos) {
			indexing_allowed = false;
			throw true;
		    }
		}
	    } else if ((j = p.find("http-equiv")) != p.end()) {
		string hequiv = j->second;
		lowercase_term(hequiv);
		if (hequiv == "content-type") {
		    string value = i->second;
		    MimeHeaderValue p = parseMimeHeaderValue(value);
		    map<string, string>::const_iterator k;
		    if ((k = p.params.find("charset")) != p.params.end()) {
			doccharset = k->second;
			if (doccharset != ocharset) {
			    LOGDEB1(("Doc specified charset '%s' "
				     "differs from announced '%s'\n",
				     doccharset.c_str(), ocharset.c_str()));
			    throw true;
			}
		    }
		}
	    }
	}
    } else if (tag == "p" || tag == "br") {
	dump += "\n";
    } else if (tag == "script") {
	in_script_tag = true;
    } else if (tag == "style") {
	in_style_tag = true;
    } else if (tag == "body") {
	dump = "";
    }
}

void
MyHtmlParser::closing_tag(const string &tag)
{
    if (tag == "title") {
	title = dump;
	dump = "";
    } else if (tag == "script") {
	in_script_tag = false;
    } else if (tag == "style") {
	in_style_tag = false;
    } else if (tag == "body") {
	throw true;
    }
}

bool textHtmlToDoc(RclConfig *conf, const string &fn, 
			 const string &mtype, Rcl::Doc &docout)
{
    LOGDEB(("textHtmlToDoc: %s\n", fn.c_str()));
    string otext;
    if (!file_to_string(fn, otext)) {
	LOGINFO(("textHtmlToDoc: cant read: %s\n", fn.c_str()));
	return false;
    }
    
    // Character set handling:

    // - We first try to convert from the default configured charset
    //   (which may depend of the current directory) to utf-8. If this
    //   fails, we keep the original text
    // - During parsing, if we find a charset parameter, and it differs from
    //   what we started with, we abort and restart with the parameter value
    //   instead of the configuration one.
    string charset;
    if (conf->guesscharset) {
	charset = csguess(otext, conf->defcharset);
    } else
	charset = conf->defcharset;

    LOGDEB(("textHtmlToDoc: charset before parsing: %s\n", charset.c_str()));

    MyHtmlParser pres;
    for (int pass = 0; pass < 2; pass++) {
	string transcoded;

	MyHtmlParser p;
	// Try transcoding. If it fails, use original text.
	if (!transcode(otext, transcoded, charset, "UTF-8")) {
	    LOGERR(("textHtmlToDoc: transcode failed from cs '%s' to UTF-8\n",
		    charset.c_str()));
	    transcoded = otext;
	    // We don't know the charset, at all
	    p.ocharset = p.charset = charset = "";
	} else {
	    // ocharset has the putative source charset, transcoded is now
	    // in utf-8
	    p.ocharset = charset;
	    p.charset = "utf-8";
	}

	try {
	    p.parse_html(transcoded);
	} catch (bool) {
	    pres = p;
	    if (!pres.doccharset.empty() && 
		pres.doccharset != pres.ocharset) {
		LOGDEB(("textHtmlToDoc: charset '%s' doc charset '%s',"
			"reparse\n", charset.c_str(),pres.doccharset.c_str()));
		charset = pres.doccharset;
	    } else
		break;
	}
    }

    Rcl::Doc out;
    out.origcharset = charset;
    out.text = pres.dump;
    //    LOGDEB(("textHtmlToDoc: dump : %s\n", pres.dump.c_str()));
    out.title = pres.title;
    out.keywords = pres.keywords;
    out.abstract = pres.sample;
    docout = out;
    return true;
}
