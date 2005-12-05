#ifndef lint
static char rcsid[] = "@(#$Id: docseq.cpp,v 1.3 2005-12-05 12:02:01 dockes Exp $ (C) 2005 J.F.Dockes";
#endif
#include <math.h>

#include "docseq.h"

bool DocSequenceDb::getDoc(int num, Rcl::Doc &doc, int *percent, string *sh)
{
    if (sh) sh->erase();
    return db ? db->getDoc(num, doc, percent) : false;
}

int DocSequenceDb::getResCnt()
{
    if (!db)
	return -1;
    // Need to fetch one document before we can get the result count 
    Rcl::Doc doc;
    int percent;
    db->getDoc(0, doc, &percent);
    return db->getResCnt();
}


bool DocSequenceHistory::getDoc(int num, Rcl::Doc &doc, int *percent, 
				string *sh) {
    // Retrieve history list
    if (!hist)
	return false;
    if (hlist.empty())
	hlist = hist->getDocHistory();

    if (num < 0 || num >= (int)hlist.size())
	return false;
    int skip;
    if (prevnum >= 0 && num >= prevnum) {
	skip = num - prevnum;
    } else {
	skip = num;
	it = hlist.begin();
	prevtime = -1;
    }
    prevnum = num;
    while (skip--) 
	it++;
    if (percent)
	*percent = 100;
    if (sh) {
	if (prevtime < 0 || abs(prevtime - (*it).unixtime) > 86400) {
	    prevtime = it->unixtime;
	    time_t t = (time_t)(it->unixtime);
	    *sh = string(ctime(&t));
	    // Get rid of the final \n in ctime
	    sh->erase(sh->length()-1);
	} else
	    sh->erase();
    }
    return db->getDoc((*it).fn, (*it).ipath, doc);
}

int DocSequenceHistory::getResCnt()
{	
    if (hlist.empty())
	hlist = hist->getDocHistory();
    return hlist.size();
}

