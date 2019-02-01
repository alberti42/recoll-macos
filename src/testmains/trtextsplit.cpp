#include "autoconfig.h"

#include "textsplit.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#include <iostream>

#include "readfile.h"
#include "log.h"
#include "transcode.h"
#include "unacpp.h"
#include "termproc.h"
#include "rclutil.h"
#include "rclconfig.h"

using namespace std;

class myTermProc : public Rcl::TermProc {
    int first;
    bool nooutput;
public:
    myTermProc() : TermProc(0), first(1), nooutput(false) {}
    void setNoOut(bool val) {nooutput = val;}
    virtual bool takeword(const string &term, int pos, int bs, int be)
    {
        if (nooutput)
            return true;
        FILE *fp = stdout;
        if (first) {
            fprintf(fp, "%3s %-20s %4s %4s\n", "pos", "Term", "bs", "be");
            first = 0;
        }
        fprintf(fp, "%3d %-20s %4d %4d\n", pos, term.c_str(), bs, be);
        return true;
    }
};

#define OPT_s     0x1 
#define OPT_w     0x2
#define OPT_q     0x4
#define OPT_c     0x8
#define OPT_k     0x10
#define OPT_C     0x20
#define OPT_n     0x40
#define OPT_S     0x80
#define OPT_u     0x100
#define OPT_p     0x200

bool dosplit(const string& data, TextSplit::Flags flags, int op_flags)
{
    myTermProc printproc;

    Rcl::TermProc *nxt = &printproc;

//    Rcl::TermProcCommongrams commonproc(nxt, stoplist);
//    if (op_flags & OPT_S)
//        nxt = &commonproc;

    Rcl::TermProcPrep preproc(nxt);
    if (op_flags & OPT_u) 
        nxt = &preproc;

    Rcl::TextSplitP splitter(nxt, flags);

    if (op_flags & OPT_q)
        printproc.setNoOut(true);

    splitter.text_to_words(data);

#ifdef TEXTSPLIT_STATS
        TextSplit::Stats::Values v = splitter.getStats();
        cout << "Average length: " 
             <<  v.avglen
             << " Standard deviation: " 
             << v.sigma
             << " Coef of variation "
             << v.sigma / v.avglen
             << endl;
#endif
    return true;
}

static const char *teststrings[] = {
    "Un bout de texte \nnormal. 2eme phrase.3eme;quatrieme.\n",
    "\"Jean-Francois Dockes\" <jfd@okyz.com>\n",
    "n@d @net .net net@ t@v@c c# c++ o'brien 'o'brien'",
    "_network_ some_span",
    "data123\n",
    "134 +134 -14 0.1 .1 2. -1.5 +1.5 1,2 1.54e10 1,2e30 .1e10 1.e-8\n",
    "@^#$(#$(*)\n",
    "192.168.4.1 one\n\rtwo\r",
    "[olala][ululu]  (valeur) (23)\n",
    "utf-8 ucs-4© \\nodef\n",
    "A b C 2 . +",
    "','this\n",
    " ,able,test-domain",
    " -wl,--export-dynamic",
    " ~/.xsession-errors",
    "this_very_long_span_this_very_long_span_this_very_long_span",
    "soft\xc2\xadhyphen",
    "soft\xc2\xad\nhyphen",
    "soft\xc2\xad\n\rhyphen",
    "real\xe2\x80\x90hyphen",
    "real\xe2\x80\x90\nhyphen",
    "hyphen-\nminus",
};
const int teststrings_cnt = sizeof(teststrings)/sizeof(char *);

static string teststring1 = " nouvel-an ";

static string thisprog;

static string usage =
    " textsplit [opts] [filename]\n"
    "   -q : no output\n"
    "   -s :  only spans\n"
    "   -w :  only words\n"
    "   -n :  no numbers\n"
    "   -k :  preserve wildcards (?*)\n"
    "   -c : just count words\n"
    "   -u : use unac\n"
    "   -C [charset] : input charset\n"
    "   -S [stopfile] : stopfile to use for commongrams\n"
    " if filename is 'stdin', will read stdin for data (end with ^D)\n\n"
    " textplit -p somephrase : display results from stringToStrings()\n"
    "  \n"
    ;

static void
Usage(void)
{
    cerr << thisprog  << ": usage:\n" << usage;
    exit(1);
}

static int        op_flags;

int main(int argc, char **argv)
{
    string charset, stopfile;

    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
        (*argv)++;
        if (!(**argv))
            /* Cas du "adb - core" */
            Usage();
        while (**argv)
            switch (*(*argv)++) {
            case 'c':   op_flags |= OPT_c; break;
            case 'C':   op_flags |= OPT_C; if (argc < 2)  Usage();
                charset = *(++argv); argc--; 
                goto b1;
            case 'k':   op_flags |= OPT_k; break;
            case 'n':   op_flags |= OPT_n; break;
            case 'p':   op_flags |= OPT_p; break;
            case 'q':   op_flags |= OPT_q; break;
            case 's':   op_flags |= OPT_s; break;
            case 'S':   op_flags |= OPT_S; if (argc < 2)  Usage();
                stopfile = *(++argv); argc--; 
                goto b1;
            case 'u':   op_flags |= OPT_u; break;
            case 'w':   op_flags |= OPT_w; break;
            default: Usage();   break;
            }
    b1: argc--; argv++;
    }

    TextSplit::Flags flags = TextSplit::TXTS_NONE;

    if (op_flags&OPT_s)
        flags = TextSplit::TXTS_ONLYSPANS;
    else if (op_flags&OPT_w)
        flags = TextSplit::TXTS_NOSPANS;
    if (op_flags & OPT_k) 
        flags = (TextSplit::Flags)(flags | TextSplit::TXTS_KEEPWILD); 


    // We need a configuration file, which we build in a temp file
    TempFile tmpconf("conf");
    string cffn(tmpconf.filename());
    FILE *fp = fopen(tmpconf.filename(), "w");
    if (op_flags & OPT_n) {
        fprintf(fp, "nonumbers = 1");
    }
    fclose(fp);

    RclConfig *config = new RclConfig(&cffn);
    TextSplit::staticConfInit(config);


    Rcl::StopList stoplist;
    if (op_flags & OPT_S) {
        if (!stoplist.setFile(stopfile)) {
            cerr << "Can't read stopfile: " << stopfile << endl;
            exit(1);
        }
    }
    string odata, reason;
    if (argc == 1) {
        const char *filename = *argv++; argc--;
        if (op_flags& OPT_p) {
            vector<string> tokens;
            TextSplit::stringToStrings(filename, tokens);
            for (vector<string>::const_iterator it = tokens.begin();
                 it != tokens.end(); it++) {
                cout << "[" << *it << "] ";
            }
            cout << endl;
            exit(0);
        }
        if (!strcmp(filename, "stdin")) {
            char buf[1024];
            int nread;
            while ((nread = read(0, buf, 1024)) > 0) {
                odata.append(buf, nread);
            }
        } else if (!file_to_string(filename, odata, &reason)) {
            cerr << "Failed: file_to_string(" << filename << ") failed: " 
                 << reason << endl;
            exit(1);
        }
    } else {
        if (op_flags & OPT_p)
            Usage();
        for (int i = 0; i < teststrings_cnt; i++) {
            cout << endl << teststrings[i] << endl;  
            dosplit(teststrings[i], flags, op_flags);
        }
        exit(0);
    }

    string& data = odata;
    string ndata;
    if ((op_flags & OPT_C)) {
        if (!transcode(odata, ndata, charset, "UTF-8")) {
            cerr << "Failed: transcode error" << endl;
            exit(1);
        } else {
            data = ndata;
        }
    }

    if (op_flags & OPT_c) {
        int n = TextSplit::countWords(data, flags);
        cout << n << " words" << endl;
    } else {
        dosplit(data, flags, op_flags);
    }    
}
