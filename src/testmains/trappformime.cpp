#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>

#include <string>
#include <iostream>
#include <vector>
using namespace std;

#include "appformime.h"

static char *thisprog;

static char usage [] =
    "  trappformime <mime type>\n"
    "  trappformime --all\n"
    "  trappformime --fromname <name>\n"
    ;
static void
Usage(FILE *fp = stderr)
{
    fprintf(fp, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

static int       op_flags;
#define OPT_a    0x1
#define OPT_n    0x2
static struct option long_options[] = {
    {"fromname", required_argument, 0, 'n'},
    {"all", 0, 0, 'a'},
    {"help", 0, 0, 'h'},
    {0, 0, 0, 0}
};

int main(int argc, char *argv[])
{
    thisprog = argv[0];
    int ret;
    std::string nm;

    while ((ret = getopt_long(argc, argv, "han:", long_options, NULL)) != -1) {
        switch (ret) {
        case 'h': Usage(stdout); break;
        case 'n': nm = optarg; op_flags |= OPT_n; break;
        case 'a': op_flags |= OPT_a; break;
        default:
            Usage();
        }
    }
    std::string mime;
    if (op_flags & OPT_a) {
        if (optind != argc) 
            Usage();
    } else if (op_flags & OPT_n) {
        if (optind != argc) 
            Usage();
    } else {
        if (optind != argc - 1) 
            Usage();
        mime = argv[optind++];
    }
    string reason;
    DesktopDb *ddb = DesktopDb::getDb();
    if (nullptr == ddb) {
        std::cerr << "Could not create desktop db\n";
        exit(1);
    }
    if (op_flags & OPT_a) {
        std::vector<DesktopDb::AppDef> apps;
        if (!ddb->allApps(&apps)) {
            std::cerr << "allApps failed\n";
            return 1;
        }
        for (const auto& app : apps) {
            std::cout << app.name << ", " << app.command << '\n';
        }
    } else if (op_flags & OPT_n) {
        DesktopDb::AppDef app;
        if (!ddb->appByName(nm, app)) {
            std::cerr << "No application found with name [" << nm << "]" << '\n';
            return 1;
        }
        std::cout << app.name << ", " << app.command << '\n';
    } else {
        vector<DesktopDb::AppDef> appdefs;
        if (!ddb->appForMime(mime, &appdefs, &reason)) {
            std::cerr << "appForMime failed: " << reason << '\n';
            exit(1);
        }
        if (appdefs.empty()) {
            std::cerr << "No application found for [" << mime << "]" << '\n';
            exit(1);
        }
        std::cout << mime << " -> ";
        for (const auto& appdef : appdefs) {
            std::cout << "[" << appdef.name << ", " << appdef.command << "], ";
        }
        std::cout << '\n';
    }

    exit(0);
}
