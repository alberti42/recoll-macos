/* myhtmlparse.cc: subclass of HtmlParser for extracting text
 *
 * ----START-LICENCE----
 * Copyright 1999,2000,2001 BrightStation PLC
 * Copyright 2002,2003,2004 Olly Betts
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

#include "myhtmlparse.h"

#include "indextext.h" // for lowercase_term()

#include "mimeparse.h"

// The original version for this compresses whitespace and suppresses newlines
// I can see no good reason to do this, and it actually helps preview to keep
// whitespace, especially if the html comes from a filter that generated it 
// from text (ie: inside '<pre> tags)
void
MyHtmlParser::process_text(const string &text)
{
    if (!in_script_tag && !in_style_tag) {
#if 0
	string::size_type b = 0;
	while ((b = text.find_first_not_of(WHITESPACE, b)) != string::npos) {
	    if (pending_space || b != 0)
		if (!dump.empty()) dump += ' ';
	    pending_space = true;
	    string::size_type e = text.find_first_of(WHITESPACE, b);
	    if (e == string::npos) {
		dump += text.substr(b);
		pending_space = false;
		break;
	    }
	    dump += text.substr(b, e - b);
	    b = e + 1;
	}
#else
	if (pending_space)
	    dump += ' ';
	dump += text;
#endif
    }
}

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
    if (tag.empty()) return;
    switch (tag[0]) {
	case 'a':
	    if (tag == "address") pending_space = true;
	    break;
	case 'b':
	    if (tag == "body") {
		dump = "";
		break;
	    }
	    if (tag == "blockquote" || tag == "br") {
		dump += '\n';
		pending_space = true;
	    }
	    break;
	case 'c':
	    if (tag == "center") pending_space = true;
	    break;
	case 'd':
	    if (tag == "dd" || tag == "dir" || tag == "div" || tag == "dl" ||
		tag == "dt") pending_space = true;
	    if (tag == "dt")
		dump += '\n';
	    break;
	case 'e':
	    if (tag == "embed") pending_space = true;
	    break;
	case 'f':
	    if (tag == "fieldset" || tag == "form") pending_space = true;
	    break;
	case 'h':
	    // hr, and h1, ..., h6
	    if (tag.length() == 2 && strchr("r123456", tag[1])) {
		dump += '\n';
		pending_space = true;
	    }
	    break;
	case 'i':
	    if (tag == "iframe" || tag == "img" || tag == "isindex" ||
		tag == "input") pending_space = true;
	    break;
	case 'k':
	    if (tag == "keygen") pending_space = true;
	    break;
	case 'l':
	    if (tag == "legend" || tag == "li" || tag == "listing") {
		dump += '\n';
		pending_space = true;
	    }
	    break;
	case 'm':
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
			    MimeHeaderValue p;
			    parseMimeHeaderValue(value, p);
			    map<string, string>::const_iterator k;
			    if ((k = p.params.find("charset")) != 
				p.params.end()) {
				doccharset = k->second;
				if (doccharset != ocharset) {
				    LOGDEB1(("Doc specified charset '%s' "
					     "differs from announced '%s'\n",
					     doccharset.c_str(), 
					     ocharset.c_str()));
				    throw true;
				}
			    }
			}
		    }
		}
		break;
	    }
	    if (tag == "marquee" || tag == "menu" || tag == "multicol")
		pending_space = true;
	    break;
	case 'o':
	    if (tag == "ol" || tag == "option") pending_space = true;
	    break;
	case 'p':
	    if (tag == "p" || tag == "pre" || tag == "plaintext") {
		dump += '\n';
		pending_space = true;
	    }
	    break;
	case 'q':
	    if (tag == "q") pending_space = true;
	    break;
	case 's':
	    if (tag == "style") {
		in_style_tag = true;
		break;
	    }
	    if (tag == "script") {
		in_script_tag = true;
		break;
	    }
	    if (tag == "select") pending_space = true;
	    break;
	case 't':
	    if (tag == "table" || tag == "td" || tag == "textarea" ||
		tag == "th") pending_space = true;
	    break;
	case 'u':
	    if (tag == "ul") pending_space = true;
	    break;
	case 'x':
	    if (tag == "xmp") pending_space = true;
	    break;
    }
}

void
MyHtmlParser::closing_tag(const string &tag)
{
    if (tag.empty()) return;
    switch (tag[0]) {
	case 'a':
	    if (tag == "address") pending_space = true;
	    break;
	case 'b':
	    if (tag == "body") {
		throw true;
	    }
	    if (tag == "blockquote" || tag == "br") pending_space = true;
	    break;
	case 'c':
	    if (tag == "center") pending_space = true;
	    break;
	case 'd':
	    if (tag == "dd" || tag == "dir" || tag == "div" || tag == "dl" ||
		tag == "dt") pending_space = true;
	    break;
	case 'f':
	    if (tag == "fieldset" || tag == "form") pending_space = true;
	    break;
	case 'h':
	    // hr, and h1, ..., h6
	    if (tag.length() == 2 && strchr("r123456", tag[1]))
		pending_space = true;
	    break;
	case 'i':
	    if (tag == "iframe") pending_space = true;
	    break;
	case 'l':
	    if (tag == "legend" || tag == "li" || tag == "listing")
		pending_space = true;
	    break;
	case 'm':
	    if (tag == "marquee" || tag == "menu") pending_space = true;
	    break;
	case 'o':
	    if (tag == "ol" || tag == "option") pending_space = true;
	    break;
	case 'p':
	    if (tag == "p" || tag == "pre") pending_space = true;
	    break;
	case 'q':
	    if (tag == "q") pending_space = true;
	    break;
	case 's':
	    if (tag == "style") {
		in_style_tag = false;
		break;
	    }
	    if (tag == "script") {
		in_script_tag = false;
		break;
	    }
	    if (tag == "select") pending_space = true;
	    break;
	case 't':
	    if (tag == "title") {
		if (title.empty()) {
		    title = dump;
		    dump = "";
		}
		break;
	    }
	    if (tag == "table" || tag == "td" || tag == "textarea" ||
		tag == "th") pending_space = true;
	    break;
	case 'u':
	    if (tag == "ul") pending_space = true;
	    break;
	case 'x':
	    if (tag == "xmp") pending_space = true;
	    break;
    }
}
