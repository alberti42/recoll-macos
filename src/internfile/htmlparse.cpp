/* This file was copied from omega-0.8.5 and modified */

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

#ifndef lint
static char rcsid[] = "@(#$Id: htmlparse.cpp,v 1.6 2006-01-30 11:15:27 dockes Exp $ ";
#endif

//#include <config.h>

#include <algorithm>
#ifndef NO_NAMESPACES
using namespace std;
//using std::find;
//using std::find_if;
#endif /* NO_NAMESPACES */
#include "htmlparse.h"
#include <stdio.h>
#include <ctype.h>

#include "transcode.h"

map<string, string> HtmlParser::named_ents;

inline static bool
p_alpha(char c)
{
    return isalpha(c);
}

inline static bool
p_notdigit(char c)
{
    return !isdigit(c);
}

inline static bool
p_notxdigit(char c)
{
    return !isxdigit(c);
}

inline static bool
p_notalnum(char c)
{
    return !isalnum(c);
}

inline static bool
p_notwhitespace(char c)
{
    return !isspace(c);
}

inline static bool
p_nottag(char c)
{
    return !isalnum(c) && c != '.' && c != '-';
}

inline static bool
p_whitespacegt(char c)
{
    return isspace(c) || c == '>';
}

inline static bool
p_whitespaceeqgt(char c)
{
    return isspace(c) || c == '=' || c == '>';
}

/*
 * The following array was taken from Estraier. Estraier was
 * written by Mikio Hirabayashi. 
 *                Copyright (C) 2003-2004 Mikio Hirabayashi
 * The version where this comes from 
 * is covered by the GNU licence, as this file.*/
static const char *epairs[] = {
    /* basic symbols */
    "amp", "&", "lt", "<", "gt", ">", "quot", "\"", "apos", "'",
    /* ISO-8859-1 */
    "nbsp", "\xc2\xa0", "iexcl", "\xc2\xa1", "cent", "\xc2\xa2",
    "pound", "\xc2\xa3", "curren", "\xc2\xa4", "yen", "\xc2\xa5",
    "brvbar", "\xc2\xa6", "sect", "\xc2\xa7", "uml", "\xc2\xa8",
    "copy", "\xc2\xa9", "ordf", "\xc2\xaa", "laquo", "\xc2\xab",
    "not", "\xc2\xac", "shy", "\xc2\xad", "reg", "\xc2\xae",
    "macr", "\xc2\xaf", "deg", "\xc2\xb0", "plusmn", "\xc2\xb1",
    "sup2", "\xc2\xb2", "sup3", "\xc2\xb3", "acute", "\xc2\xb4",
    "micro", "\xc2\xb5", "para", "\xc2\xb6", "middot", "\xc2\xb7",
    "cedil", "\xc2\xb8", "sup1", "\xc2\xb9", "ordm", "\xc2\xba",
    "raquo", "\xc2\xbb", "frac14", "\xc2\xbc", "frac12", "\xc2\xbd",
    "frac34", "\xc2\xbe", "iquest", "\xc2\xbf", "Agrave", "\xc3\x80",
    "Aacute", "\xc3\x81", "Acirc", "\xc3\x82", "Atilde", "\xc3\x83",
    "Auml", "\xc3\x84", "Aring", "\xc3\x85", "AElig", "\xc3\x86",
    "Ccedil", "\xc3\x87", "Egrave", "\xc3\x88", "Eacute", "\xc3\x89",
    "Ecirc", "\xc3\x8a", "Euml", "\xc3\x8b", "Igrave", "\xc3\x8c",
    "Iacute", "\xc3\x8d", "Icirc", "\xc3\x8e", "Iuml", "\xc3\x8f",
    "ETH", "\xc3\x90", "Ntilde", "\xc3\x91", "Ograve", "\xc3\x92",
    "Oacute", "\xc3\x93", "Ocirc", "\xc3\x94", "Otilde", "\xc3\x95",
    "Ouml", "\xc3\x96", "times", "\xc3\x97", "Oslash", "\xc3\x98",
    "Ugrave", "\xc3\x99", "Uacute", "\xc3\x9a", "Ucirc", "\xc3\x9b",
    "Uuml", "\xc3\x9c", "Yacute", "\xc3\x9d", "THORN", "\xc3\x9e",
    "szlig", "\xc3\x9f", "agrave", "\xc3\xa0", "aacute", "\xc3\xa1",
    "acirc", "\xc3\xa2", "atilde", "\xc3\xa3", "auml", "\xc3\xa4",
    "aring", "\xc3\xa5", "aelig", "\xc3\xa6", "ccedil", "\xc3\xa7",
    "egrave", "\xc3\xa8", "eacute", "\xc3\xa9", "ecirc", "\xc3\xaa",
    "euml", "\xc3\xab", "igrave", "\xc3\xac", "iacute", "\xc3\xad",
    "icirc", "\xc3\xae", "iuml", "\xc3\xaf", "eth", "\xc3\xb0",
    "ntilde", "\xc3\xb1", "ograve", "\xc3\xb2", "oacute", "\xc3\xb3",
    "ocirc", "\xc3\xb4", "otilde", "\xc3\xb5", "ouml", "\xc3\xb6",
    "divide", "\xc3\xb7", "oslash", "\xc3\xb8", "ugrave", "\xc3\xb9",
    "uacute", "\xc3\xba", "ucirc", "\xc3\xbb", "uuml", "\xc3\xbc",
    "yacute", "\xc3\xbd", "thorn", "\xc3\xbe", "yuml", "\xc3\xbf",
    /* ISO-10646 */
    "fnof", "\xc6\x92", "Alpha", "\xce\x91", "Beta", "\xce\x92",
    "Gamma", "\xce\x93", "Delta", "\xce\x94", "Epsilon", "\xce\x95",
    "Zeta", "\xce\x96", "Eta", "\xce\x97", "Theta", "\xce\x98",
    "Iota", "\xce\x99", "Kappa", "\xce\x9a", "Lambda", "\xce\x9b",
    "Mu", "\xce\x9c", "Nu", "\xce\x9d", "Xi", "\xce\x9e",
    "Omicron", "\xce\x9f", "Pi", "\xce\xa0", "Rho", "\xce\xa1",
    "Sigma", "\xce\xa3", "Tau", "\xce\xa4", "Upsilon", "\xce\xa5",
    "Phi", "\xce\xa6", "Chi", "\xce\xa7", "Psi", "\xce\xa8",
    "Omega", "\xce\xa9", "alpha", "\xce\xb1", "beta", "\xce\xb2",
    "gamma", "\xce\xb3", "delta", "\xce\xb4", "epsilon", "\xce\xb5",
    "zeta", "\xce\xb6", "eta", "\xce\xb7", "theta", "\xce\xb8",
    "iota", "\xce\xb9", "kappa", "\xce\xba", "lambda", "\xce\xbb",
    "mu", "\xce\xbc", "nu", "\xce\xbd", "xi", "\xce\xbe",
    "omicron", "\xce\xbf", "pi", "\xcf\x80", "rho", "\xcf\x81",
    "sigmaf", "\xcf\x82", "sigma", "\xcf\x83", "tau", "\xcf\x84",
    "upsilon", "\xcf\x85", "phi", "\xcf\x86", "chi", "\xcf\x87",
    "psi", "\xcf\x88", "omega", "\xcf\x89", "thetasym", "\xcf\x91",
    "upsih", "\xcf\x92", "piv", "\xcf\x96", "bull", "\xe2\x80\xa2",
    "hellip", "\xe2\x80\xa6", "prime", "\xe2\x80\xb2", "Prime", "\xe2\x80\xb3",
    "oline", "\xe2\x80\xbe", "frasl", "\xe2\x81\x84", "weierp", "\xe2\x84\x98",
    "image", "\xe2\x84\x91", "real", "\xe2\x84\x9c", "trade", "\xe2\x84\xa2",
    "alefsym", "\xe2\x84\xb5", "larr", "\xe2\x86\x90", "uarr", "\xe2\x86\x91",
    "rarr", "\xe2\x86\x92", "darr", "\xe2\x86\x93", "harr", "\xe2\x86\x94",
    "crarr", "\xe2\x86\xb5", "lArr", "\xe2\x87\x90", "uArr", "\xe2\x87\x91",
    "rArr", "\xe2\x87\x92", "dArr", "\xe2\x87\x93", "hArr", "\xe2\x87\x94",
    "forall", "\xe2\x88\x80", "part", "\xe2\x88\x82", "exist", "\xe2\x88\x83",
    "empty", "\xe2\x88\x85", "nabla", "\xe2\x88\x87", "isin", "\xe2\x88\x88",
    "notin", "\xe2\x88\x89", "ni", "\xe2\x88\x8b", "prod", "\xe2\x88\x8f",
    "sum", "\xe2\x88\x91", "minus", "\xe2\x88\x92", "lowast", "\xe2\x88\x97",
    "radic", "\xe2\x88\x9a", "prop", "\xe2\x88\x9d", "infin", "\xe2\x88\x9e",
    "ang", "\xe2\x88\xa0", "and", "\xe2\x88\xa7", "or", "\xe2\x88\xa8",
    "cap", "\xe2\x88\xa9", "cup", "\xe2\x88\xaa", "int", "\xe2\x88\xab",
    "there4", "\xe2\x88\xb4", "sim", "\xe2\x88\xbc", "cong", "\xe2\x89\x85",
    "asymp", "\xe2\x89\x88", "ne", "\xe2\x89\xa0", "equiv", "\xe2\x89\xa1",
    "le", "\xe2\x89\xa4", "ge", "\xe2\x89\xa5", "sub", "\xe2\x8a\x82",
    "sup", "\xe2\x8a\x83", "nsub", "\xe2\x8a\x84", "sube", "\xe2\x8a\x86",
    "supe", "\xe2\x8a\x87", "oplus", "\xe2\x8a\x95", "otimes", "\xe2\x8a\x97",
    "perp", "\xe2\x8a\xa5", "sdot", "\xe2\x8b\x85", "lceil", "\xe2\x8c\x88",
    "rceil", "\xe2\x8c\x89", "lfloor", "\xe2\x8c\x8a", "rfloor", "\xe2\x8c\x8b",
    "lang", "\xe2\x8c\xa9", "rang", "\xe2\x8c\xaa", "loz", "\xe2\x97\x8a",
    "spades", "\xe2\x99\xa0", "clubs", "\xe2\x99\xa3", "hearts", "\xe2\x99\xa5",
    "diams", "\xe2\x99\xa6", "OElig", "\xc5\x92", "oelig", "\xc5\x93",
    "Scaron", "\xc5\xa0", "scaron", "\xc5\xa1", "Yuml", "\xc5\xb8",
    "circ", "\xcb\x86", "tilde", "\xcb\x9c", "ensp", "\xe2\x80\x82",
    "emsp", "\xe2\x80\x83", "thinsp", "\xe2\x80\x89", "zwnj", "\xe2\x80\x8c",
    "zwj", "\xe2\x80\x8d", "lrm", "\xe2\x80\x8e", "rlm", "\xe2\x80\x8f",
    "ndash", "\xe2\x80\x93", "mdash", "\xe2\x80\x94", "lsquo", "\xe2\x80\x98",
    "rsquo", "\xe2\x80\x99", "sbquo", "\xe2\x80\x9a", "ldquo", "\xe2\x80\x9c",
    "rdquo", "\xe2\x80\x9d", "bdquo", "\xe2\x80\x9e", "dagger", "\xe2\x80\xa0",
    "Dagger", "\xe2\x80\xa1", "permil", "\xe2\x80\xb0", "lsaquo", "\xe2\x80\xb9",
    "rsaquo", "\xe2\x80\xba", "euro", "\xe2\x82\xac",
    NULL, NULL
};

HtmlParser::HtmlParser()
{
    if (named_ents.empty()) {
	for (int i = 0;;) {
	    const char *ent;
	    const char *val;
	    ent = epairs[i++];
	    if (ent == 0) 
		break;
	    val = epairs[i++];
	    if (val == 0) 
		break;
	    named_ents[string(ent)] = val;
	}
    }
}

void
HtmlParser::decode_entities(string &s)
{
    // This has no meaning whatsoever if the character encoding is unknown,
    // so don't do it. If charset known, caller has converted text to utf-8, 
    // and this is also how we translate entities
    //    if (charset != "utf-8")
    //    	return;

    // We need a const_iterator version of s.end() - otherwise the
    // find() and find_if() templates don't work...
    string::const_iterator amp = s.begin(), s_end = s.end();
    while ((amp = find(amp, s_end, '&')) != s_end) {
	unsigned int val = 0;
	string::const_iterator end, p = amp + 1;
	string subs;
	if (p != s_end && *p == '#') {
	    p++;
	    if (p != s_end && tolower(*p) == 'x') {
		// hex
		p++;
		end = find_if(p, s_end, p_notxdigit);
		sscanf(s.substr(p - s.begin(), end - p).c_str(), "%x", &val);
	    } else {
		// number
		end = find_if(p, s_end, p_notdigit);
		val = atoi(s.substr(p - s.begin(), end - p).c_str());
	    }
	} else {
	    end = find_if(p, s_end, p_notalnum);
	    string code = s.substr(p - s.begin(), end - p);
	    map<string, string>::const_iterator i;
	    i = named_ents.find(code);
	    if (i != named_ents.end()) 
		subs = i->second;
	}

	if (end < s_end && *end == ';') 
	    end++;
	
	if (val) {
	    // The code is the code position for a unicode char. We need
	    // to translate it to an utf-8 string.
	    string utf16be;
	    utf16be += char(val / 256);
	    utf16be += char(val % 256);
	    transcode(utf16be, subs, "UTF-16BE", "UTF-8");
	} 

	if (subs.length() > 0) {
	    string::size_type amp_pos = amp - s.begin();
	    s.replace(amp_pos, end - amp, subs);
	    s_end = s.end();
	    // We've modified the string, so the iterators are no longer
	    // valid...
	    amp = s.begin() + amp_pos + subs.length();
	} else {
	    amp = end;
	}
    }
}

void
HtmlParser::parse_html(const string &body)
{
    map<string,string> Param;
    string::const_iterator start = body.begin();

    while (1) {
	string::const_iterator p = start;

	// Eat text until we find an HTML tag, a comment, or the end
	// of document.  Ignore isolated occurences of `<' which don't
	// start a tag or comment
	while (1) {
	    p = find(p, body.end(), '<');
	    if (p == body.end()) break;
	    char ch = *(p + 1);
	    // tag, closing tag, comment (or SGML declaration), or PHP
	    if (isalpha(ch) || ch == '/' || ch == '!' || ch == '?') break;
	    p++; 
	}

	// Process text
	if (p > start || p == body.end()) {
	    string text = body.substr(start - body.begin(), p - start);
	    decode_entities(text);
	    process_text(text);
	}

	if (p == body.end()) {
	    do_eof();
	    break;
	}

	start = p + 1;
   
	if (start == body.end()) break;

	if (*start == '!') {
	    if (++start == body.end()) break;
	    if (++start == body.end()) break;
	    // comment or SGML declaration
	    if (*(start - 1) == '-' && *start == '-') {
		start = find(start + 1, body.end(), '>');
		// unterminated comment swallows rest of document
		// (like NS, but unlike MSIE iirc)
		if (start == body.end()) break;
		
		p = start;
		// look for -->
		while (p != body.end() && (*(p - 1) != '-' || *(p - 2) != '-'))
		    p = find(p + 1, body.end(), '>');

		// If we found --> skip to there, otherwise
		// skip to the first > we found (as Netscape does)
		if (p != body.end()) start = p;
	    } else {
		// just an SGML declaration, perhaps giving the DTD - ignore it
		start = find(start - 1, body.end(), '>');
		if (start == body.end()) break;
	    }
	    ++start;
	} else if (*start == '?') {
	    if (++start == body.end()) break;
	    // PHP - swallow until ?> or EOF
	    start = find(start + 1, body.end(), '>');

	    // look for ?>
	    while (start != body.end() && *(start - 1) != '?')
		start = find(start + 1, body.end(), '>');

	    // unterminated PHP swallows rest of document (rather arbitrarily
	    // but it avoids polluting the database when things go wrong)
	    if (start != body.end()) ++start;
	} else {
	    // opening or closing tag
	    int closing = 0;

	    if (*start == '/') {
		closing = 1;
		start = find_if(start + 1, body.end(), p_notwhitespace);
	    }
	      
	    p = start;
	    start = find_if(start, body.end(), p_nottag);
	    string tag = body.substr(p - body.begin(), start - p);
	    // convert tagname to lowercase
	    for (string::iterator i = tag.begin(); i != tag.end(); i++)
		*i = tolower(*i);
	       
	    if (closing) {
		closing_tag(tag);
		   
		/* ignore any bogus parameters on closing tags */
		p = find(start, body.end(), '>');
		if (p == body.end()) break;
		start = p + 1;
	    } else {
		while (start < body.end() && *start != '>') {
		    string name, value;

		    p = find_if(start, body.end(), p_whitespaceeqgt);

		    name = body.substr(start - body.begin(), p - start);
		       
		    p = find_if(p, body.end(), p_notwhitespace);
		      
		    start = p;
		    if (start != body.end() && *start == '=') {
			int quote;
		       
			start = find_if(start + 1, body.end(), p_notwhitespace);

			p = body.end();
			   
			quote = *start;
			if (quote == '"' || quote == '\'') {
			    start++;
			    p = find(start, body.end(), quote);
			}
			   
			if (p == body.end()) {
			    // unquoted or no closing quote
			    p = find_if(start, body.end(), p_whitespacegt);
			    
			    value = body.substr(start - body.begin(), p - start);

			    start = find_if(p, body.end(), p_notwhitespace);
			} else {
			    value = body.substr(start - body.begin(), p - start);
			}
		       
			if (name.size()) {
			    // convert parameter name to lowercase
			    string::iterator i;
			    for (i = name.begin(); i != name.end(); i++)
				*i = tolower(*i);
			    // in case of multiple entries, use the first
			    // (as Netscape does)
			    if (Param.find(name) == Param.end())
				Param[name] = value;
			}
		    }
		}
		opening_tag(tag, Param);
		Param.clear();

		if (start != body.end() && *start == '>') ++start;
	    }
	}
    }
}
