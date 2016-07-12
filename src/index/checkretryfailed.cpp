/* Copyright (C) 2014 J.F.Dockes
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
#include "autoconfig.h"

#include <string>
#include <vector>

#include "rclconfig.h"
#include "execmd.h"
#include "log.h"
#include "checkretryfailed.h"

using namespace std;

bool checkRetryFailed(RclConfig *conf, bool record)
{
#ifdef _WIN32
    return true;
#else
    string cmd;

    if (!conf->getConfParam("checkneedretryindexscript", cmd)) {
        LOGDEB("checkRetryFailed: 'checkneedretryindexscript' not set in config\n" );
        // We could toss a dice ? Say no retry in this case.
        return false;
    }

    // Look in the filters directory (ies). If not found execpath will
    // be the same as cmd, and we'll let execvp do its thing.
    string execpath = conf->findFilter(cmd);

    vector<string> args;
    if (record) {
        args.push_back("1");
    }
    ExecCmd ecmd;
    int status = ecmd.doexec(execpath, args);
    if (status == 0) {
        return true;
    } 
    return false;
#endif
}

