#!/usr/bin/env python

import sys
import recollq
allmeta = ("title", "keywords", "abstract", "url", "mimetype", "mtime",
           "ipath", "fbytes", "dbytes", "relevance")


def dotest(db, q):
    query = db.query()
#query1 = db.query()

    nres = query.execute(q)
    print "Result count: ", nres
    if nres > 10:
        nres = 10
    while query.next >= 0 and query.next < nres: 
        doc = query.fetchone()
        print query.next
        for k in ("title",):
            print k, ":", getattr(doc, k).encode('utf-8')
            abs = db.makeDocAbstract(doc, query).encode('utf-8')
            print abs
            print

# End dotest

q = "essaouira"

print "TESTING WITH .recoll"
db = recollq.connect()
db.setAbstractParams(maxchars=80, contextwords=2)
dotest(db, q)

sys.exit(0)

print "TESTING WITH .recoll-test"
db = recollq.connect(confdir="/Users/dockes/.recoll-test")
dotest(db, q)

print "TESTING WITH .recoll-doc"
db = recollq.connect(confdir="/y/home/dockes/.recoll-doc")
dotest(db, q)

print "TESTING WITH .recoll and .recoll-doc"
db = recollq.connect(confdir="/Users/dockes/.recoll", 
                     extra_dbs=("/y/home/dockes/.recoll-doc",))
dotest(db, q)

