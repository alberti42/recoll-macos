#ifndef lint
static char rcsid[] = "@(#$Id: utf8iter.cpp,v 1.1 2005-02-10 19:52:50 dockes Exp $ (C) 2005 J.F.Dockes";
#endif
#include <stdio.h>
#include <string>
#include <iostream>
#include <list>
#include "debuglog.h"
using namespace std;

#include "utf8iter.h"
#include "readfile.h"



int main(int argc, char **argv)
{
    if (argc != 3) {
	cerr << "Usage: utf8iter infile outfile" << endl;
	exit(1);
    }
    const char *infile = argv[1];
    const char *outfile = argv[2];
    string in;
    string out;
    if (!file_to_string(infile, in)) {
	cerr << "Cant read file\n" << endl;
	exit(1);
    }
    Utf8Iter it(in);
    FILE *fp = fopen(outfile, "w");
    if (fp == 0) {
	fprintf(stderr, "cant create %s\n", outfile);
	exit(1);
    }
    while (!it.eof()) {
	unsigned int value = *it;
	it.appendchartostring(out);
	it++;
	fwrite(&value, 4, 1, fp);
    }
    fclose(fp);
    if (it.error()) {
	fprintf(stderr, "Conversion error occurred\n");
	exit(1);
    }
    if (in != out) {
	fprintf(stderr, "error: out != in\n");
	exit(1);
    }
    exit(0);
}

