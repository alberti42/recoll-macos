#ifndef lint
static char rcsid[] = "@(#$Id: recollindex.cpp,v 1.10 2005-04-05 09:35:35 dockes Exp $ (C) 2004 J.F.Dockes";
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
    RclConfig *config = recollinit(cleanup, sigcleanup);

    indexer = new ConfIndexer(config);
  
    exit(!indexer->index());
}
