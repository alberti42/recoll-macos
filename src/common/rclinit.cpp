#ifndef lint
static char rcsid[] = "@(#$Id: rclinit.cpp,v 1.6 2006-09-08 09:02:47 dockes Exp $ (C) 2004 J.F.Dockes";
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

#include <stdio.h>
#include <signal.h>
#include <locale.h>

#include "debuglog.h"
#include "rclconfig.h"
#include "rclinit.h"

RclConfig *recollinit(void (*cleanup)(void), void (*sigcleanup)(int), 
		      string &reason, const string *argcnf)
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
    RclConfig *config = new RclConfig(argcnf);
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

    // Make sure the locale is set. This is only for converting file names 
    // to utf8 for indexation.
    setlocale(LC_CTYPE, "");

    return config;
}
