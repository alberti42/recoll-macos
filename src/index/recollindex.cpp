#ifndef lint
static char rcsid[] = "@(#$Id: recollindex.cpp,v 1.11 2005-11-05 14:40:50 dockes Exp $ (C) 2004 J.F.Dockes";
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

int main(int argc, const char **argv)
{
    string reason;
    RclConfig *config = recollinit(cleanup, sigcleanup, reason);

    if (config == 0 || !config->ok()) {
	string str = "Configuration problem: ";
	str += reason;
	fprintf(stderr, "%s\n", str.c_str());
	exit(1);
    }
    indexer = new ConfIndexer(config);
  
    exit(!indexer->index());
}
