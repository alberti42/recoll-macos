#include "pathut.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <iostream>
#include <map>

using namespace std;

static std::map<std::string, int> options {
    {"path_home", 0},
    {"path_tildexpand", 0},
    {"listdir", 0},
    {"url_encode", 0},
        };

static const char *thisprog;
static void Usage(void)
{
    string sopts;
    for (const auto& opt: options) {
        sopts += "--" + opt.first + "\n";
    }
    fprintf(stderr, "%s: usage: %s\n%s", thisprog, thisprog, sopts.c_str());
    exit(1);
}

int main(int argc, char **argv)
{
    thisprog = *argv;
    std::vector<struct option> long_options;

    for (auto& entry : options) {
        struct option opt;
        opt.name = entry.first.c_str();
        opt.has_arg = 0;
        opt.flag = &entry.second;
        opt.val = 1;
        long_options.push_back(opt);
    }
    long_options.push_back({0, 0, 0, 0});

    while (getopt_long(argc, argv, "", &long_options[0], nullptr) != -1) {
    }
    if (options["path_home"]) {
        if (optind != argc) {
            cerr << "Usage: trsmallut --path_home\n";
            return 1;
        }
        cout << "path_home() -> [" << path_home() << "]\n";
    } else if (options["path_tildexpand"]) {
        if (optind >= argc) {
            cerr << "Usage: trsmallut --path_tildexpand <arg>\n";
            return 1;
        }
        string s = argv[optind];
        optind++;
        if (optind != argc) {
            return 1;
        }
        cout << "path_tildexpand(" << s << ") -> [" << path_tildexpand(s) << "]\n";
    } else if (options["url_encode"]) {
        if (optind >= argc) {
            cerr << "Usage: trsmallut --url_encode <arg> [offs=0]\n";
            return 1;
        }
        string s = argv[optind];
        optind++;
        int offs = 0;
        if (optind != argc) {
            offs = atoi(argv[optind]);
            optind++;
        }
        if (optind != argc) {
            return 1;
        }
        cout << "url_encode(" << s << ", " << offs << ") -> [" << url_encode(s, offs) << "]\n";
    } else if (options["listdir"]) {
        if (optind >= argc) {
            cerr << "Usage: trsmallut --listdir <arg>\n";
            return 1;
        }
        std::string path = argv[optind];
        optind++;
        if (optind != argc) {
            cerr << "Usage: trsmallut --listdir <arg>\n";
            return 1;
        }
        std::string reason;
        std::set<std::string> entries;
        if (!listdir(path, reason, entries)) {
            std::cerr<< "listdir(" << path << ") failed : " << reason << "\n";
            return 1;
        }
        for (const auto& entry : entries) {
            cout << entry << "\n";
        }
    } else {
        Usage();
    }

    return 0;
}
