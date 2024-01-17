#include <stdio.h>
#include <stdlib.h>

#include "readfile.h"
#include "base64.h"

using namespace std;

const char *thisprog;
static char usage [] = "testfile\n\n"
    ;
static void
Usage(void)
{
    fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_i      0x2 
#define OPT_P      0x4 

int main(int argc, char **argv)
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
            case 'i':    op_flags |= OPT_i; break;
            default: Usage();    break;
            }
        argc--; argv++;
    }
    
    if (op_flags & OPT_i)  {
        const char *values[] = {"", "1", "12", "123", "1234", "12345", "123456"};
        int nvalues = sizeof(values) / sizeof(char *);
        string in, out, back;
        int err = 0;
        for (int i = 0; i < nvalues; i++) {
            in = values[i];
            base64_encode(in, out);
            base64_decode(out, back);
            if (in != back) {
                fprintf(stderr, "In [%s] %d != back [%s] %d (out [%s] %d\n", 
                        in.c_str(), int(in.length()), 
                        back.c_str(), int(back.length()),
                        out.c_str(), int(out.length())
                    );
                err++;
            }
        }
        in.erase();
        in += char(0);
        in += char(0);
        in += char(0);
        in += char(0);
        base64_encode(in, out);
        base64_decode(out, back);
        if (in != back) {
            fprintf(stderr, "In [%s] %d != back [%s] %d (out [%s] %d\n", 
                    in.c_str(), int(in.length()), 
                    back.c_str(), int(back.length()),
                    out.c_str(), int(out.length())
                );
            err++;
        }
        exit(!(err == 0));
    } else {
        if (argc > 1)
            Usage();
        string infile;
        if (argc == 1) {
            infile = *argv++;argc--;
        }
        string idata, reason;
        if (!file_to_string(infile, idata, &reason)) {
            fprintf(stderr, "Can't read file: %s\n", reason.c_str());
            exit(1);
        }
        string odata;
        if (!base64_decode(idata, odata)) {
            fprintf(stderr, "Decoding failed\n");
            exit(1);
        }
        fwrite(odata.c_str(), 1, odata.size() * sizeof(string::value_type), stdout);
        exit(0);
    }
}
