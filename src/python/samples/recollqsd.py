#!/usr/bin/env python
"""Example for using the ''searchdata''' structured query interface.
Not good for anything except showing/trying the code."""

import sys
import recoll

def dotest(db, q):
    query = db.query()
    query.sortby("title", 1)

    nres = query.executesd(q)
    print "Result count: ", nres
    if nres > 10:
        nres = 10
    while query.next >= 0 and query.next < nres: 
        doc = query.fetchone()
        print query.next
        for k in ("url", "mtime", "title", "author", "abstract"):
            print k, ":", getattr(doc, k).encode('utf-8')
            #abs = db.makeDocAbstract(doc, query).encode('utf-8')
            #print abs
        print
# End dotest

sd = recoll.SearchData()
sd.addclause("and", "essaouira maroc")
#sd.addclause("and", "dockes", field="author")
#sd.addclause("phrase", "jean francois", 1)
#sd.addclause("excl", "plage")

db = recoll.connect()
dotest(db, sd)

sys.exit(0)
