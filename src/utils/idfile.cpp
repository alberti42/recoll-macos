#ifndef lint
static char rcsid[] = "@(#$Id: idfile.cpp,v 1.4 2006-01-23 13:32:28 dockes Exp $ (C) 2005 J.F.Dockes";
#endif
/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef TEST_IDFILE
#include <unistd.h> // for access(2)
#include <ctype.h>

#include <fstream>
#include <sstream>

#include "idfile.h"
#include "debuglog.h"

#ifndef NO_NAMESPACES
using namespace std;
#endif /* NO_NAMESPACES */

std::list<string> idFileAllTypes()
{
    std::list<string> lst;
    lst.push_back("text/x-mail");
    lst.push_back("message/rfc822");
    return lst;
}

// Mail headers we compare to:
static const char *mailhs[] = {"From: ", "Received: ", "Message-Id: ", "To: ", 
			       "Date: ", "Subject: ", "Status: "};
static const int mailhsl[] = {6, 10, 12, 4, 6, 9, 8};
static const int nmh = sizeof(mailhs) / sizeof(char *);

const int wantnhead = 3;

string idFile(const char *fn)
{
    ifstream input;
    input.open(fn, ios::in);
    if (!input.is_open()) {
	LOGERR(("idFile: could not open [%s]\n", fn));
	return string("");
    }	    

    bool line1HasFrom = false;
    int lookslikemail = 0;

    // emacs VM sometimes inserts very long lines with continuations or
    // not (for folder information). This forces us to look at many
    // lines and long ones
    for (int lnum = 1; lnum < 200; lnum++) {

#define LL 1024
	char cline[LL+1];
	cline[LL] = 0;
	input.getline(cline, LL-1);
	if (input.fail()) {
	    if (input.bad()) {
		LOGERR(("idfile: error while reading [%s]\n", fn));
		return string("");
	    }
	    // Must be eof ?
	    break;
	}

	LOGDEB2(("idfile: lnum %d : [%s]\n", lnum, cline));
	// Check for a few things that can't be found in a mail file,
	// (optimization to get a quick negative

	// Lines must begin with whitespace or have a colon in the
	// first 50 chars (hope no one comes up with a longer header
	// name !
	if (!isspace(cline[0])) {
	    char *cp = strchr(cline, ':');
	    if (cp == 0 || (cp - cline) > 70) {
		LOGDEB2(("idfile: can't be mail header line: [%s]\n", cline));
		break;
	    }
	}
 
	int ll = strlen(cline);
	if (ll > 1000) {
	    LOGDEB2(("idFile: Line too long\n"));
	    return string("");
	}
	if (lnum == 1) {
	    if (!strncmp("From ", cline, 5)) {
		line1HasFrom = true;
		continue;
	    }
	}

	for (int i = 0; i < nmh; i++) {
	    if (!strncasecmp(mailhs[i], cline, mailhsl[i])) {
		//fprintf(stderr, "Got [%s]\n", mailhs[i]);
		lookslikemail++;
		break;
	    }
	}
	if (lookslikemail >= wantnhead)
	    break;
    }
    if (line1HasFrom)
	lookslikemail++;

    if (lookslikemail >= wantnhead)
	return line1HasFrom ? string("text/x-mail") : string("message/rfc822");

    return string("");
}


#else

#include <string>
#include <iostream>

#include <unistd.h>
#include <fcntl.h>

using namespace std;

#include "debuglog.h"
#include "idfile.h"

int main(int argc, char **argv)
{
    if (argc != 2) {
	cerr << "Usage: idfile filename" << endl;
	exit(1);
    }
    DebugLog::getdbl()->setloglevel(DEBDEB1);
    DebugLog::setfilename("stderr");
    string mime = idFile(argv[1]);
    cout << argv[1] << " : " << mime << endl;
    exit(0);
}

#endif
