/* Copyright (C) 2017-2019 J.F.Dockes
 *
 * License: GPL 2.1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include <string>
#include <iostream>


using namespace std;

#include "readfile.h"
#include "transcode.h"

// Repeatedly transcode a small string for timing measurements
static const string testword("\xc3\xa9\x6c\x69\x6d\x69\x6e\xc3\xa9\xc3\xa0");
// Without cache 10e6 reps on y -> 6.68
// With cache                   -> 4.73
// With cache and lock          -> 4.9
void looptest()
{
    cout << testword << endl;
    string out;
    for (int i = 0; i < 10*1000*1000; i++) {
        if (!transcode(testword, out, "UTF-8", "UTF-16BE")) {
            cerr << "Transcode failed" << endl;
            break;
        }
    }
}

int main(int argc, char **argv)
{
#if 0
    looptest();
    exit(0);
#endif
    if (argc != 5) {
        cerr << "Usage: transcode ifilename icode ofilename ocode" << endl;
        exit(1);
    }
    const string ifilename = argv[1];
    const string icode = argv[2];
    const string ofilename = argv[3];
    const string ocode = argv[4];

    string text;
    if (!file_to_string(ifilename, text)) {
        cerr << "Couldnt read file, errno " << errno << endl;
        exit(1);
    }
    string out;
    if (!transcode(text, out, icode, ocode)) {
        cerr << out << endl;
        exit(1);
    }
    FILE *fp = fopen(ofilename.c_str(), "wb");
    if (fp == 0) {
        perror("Open/create output");
        exit(1);
    }
    if (fwrite(out.c_str(), 1, out.length(), fp) != out.length()) {
        perror("fwrite");
        exit(1);
    }
    fclose(fp);
    exit(0);
}
