#!/usr/bin/env python

import sys
import recoll
allmeta = ("title", "keywords", "abstract", "url", "mimetype", "mtime",
           "ipath", "fbytes", "dbytes", "relevancyrating")

def Usage():
    print >> sys.stderr, "Usage: recollq.py <recoll query>"
    sys.exit(1);
    
def dotest(db, q):
    query = db.query()

    nres = query.execute(q)
    print "Result count: ", nres
    while query.next >= 0 and query.next < nres: 
        doc = query.fetchone()
        print query.next, ":",
        for k in ("title", "url"):
            print k, ":", getattr(doc, k).encode('utf-8')
        abs = db.makeDocAbstract(doc, query).encode('utf-8')
        print abs
        print

# End dotest
if len(sys.argv) < 2:
    Usage()

q = ""
for word in sys.argv[1:]:
    q += word + " "

print "TESTING WITH .recoll, question: [" + q + "]"
db = recoll.connect()
db.setAbstractParams(maxchars=120, contextwords=4)
dotest(db, q)

sys.exit(0)

print "TESTING WITH .recoll-test"
db = recoll.connect(confdir="/Users/dockes/.recoll-test")
dotest(db, q)

print "TESTING WITH .recoll-doc"
db = recoll.connect(confdir="/y/home/dockes/.recoll-doc")
dotest(db, q)

print "TESTING WITH .recoll and .recoll-doc"
db = recoll.connect(confdir="/Users/dockes/.recoll", 
                     extra_dbs=("/y/home/dockes/.recoll-doc",))
dotest(db, q)

