#ifndef 	lint
static char rcsid [] = "@(#$Id: trbinc.cc,v 1.1 2005-03-25 09:40:27 dockes Exp $  (C) 1994 CDKIT";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "mime.h"

static char *thisprog;

static char usage [] =
    "trbinc <mboxfile> \n\n"
    ;
static void
Usage(void)
{
    fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_s	  0x2 
#define OPT_b	  0x4 

#define DEFCOUNT 10

const char *hnames[] = {"Subject", "Content-type"};
int nh = sizeof(hnames) / sizeof(char *);

int main(int argc, char **argv)
{
    int count = DEFCOUNT;
    
    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
	(*argv)++;
	if (!(**argv))
	    /* Cas du "adb - core" */
	    Usage();
	while (**argv)
	    switch (*(*argv)++) {
	    case 's':	op_flags |= OPT_s; break;
	    case 'b':	op_flags |= OPT_b; if (argc < 2)  Usage();
		if ((sscanf(*(++argv), "%d", &count)) != 1) 
		    Usage(); 
		argc--; 
		goto b1;
	    default: Usage();	break;
	    }
    b1: argc--; argv++;
    }

    if (argc != 1)
	Usage();

    char *mfile = *argv++;argc--;
    int fd;
    if ((fd = open(mfile, 0)) < 0) {
	perror("Opening");
	exit(1);
    }
    Binc::MimeDocument doc;
    doc.parseFull(fd);

    if (!doc.isHeaderParsed() && !doc.isAllParsed()) {
	fprintf(stderr, "Parse error\n");
	exit(1);
    }
    close(fd);
    Binc::HeaderItem hi;
    for (int i = 0; i < nh ; i++) {
	if (!doc.h.getFirstHeader(hnames[i], hi)) {
	    fprintf(stderr, "No %s\n", hnames[i]);
	    exit(1);
	}
	printf("%s: %s\n", hnames[i], hi.getValue().c_str());
    }
    exit(0);
}
