#ifndef lint
static char rcsid[] = "@(#$Id: rclinit.cpp,v 1.1 2005-04-05 09:35:35 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <stdio.h>
#include <signal.h>

#include "debuglog.h"
#include "rclconfig.h"

RclConfig *recollinit(void (*cleanup)(void), void (*sigcleanup)(int))
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

    DebugLog::getdbl()->setloglevel(DEBDEB1);
    DebugLog::setfilename("stderr");
    RclConfig *config = new RclConfig;
    if (!config || !config->ok()) {
	fprintf(stderr, "Config could not be built\n");
	exit(1);
    }

    string logfilename, loglevel;
    if (config->getConfParam(string("logfilename"), logfilename))
	DebugLog::setfilename(logfilename.c_str());
    if (config->getConfParam(string("loglevel"), loglevel)) {
	int lev = atoi(loglevel.c_str());
	DebugLog::getdbl()->setloglevel(lev);
    }
    
    return config;
}
