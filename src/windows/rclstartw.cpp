/* Copyright (C) 2015 J.F.Dockes
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

#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

// This exists only because I could not find any way to get "cmd /c
// start fn" to work when executed from the recoll GUI. Otoh,
// cygstart, which uses ShellExecute() did work... So this is just a
// simpler cygstart
static char *thisprog;
static char usage [] ="rclstartw <fn>\n" 
    "   Will use ShellExecute to open the arg with the default app\n";

static void Usage(FILE *fp = stderr)
{
    fprintf(fp, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

int main(int argc, char *argv[])
{
    thisprog = argv[0];
    argc--; argv++;

    if (argc != 1) {
        Usage();
    }
    char *fn = strdup(argv[0]);

    // Do we need this ?
    //https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153%28v=vs.85%29.aspx
    //CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    
    int ret = (int)ShellExecute(NULL, "open", fn, NULL, NULL, SW_SHOWNORMAL);
    if (ret) {
        fprintf(stderr, "ShellExecute returned %d\n", ret);
    }
    return ret;
}
