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

#if defined(__gnu_linux__) 

#include <stdlib.h>
#include "execmd.h"
#include "smallut.h"

using std::string;
using std::vector;

// It seems that we could use sysconf as on macosx actually
bool getCpuConf(CpuConf& conf)
{
    vector<string> cmdv = create_vector<string>("sh")("-c")
	("egrep ^processor /proc/cpuinfo | wc -l");

    string result;
    if (!ExecCmd::backtick(cmdv, result))
	return false;
    conf.ncpus = atoi(result.c_str());
    if (conf.ncpus < 1 || conf.ncpus > 100)
	conf.ncpus = 1;
    return true;
}

#elif defined(__FreeBSD__) 

#include <stdlib.h>
#include "execmd.h"
#include "smallut.h"

using std::string;
using std::vector;

bool getCpuConf(CpuConf& conf)
{
    vector<string> cmdv = create_vector<string>("sysctl")("hw.ncpu");

    string result;
    if (!ExecCmd::backtick(cmdv, result))
	return false;
    conf.ncpus = atoi(result.c_str());
    if (conf.ncpus < 1 || conf.ncpus > 100)
	conf.ncpus = 1;
    return true;
}

#elif 0 && defined(_WIN32)
// On windows, indexing is actually twice slower with threads enabled +
// there is a bug and the process does not exit at the end of indexing.
// Until these are solved, pretend there is only 1 cpu
#include <thread>
bool getCpuConf(CpuConf& cpus)
{
#if 0
    // Native way
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );
    cpus.ncpus = sysinfo.dwNumberOfProcessors;
#else
    // c++11
    cpus.ncpus = std::thread::hardware_concurrency();
#endif
    return true;
}

#elif defined(__APPLE__)

#include <unistd.h>
bool getCpuConf(CpuConf& cpus)
{
    cpus.ncpus = sysconf( _SC_NPROCESSORS_ONLN );
    return true;
}
        
#else // Any other system

// Generic, pretend there is one
bool getCpuConf(CpuConf& cpus)
{
    cpus.ncpus = 1;
    return true;
}
#endif

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
