#ifndef lint
static char rcsid[] = "@(#$Id: mimeparse.cpp,v 1.3 2005-03-25 09:40:28 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#ifndef TEST_MIMEPARSE

#include <string>
#include <ctype.h>
#include <stdio.h>
#include <ctype.h>

#include "mimeparse.h"

using namespace std;

// Parsing a header value. Only content-type has parameters, but
// others are compatible with content-type syntax, only, parameters
// are not used. So we can parse all like content-type:
//    headertype: value [; paramname=paramvalue] ...
// Value and paramvalues can be quoted strings, and there can be
// comments in there



// The lexical token returned by find_next_token
class Lexical {
 public:
    enum kind {none, token, separator};
    kind   what;
    string value;
    string error;
    char quote;
    Lexical() : what(none), quote(0) {}
    void reset() {what = none; value.erase(); error.erase();quote = 0;}
};

// Skip mime comment. This must be called with in[start] == '('
int skip_comment(const string &in, unsigned int start, Lexical &lex)
{
    int commentlevel = 0;
    for (; start < in.size(); start++) {
	if (in[start] == '\\') {
	    // Skip escaped char. 
	    if (start+1 < in.size()) {
		start++;
		continue;
	    } else {
		lex.error.append("\\ at end of string ");
		return string::npos;
	    }
	}
	if (in[start] == '(')
	    commentlevel++;
	if (in[start] == ')') {
	    if (--commentlevel == 0)
		break;
	}
    }
    if (start == in.size()) {
	lex.error.append("Unclosed comment ");
	return string::npos;
    }
    return start;
}

// Skip initial whitespace and (possibly nested) comments. 
int skip_whitespace_and_comment(const string &in, unsigned int start, 
				Lexical &lex)
{
    while (1) {
	if ((start = in.find_first_not_of(" \t\r\n", start)) == string::npos)
	    return in.size();
	if (in[start] == '(') {
	    if ((start = skip_comment(in, start, lex)) == string::npos)
		return string::npos;
	} else {
	    break;
	}
    }
    return start;
}

/// Find next token in mime header value string. 
/// @return the next starting position in string, string::npos for error 
///   (ie unbalanced quoting)
/// @param in the input string
/// @param start the starting position
/// @param lex  the returned token and its description
/// @param delims separators we should look for
int find_next_token(const string &in, unsigned int start, 
		    Lexical &lex, string delims = ";=")
{
    char oquot, cquot;

    start = skip_whitespace_and_comment(in, start, lex);
    if (start == string::npos || start == in.size())
	return start;

    // Begins with separator ? return it.
    unsigned int delimi = delims.find_first_of(in[start]);
    if (delimi != string::npos) {
	lex.what = Lexical::separator;
	lex.value = delims[delimi];
	return start+1;
    }

    // Check for start of quoted string
    oquot = in[start];
    switch (oquot) {
    case '<': cquot = '>';break;
    case '"': cquot = '"';break;
    default: cquot = 0; break;
    }

    if (cquot != 0) {
	// Quoted string parsing
	unsigned int end;
	start++; // Skip quote character
	for (end = start;end < in.size() && in[end] != cquot; end++) {
	    if (in[end] == '\\') {
		// Skip escaped char. 
		if (end+1 < in.size()) {
		    end++;
		} else {
		    // backslash at end of string: error
		    lex.error.append("\\ at end of string ");
		    return string::npos;
		}
	    }
	}
	if (end == in.size()) {
	    // Found end of string before closing quote character: error
	    lex.error.append("Unclosed quoted string ");
	    return string::npos;
	}
	lex.what = Lexical::token;
	lex.value = in.substr(start, end-start);
	lex.quote = oquot;
	return ++end;
    } else {
	unsigned int end = in.find_first_of(delims + " \t(", start);
	lex.what = Lexical::token;
	lex.quote = 0;
	if (end == string::npos) {
	    end = in.size();
	    lex.value = in.substr(start);
	} else {
	    lex.value = in.substr(start, end-start);
	}
	return end;
    }
}

void stringtolower(string &out, const string& in)
{
    for (unsigned int i = 0; i < in.size(); i++)
	out.append(1, char(tolower(in[i])));
}

bool parseMimeHeaderValue(const string& value, MimeHeaderValue& parsed)
{
    parsed.value.erase();
    parsed.params.clear();

    Lexical lex;
    unsigned int start = 0;
    start = find_next_token(value, start, lex);
    if (start == string::npos || lex.what != Lexical::token) 
	return false;

    parsed.value = lex.value;

    for (;;) {
	string paramname, paramvalue;
	lex.reset();
	start = find_next_token(value, start, lex);
	if (start == value.size())
	    return true;
	if (start == string::npos)
	    return false;
	if (lex.what == Lexical::separator && lex.value[0] == ';')
	    continue;
	if (lex.what != Lexical::token) 
	    return false;
	stringtolower(paramname, lex.value);

	start = find_next_token(value, start, lex);
	if (start == string::npos || lex.what != Lexical::separator || 
	    lex.value[0] != '=') 
	    return false;

	start = find_next_token(value, start, lex);
	if (start == string::npos || lex.what != Lexical::token)
	    return false;
	paramvalue = lex.value;
	parsed.params[paramname] = paramvalue;
    }
    return true;
}

// Decode a string encoded with quoted-printable encoding. 
bool qp_decode(const string& in, string &out) 
{
    out.reserve(in.length());
    unsigned int ii;
    for (ii = 0; ii < in.length(); ii++) {
	if (in[ii] == '=') {
	    ii++; // Skip '='
	    if(ii >= in.length() - 1) { // Need at least 2 more chars
		break;
	    } else if (in[ii] == '\r' && in[ii+1] == '\n') { // Soft nl, skip
		ii++;
	    } else if (in[ii] != '\n' && in[ii] != '\r') { // decode
		char c = in[ii];
		char co;
		if(c >= 'A' && c <= 'F') {
		    co = char((c - 'A' + 10) * 16);
		} else if (c >= 'a' && c <= 'f') {
		    co = char((c - 'a' + 10) * 16);
		} else if (c >= '0' && c <= '9') {
		    co = char((c - '0') * 16);
		} else {
		    return false;
		}
		if(++ii >= in.length()) 
		    break;
		c = in[ii];
		if (c >= 'A' && c <= 'F') {
		    co += char(c - 'A' + 10);
		} else if (c >= 'a' && c <= 'f') {
		    co += char(c - 'a' + 10);
		} else if (c >= '0' && c <= '9') {
		    co += char(c - '0');
		} else {
		    return false;
		}
		out += co;
	    }
	} else {
	    out += in[ii];
	}
    }
    return true;
}


// This is adapted from FreeBSD's code.
static const char Base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Pad64 = '=';
bool base64_decode(const string& in, string& out)
{
    int io = 0, state = 0, ch;
    char *pos;
    unsigned int ii = 0;
    out.reserve(in.length());

    for (ii = 0; ii < in.length(); ii++) {
	ch = in[ii];
	if (isspace((unsigned char)ch))        /* Skip whitespace anywhere. */
	    continue;

	if (ch == Pad64)
	    break;

	pos = strchr(Base64, ch);
	if (pos == 0) 		/* A non-base64 character. */
	    return false;

	switch (state) {
	case 0:
	    out[io] = (pos - Base64) << 2;
	    state = 1;
	    break;
	case 1:
	    out[io]   |=  (pos - Base64) >> 4;
	    out[io+1]  = ((pos - Base64) & 0x0f) << 4 ;
	    io++;
	    state = 2;
	    break;
	case 2:
	    out[io]   |=  (pos - Base64) >> 2;
	    out[io+1]  = ((pos - Base64) & 0x03) << 6;
	    io++;
	    state = 3;
	    break;
	case 3:
	    out[io] |= (pos - Base64);
	    io++;
	    state = 0;
	    break;
	default:
	    return false;
	}
    }

    /*
     * We are done decoding Base-64 chars.  Let's see if we ended
     * on a byte boundary, and/or with erroneous trailing characters.
     */

    if (ch == Pad64) {		/* We got a pad char. */
	ch = in[ii++];		/* Skip it, get next. */
	switch (state) {
	case 0:		/* Invalid = in first position */
	case 1:		/* Invalid = in second position */
	    return false;

	case 2:		/* Valid, means one byte of info */
			/* Skip any number of spaces. */
	    for (; ii < in.length(); ch = in[ii++])
		if (!isspace((unsigned char)ch))
		    break;
	    /* Make sure there is another trailing = sign. */
	    if (ch != Pad64)
		return false;
	    ch = in[ii++];		/* Skip the = */
	    /* Fall through to "single trailing =" case. */
	    /* FALLTHROUGH */

	case 3:		/* Valid, means two bytes of info */
			/*
			 * We know this char is an =.  Is there anything but
			 * whitespace after it?
			 */
	    for ((void)NULL; ii < in.length(); ch = in[ii++])
		if (!isspace((unsigned char)ch))
		    return false;

	    /*
	     * Now make sure for cases 2 and 3 that the "extra"
	     * bits that slopped past the last full byte were
	     * zeros.  If we don't check them, they become a
	     * subliminal channel.
	     */
	    if (out[io] != 0)
		return false;
	}
    } else {
	/*
	 * We ended by seeing the end of the string.  Make sure we
	 * have no partial bytes lying around.
	 */
	if (state != 0)
	    return false;
    }

    return true;
}

#else 

#include <string>
#include "mimeparse.h"
using namespace std;
int
main(int argc, const char **argv)
{
#if 0
    //    const char *tr = "text/html; charset=utf-8; otherparam=garb";
    const char *tr = "text/html;charset = UTF-8 ; otherparam=garb; \n"
	"QUOTEDPARAM=\"quoted value\"";

    MimeHeaderValue parsed;

    if (!parseMimeHeaderValue(tr, parsed)) {
	fprintf(stderr, "PARSE ERROR\n");
    }
    
    printf("'%s' \n", parsed.value.c_str());
    map<string, string>::iterator it;
    for (it = parsed.params.begin();it != parsed.params.end();it++) {
	printf("  '%s' = '%s'\n", it->first.c_str(), it->second.c_str());
    }
#elif 0
    const char *qp = "=41=68 =e0 boire=\r\n continue 1ere\ndeuxieme\n\r3eme "
	"agrave is: '=E0' probable skipped decode error: =\n"
	"Actual decode error =xx this wont show";

    string out;
    if (!qp_decode(string(qp), out)) {
	fprintf(stderr, "qp_decode returned error\n");
    }
    printf("Decoded: '%s'\n", out.c_str());
#else
    //'C'est à boire qu'il nous faut éviter l'excès.'
    //'Deuxième ligne'
    //'Troisième ligne'
    //'Et la fin (pas de nl). '
    const char *b64 = 
 "Qydlc3Qg4CBib2lyZSBxdSdpbCBub3VzIGZhdXQg6XZpdGVyIGwnZXhj6HMuCkRldXhp6G1l\r\n"
	"IGxpZ25lClRyb2lzaehtZSBsaWduZQpFdCBsYSBmaW4gKHBhcyBkZSBubCkuIA==\r\n";

    string out;
    if (!base64_decode(string(b64), out)) {
	fprintf(stderr, "base64_decode returned error\n");
    }
    printf("Decoded: '%s'\n", out.c_str());
#endif
}

#endif // TEST_MIMEPARSE
