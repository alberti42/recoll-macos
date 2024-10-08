/* Copyright (C) 2006 J.F.Dockes
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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
// Takes a query and run it, no gui, results to stdout
#include "autoconfig.h"

#include <stdlib.h>

#include "rclconfig.h"
#include "recollq.h"
#include "rclutil.h"
#include "pathut.h"

static RclConfig *rclconfig;

int main(int argc, char **argv)
{
    pathut_setargv0(argv[0]);
    return(recollq(&rclconfig, argc, argv));
}
