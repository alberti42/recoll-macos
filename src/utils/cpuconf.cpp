/* Copyright (C) 2013 J.F.Dockes
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
#ifndef TEST_CPUCONF

#include "autoconfig.h"


#include "cpuconf.h"

#include <thread>

// Go c++11 !
bool getCpuConf(CpuConf& cpus)
{
    // c++11
    cpus.ncpus = std::thread::hardware_concurrency();
    return true;
}

#else // TEST_CPUCONF

#include <stdlib.h>

#include <iostream>
using namespace std;

#include "cpuconf.h"

// Test driver
int main(int argc, const char **argv)
{
    CpuConf cpus;
    if (!getCpuConf(cpus)) {
	cerr << "getCpuConf failed" << endl;
	exit(1);
    }
    cout << "Cpus: " << cpus.ncpus << endl;
    exit(0);
}
#endif // TEST_CPUCONF
