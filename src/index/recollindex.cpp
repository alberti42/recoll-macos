#ifndef lint
static char rcsid[] = "@(#$Id: recollindex.cpp,v 1.8 2005-01-31 14:31:09 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <stdio.h>
#include <signal.h>

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
    atexit(cleanup);
    if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
	signal(SIGHUP, sigcleanup);
    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	signal(SIGINT, sigcleanup);
    if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
	signal(SIGQUIT, sigcleanup);
    if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
	signal(SIGTERM, sigcleanup);

    RclConfig config;
    if (!config.ok()) {
	fprintf(stderr, "Config could not be built\n");
	exit(1);
    }
    indexer = new ConfIndexer(&config);
    
    exit(!indexer->index());
}
