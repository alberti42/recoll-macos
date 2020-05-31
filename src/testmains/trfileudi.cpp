/* Copyright (C) 2017-2019 J.F.Dockes
 *
 * License: GPL 2.1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "fileudi.h"

#include <stdio.h>
#include <string>

using namespace std;

int main(int argc, char **argv)
{
    string path="/usr/lib/toto.cpp";
    string ipath = "1:2:3:4:5:10";
    string udi;
    make_udi(path, ipath, udi);
    printf("udi [%s]\n", udi.c_str());
    path = "/some/much/too/looooooooooooooong/path/bla/bla/bla"
        "/looooooooooooooong/path/bla/bla/bla/llllllllllllllllll"
        "/looooooooooooooong/path/bla/bla/bla/llllllllllllllllll";
    ipath = "1:2:3:4:5:10"
        "1:2:3:4:5:10"
        "1:2:3:4:5:10";
    make_udi(path, ipath, udi);
    printf("udi [%s]\n", udi.c_str());
}

