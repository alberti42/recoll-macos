#include "pathut.h"

#include <stdio.h>
#include <stdlib.h>

#include <iostream>

using namespace std;
static const char *thisprog;

static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_r     0x2 
#define OPT_s     0x4 
static char usage [] =
    "pathut\n"
    ;
static void
Usage(void)
{
    fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

int main(int argc, const char **argv)
{
    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
        (*argv)++;
        if (!(**argv))
            /* Cas du "adb - core" */
            Usage();
        while (**argv)
            switch (*(*argv)++) {
            default: Usage();   break;
            }
    b1: argc--; argv++;
    }

    if (argc != 0)
        Usage();

    cout << "path_home() -> [" << path_home() << "]\n";
    cout << "path_tildexpand(~) -> [" << path_tildexpand("~") << "]\n";

    return 0;
}
