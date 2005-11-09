#ifndef lint
static char rcsid[] = "@(#$Id: rclinit.cpp,v 1.3 2005-11-09 21:39:04 dockes Exp $ (C) 2004 J.F.Dockes";
#endif

#include <stdio.h>
#include <signal.h>

#include "debuglog.h"
#include "rclconfig.h"
#include "rclinit.h"

RclConfig *recollinit(void (*cleanup)(void), void (*sigcleanup)(int), 
		      string &reason)
{
    if (cleanup)
	atexit(cleanup);
    if (sigcleanup) {
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
	    signal(SIGHUP, sigcleanup);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	    signal(SIGINT, sigcleanup);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
	    signal(SIGQUIT, sigcleanup);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
	    signal(SIGTERM, sigcleanup);
    }
    DebugLog::getdbl()->setloglevel(DEBDEB1);
    DebugLog::setfilename("stderr");
    RclConfig *config = new RclConfig;
    if (!config || !config->ok()) {
	reason = "Configuration could not be built:\n";
	if (config)
	    reason += config->getReason();
	else
	    reason += "Out of memory ?";
	return 0;
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
