#!/usr/bin/python3
#
# Test updating metadata from python and keeping a writable connection and multiple readonly
# connections open

import sys
import os
import threading
import time

from recoll import recoll
from recoll import fsudi

def msg(s):
    print(f"{s}", file=sys.stderr)
    
tstdata = os.environ["RECOLL_TESTDATA"]
fn = "pdf-xmp/Borensztein + Cavallo + Valenzuela - Debt Sustainability Under Catastrophic Risk.pdf"
confdir = os.environ["RECOLL_CONFDIR"]

path = os.path.join(tstdata, fn)
if not os.path.exists(path):
    raise Exception(path + "does not exist")

authorname = os.environ["AUTHORNAME"]

class Updater(threading.Thread):
    def run(self):
        udi = fsudi.fs_udi(path)
        db = recoll.connect(confdir, writable=1)
        doc = db.getDoc(udi)
        #msg(f"doc title: [{doc['title']}] author [{doc['author']}] sig [{doc['sig']}]")
        
        doc["author"] = authorname
        
        db.addOrUpdate(udi, doc, metaonly=1)
        msg("Metadata updated")
        db.close()
        # Keep the db open and linger
        db = recoll.connect(confdir, writable=1)
        for i in range(20):
            time.sleep(0.1)
        
class Queryer(threading.Thread):
    def run(self):
        time.sleep(1)
        db = recoll.connect(confdir, writable=0)
        for i in range(100):
            q = db.query()
            qstring = f"author:{authorname}"
            #msg(f"{self.name} : query: {qstring}")
            q.execute(qstring)
            cnt = 0
            for doc in q:
                cnt += 1
            #msg(f"{self.name}: got {cnt} results")
            if cnt != 1:
                msg(f"{self.name}: Query author:{authorname} returned {cnt} results (not 1)")
                raise Exception("")
            time.sleep(0.01)
        rank = int(self.name[-1])
        time.sleep(0.1*rank)
        print(f"{self.name} ran 100 queries")

updthr = Updater()
updthr.start()

rthreads = []
for i in range(3):
    rthreads.append(Queryer())
    rthreads[-1].start()

updthr.join()
for thr in rthreads:
    thr.join()
