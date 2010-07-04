#!/usr/bin/env python
"""A python version of the command line query tool recollq (a bit simplified)
The input string is always interpreted as a query language string.
This could actually be useful for something after some customization
"""

import sys
from getopt import getopt
import recoll

allmeta = ("title", "keywords", "abstract", "url", "mimetype", "mtime",
           "ipath", "fbytes", "dbytes", "relevancyrating")

def Usage():
    print >> sys.stderr, "Usage: recollq.py [-c conf] [-i extra_index] <recoll query>"
    sys.exit(1);

def doquery(db, q):
    """Parse and execute query on open db"""
    # Get query object
    query = db.query()
    # Parse/run input query string
    nres = query.execute(q)

    # Print results:
    print "Result count: ", nres
    while query.next >= 0 and query.next < nres: 
        doc = query.fetchone()
        print query.next, ":",
        for k in ("title", "url"):
            print k, ":", getattr(doc, k).encode('utf-8')
        abs = db.makeDocAbstract(doc, query).encode('utf-8')
        print abs
        print


if len(sys.argv) < 2:
    Usage()

confdir=""
extra_dbs = []
# Snippet params
maxchars = 120
contextwords = 4
# Process options: [-c confdir] [-i extra_db [-i extra_db] ...]
options, args = getopt(sys.argv[1:], "c:i:")
for opt,val in options:
    if opt == "-c":
        confdir = val
    elif opt == "-i":
        extra_dbs.append(val)
    else:
        print >> sys.stderr, "Bad opt: ", opt
        Usage()

# The query should be in the remaining arg(s)
if len(args) == 0:
    print >> sys.stderr, "No query found in command line"
    Usage()
q = ""
for word in args:
    q += word + " "

print "QUERY: [", q, "]"
db = recoll.connect(confdir=confdir, 
                    extra_dbs=extra_dbs)
db.setAbstractParams(maxchars=maxchars, contextwords=contextwords)

doquery(db, q)


