#ifndef _recollq_h_included_
#define _recollq_h_included_
/* @(#$Id: recollq.h,v 1.1 2007-11-08 09:35:47 dockes Exp $  (C) 2007 J.F.Dockes */

/// Execute query, print results to stdout. This is just an api to the
/// recollq command line program.
class RclConfig;
extern int recollq(RclConfig **cfp, int argc, char **argv);

#endif /* _recollq_h_included_ */
