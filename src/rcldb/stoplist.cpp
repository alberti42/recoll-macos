#ifndef lint
static char rcsid[] = "@(#$Id: stoplist.cpp,v 1.1 2007-06-02 08:30:42 dockes Exp $ (C) 2007 J.F.Dockes";
#endif
#ifndef TEST_STOPLIST
#include "debuglog.h"
#include "readfile.h"
#include "unacpp.h"
#include "textsplit.h"
#include "stoplist.h"

#ifndef NO_NAMESPACES
namespace Rcl 
{
#endif

class TextSplitSW : public TextSplit {
public:
    set<string>& stops;
    TextSplitSW(Flags flags, set<string>& stps) 
        : TextSplit(flags), stops(stps) 
    {}
    virtual bool takeword(const string& term, int, int, int)
    {
        string dterm;
        unacmaybefold(term, dterm, "UTF-8", true);
        stops.insert(dterm);
        return true;
    }
};

bool StopList::setFile(const string &filename)
{
    m_hasStops = false;
    m_stops.clear();
    string stoptext, reason;
    if (!file_to_string(filename, stoptext, &reason)) {
	LOGDEB(("StopList::StopList: file_to_string(%s) failed: %s\n", 
		filename.c_str(), reason.c_str()));
	return false;
    }
    TextSplitSW ts(TextSplit::TXTS_ONLYSPANS, m_stops);
    ts.text_to_words(stoptext);
    m_hasStops = !m_stops.empty();
    return true;
}

bool StopList::isStop(const string &term) const
{
    return m_hasStops ? m_stops.find(term) != m_stops.end() : false;
}


#ifndef NO_NAMESPACES
}
#endif

#else // TEST_STOPLIST

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <string>
#include <iostream>

#include "stoplist.h"

using namespace std;
using namespace Rcl;

static char *thisprog;

static char usage [] =
"trstoplist stopstermsfile\n\n"
;
static void
Usage(void)
{
    fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

const string tstwords[] = {
    "the", "is", "xweird"
};
const int tstsz = sizeof(tstwords) / sizeof(string);

int main(int argc, char **argv)
{
  int count = 10;
    
  thisprog = argv[0];
  argc--; argv++;

  if (argc != 1)
    Usage();
  string filename = argv[0]; argc--;

  StopList sl(filename);

  for (int i = 0; i < tstsz; i++) {
      const string &tst = tstwords[i];
      cout << "[" << tst << "] " << 
	  (sl.isStop(tst) ? "in stop list" : "not in stop list") << endl;
  }
  exit(0);
}

#endif // TEST_STOPLIST
