#ifndef lint
static char rcsid[] = "@(#$Id: docseq.cpp,v 1.1 2005-11-25 10:02:36 dockes Exp $ (C) 2005 J.F.Dockes";
#endif
#include "docseq.h"

bool DocSequenceDb::getDoc(int num, Rcl::Doc &doc, int *percent) 
{
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


bool DocSequenceHistory::getDoc(int num, Rcl::Doc &doc, int *percent) {
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
    }
    prevnum = num;
    while (skip--) 
	it++;
    if (percent)
	*percent = 100;
    return db->getDoc((*it).first, (*it).second, doc);
}

int DocSequenceHistory::getResCnt()
{	
    if (hlist.empty())
	hlist = hist->getDocHistory();
    return hlist.size();
}

