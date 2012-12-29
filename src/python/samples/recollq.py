#!/usr/bin/env python
"""A python version of the command line query tool recollq (a bit simplified)
The input string is always interpreted as a query language string.
This could actually be useful for something after some customization
"""

import sys
from getopt import getopt

try:
    from recoll import recoll
    from recoll import rclextract
    hasextract = True
except:
    import recoll
    hasextract = False
    
allmeta = ("title", "keywords", "abstract", "url", "mimetype", "mtime",
           "ipath", "fbytes", "dbytes", "relevancyrating")

def Usage():
    print >> sys.stderr, "Usage: recollq.py [-c conf] [-i extra_index] <recoll query>"
    sys.exit(1);

class ptrmeths:
    def __init__(self, groups):
        self.groups = groups
    def startMatch(self, idx):
        ugroup = " ".join(self.groups[idx][1])
        return '<span class="pyrclstart" idx="%d" ugroup="%s">' % (idx, ugroup)
    def endMatch(self):
        return '</span>'
    
def extract(doc):
    extractor = rclextract.Extractor(doc)
    newdoc = extractor.textextract(doc.ipath)
    return newdoc

def extractofile(doc, outfilename=""):
    extractor = rclextract.Extractor(doc)
    outfilename = extractor.idoctofile(doc.ipath, doc.mimetype, \
                                       ofilename=outfilename)
    return outfilename

def doquery(db, q):
    # Get query object
    query = db.query()
    #query.sortby("dmtime", ascending=True)

    # Parse/run input query string
    nres = query.execute(q, stemming = 0, stemlang="english")
    qs = u"Xapian query: [%s]" % query.getxquery()
    print(qs.encode("utf-8"))
    groups = query.getgroups()
    print "Groups:", groups
    m = ptrmeths(groups)

    # Print results:
    print "Result count: ", nres, query.rowcount
    if nres > 20:
        nres = 20
    #results = query.fetchmany(nres)
    #for doc in results:

    for i in range(nres):
        doc = query.fetchone()
        rownum = query.next if type(query.next) == int else \
                 query.rownumber
        print rownum, ":",
        #for k,v in doc.items().items():
        #print "KEY:", k.encode('utf-8'), "VALUE", v.encode('utf-8')
        #continue
        #outfile = extractofile(doc)
        #print "outfile:", outfile, "url", doc.url.encode("utf-8")
        for k in ("title", "mtime", "author"):
            value = getattr(doc, k)
#            value = doc.get(k)
            if value is None:
                print k, ":", "(None)"
            else:
                print k, ":", value.encode('utf-8')
        #doc.setbinurl(bytearray("toto"))
        #burl = doc.getbinurl(); print "Bin URL :", doc.getbinurl()
        abs = query.makedocabstract(doc, methods=m)
        print abs.encode('utf-8')
        print
#        fulldoc = extract(doc)
#        print "FULLDOC MIMETYPE", fulldoc.mimetype, "TEXT:", fulldoc.text.encode("utf-8")


########################################### MAIN

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
