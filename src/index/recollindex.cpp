#ifndef lint
static char rcsid[] = "@(#$Id: recollindex.cpp,v 1.12 2005-11-30 09:46:25 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <stdio.h>
#include <signal.h>

#include "debuglog.h"
#include "rclinit.h"
#include "indexer.h"

ConfIndexer *indexer;

static void cleanup()
{
    delete indexer;
    indexer = 0;
}

static void sigcleanup(int sig)
{
    fprintf(stderr, "sigcleanup\n");
    cleanup();
    exit(1);
}

static const char *thisprog;
static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_z     0x2 
#define OPT_h     0x4 

static const char usage [] =
"  recollindex [-hz]\n"
"Options:\n"
" -h : print this message\n"
" -z : reset database before starting indexation\n\n"
;

static void
Usage(void)
{
    FILE *fp = (op_flags & OPT_h) ? stdout : stderr;
    fprintf(fp, "%s: usage: %s", thisprog, usage);
    exit((op_flags & OPT_h)==0);
}


int main(int argc, const char **argv)
{
    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
	(*argv)++;
	if (!(**argv))
	    Usage();
	while (**argv)
	    switch (*(*argv)++) {
	    case 'z': op_flags |= OPT_z; break;
	    case 'h': op_flags |= OPT_h; break;
	    default: Usage(); break;
	    }
    }
    if (op_flags & OPT_h)
	Usage();
    string reason;
    RclConfig *config = recollinit(cleanup, sigcleanup, reason);

    if (config == 0 || !config->ok()) {
	string str = "Configuration problem: ";
	str += reason;
	fprintf(stderr, "%s\n", str.c_str());
	exit(1);
    }

    indexer = new ConfIndexer(config);
  
    exit(!indexer->index((op_flags & OPT_z) != 0));
}
