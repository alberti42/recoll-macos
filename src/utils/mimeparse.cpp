#ifndef lint
static char rcsid[] = "@(#$Id: mimeparse.cpp,v 1.2 2005-03-17 14:02:06 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#ifndef TEST_MIMEPARSE

#include <string>
#include <ctype.h>
#include <stdio.h>

#include "mimeparse.h"

using namespace std;
#define WHITE " \t\n"

static void stripw_lc(string &in)
{
    // fprintf(stderr, "In: '%s'\n", in.c_str());
    string::size_type pos, pos1;
    pos = in.find_first_not_of(WHITE);
    if (pos == string::npos) {
	// All white
	in = "";
	return;
    }
    in.replace(0, pos, "");
    pos1 = in.find_last_not_of(WHITE); 
    if (pos1 != in.length() -1)
	in  = in.replace(pos1+1, string::npos, "");
    string::iterator i;
    for (i = in.begin(); i != in.end(); i++)
	*i = tolower(*i);
}

MimeHeaderValue parseMimeHeaderValue(const string &ein)
{
    string in = ein;
    MimeHeaderValue out;
    string::size_type pos;

    pos = in.find_first_not_of(WHITE);
    if (pos == string::npos)
	return out;
    in = in.substr(pos, string::npos);
    if ((pos = in.find_first_of(";")) == string::npos) {
	out.value = in;
	return out;
    } 
    out.value = in.substr(0, pos);
    stripw_lc(out.value);
    in = in.substr(pos+1, string::npos);
    for (;;) {
	// Skip whitespace
	if ((pos = in.find_first_not_of(WHITE)) == string::npos)
	    return out;
	in = in.substr(pos, string::npos);

	if ((pos = in.find_first_of("=")) == string::npos)
	    return out;
	string pname = in.substr(0, pos);
	stripw_lc(pname);
	in = in.substr(pos+1, string::npos);

	pos = in.find_first_of(";");
	string pvalue = in.substr(0, pos);
	stripw_lc(pvalue);
	out.params[pname] = pvalue;
	if (pos == string::npos)
	    return out;
	in = in.substr(pos+1, string::npos);
    }

    return out;

}

#else 

#include <string>
#include "mimeparse.h"
using namespace std;
int
main(int argc, const char **argv)
{

    MimeHeaderValue parsed;

    //    const char *tr = "text/html; charset=utf-8; otherparam=garb";
    const char *tr = "text/html;charset = UTF-8 ; otherparam=garb;";

    parsed = parseMimeHeaderValue(tr);
    
    printf("'%s' \n", parsed.value.c_str());
    map<string, string>::iterator it;
    for (it = parsed.params.begin();it != parsed.params.end();it++) {
	printf("  '%s' = '%s'\n", it->first.c_str(), it->second.c_str());
    }
}

#endif // TEST_MIMEPARSE
