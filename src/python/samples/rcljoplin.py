#!/usr/bin/python3
"""This sample uses the Recoll Python API to index  the notes table from a Joplin database"""

# This is an example of an indexer for which the embedded documents are standalone ones, not
# subdocuments of a main file. This is possible because the notes have update timestamps so that we
# can decide individually if they need a reindex. See rclmbox.py for an example with subdocs.
#
# Configuration: we need a separate recoll configuration and index (else all the main data will be
# purged after indexing the Joplin db...)
#
# Configuration files. Note that the mimeconf and mimeview additions, and the backends file need to
# exist for any index which would add the Joplin one as an external index.
#
# recoll.conf: just set it to index an empty directory
#
#   topdirs = ~/tmp/empty  # (or whatever)
#
# mimeconf: this tells recoll that the data from this indexer, to which we give a specific MIME
# type, should be turned into text by a null operation. We could also choose to format the notes in
# HTML in this script, in which case, change text/plain to text/html.
#
#    [index]
#    application/x-joplin-note = internal text/plain
#
# mimeview: this tells recoll to open notes by using the URL. Can't use the default entry
# because it badly encodes the URL (%U) for some reason which I can't remember.
#
#    xallexcepts+ = application/x-joplin-note
#    [view]
#    application/x-joplin-note = xdg-open %u
#
# backends: this is the link between recoll and this script. It needs to exist also in the main
# config directory if the joplin index is to be added as an external index. Fix the path of course,
# or copy the script somewhere in the PATH and use just the script name. Recoll will add parameters
# to the base commands.
#
#    [JOPLIN]
#    fetch = /path/to/rcljoplin.py fetch
#    makesig = /path/to/rcljoplin.py makesig
#

import sys
import os
import sqlite3

from recoll import recoll

def msg(s):
    print(f"{s}", file=sys.stderr)
    

class joplin_indexer:
    """The indexer classs. An object is created for indexing one joplin db"""
    def __init__(self, rcldb, sqfile):
        """Initialize for writable db recoll.Db object and joplin db connection"""
        self.rcldb = rcldb
        self.sqconn = sqlite3.connect(sqfile)

    def sig(self, note):
        """Create update verification value for note: updtime looks ok"""
        return str(note["updtime"])

    def sigfromid(self, id):
        #msg(f"sigfromid: {id}")
        c = self.sqconn.cursor()
        stmt = "SELECT updated_time FROM notes WHERE id = ?"
        c.execute(stmt, (id,))
        r = c.fetchone()
        if r:
            return self.sig({"updtime":r[0]})
        return ""
        
    def udi(self, note):
        """Create unique document identifier for message. This should
        be shorter than 150 bytes, which we optimistically don't check
        here, as we just use the note id (which we also assume to be globally unique)"""
        return note["id"]


    # Walk the table, check if index is up to date for each note, update the index if not
    def index(self):
        names = ["id", "title", "body", "updtime", "author"]
        c = self.sqconn.cursor()
        stmt = "SELECT id, title, body, updated_time, author FROM notes"
        c.execute(stmt)
        for r in c:
            note = dict((nm,val) for nm, val in zip(names, r))
            if not self.rcldb.needUpdate(self.udi(note), self.sig(note)):
                #msg(f"Index is up to date for {note['id']}")
                continue
            #msg(f"Indexing {str(note)[0:200]}")
            self.index_note(note)
        self.rcldb.purge()
        

    # Index a specific note
    def index_note(self, note):
        doc = recoll.Doc()

        # Misc standard recoll fields
        # it appears that the joplin updtime is in mS, we want secs
        doc.mtime = str(note["updtime"])[0:-3]
        doc.title = note["title"]
        doc.author = note["author"]

        # Main document text and MIME type
        doc.text = note["body"]
        doc.dbytes = str(len(doc.text.encode('UTF-8')))
        doc.mimetype = "application/x-joplin-note"
        
        # Store data for later "up to date" checks
        doc.sig = self.sig(note)
        
        # The rclbes field is the link between the index data and this
        # script when used at query time
        doc.rclbes = "JOPLIN"

        # These get stored inside the index, returned at query
        # time, and used for opening the note (as set in mimeview)
        doc.url = f"joplin://x-callback-url/openNote?id={note['id']}"

        # The udi is the unique document identifier, later used if we
        # want to e.g. delete the document index data (and other ops).
        udi = self.udi(note)

        self.rcldb.addOrUpdate(udi, doc)


    def getdata(self, sqfile, id):
        """Implements the 'fetch' data access interface (called at
        query time from the command line)."""
        c = self.sqconn.cursor()
        stmt = "SELECT body FROM notes WHERE id = ?"
        c.execute(stmt, (id,))
        r = c.fetchone()
        if r:
            return r[0]
        return ""


# Routine for indexing a Joplin notes table. Open the recoll index, call the actual
# indexer.
def index_joplin():
    rcldb = recoll.connect(confdir=rclconfdir, writable=1)
    indexer = joplin_indexer(rcldb, joplindb)
    indexer.index()

########
# Main program. This is called from cron or the command line for indexing, or called from the recoll
# main code with a specific command line for retrieving data or checking up-to-date-ness (for
# previewing mostly).

# Recoll recoll configuration/db directory 
# Here, we hard-code the value. It could be set in the environment instead.
# In the latter case, use a specific variable (e.g. RECOLL_JOPLIN_CONFDIR), *not* RECOLL_CONFDIR,
# which may be set for the main index (esp. at query time).
rclconfdir = os.path.expanduser("~/.recoll-joplin")

# We also hard-code the path to the joplin db for simplicity. It could also be set in the recoll
# configuration file and retrieved with something like the following,
#
#    from recoll import rclconfig
#    config = rclconfig.RclConfig(argcnf=rclconfdir)
#    joplindb = config.getConfParam("joplindbpath")
# Or any other approach...
joplindb = os.path.expanduser("~/.config/joplin-desktop/database.sqlite")
    
usage_string="""Usage:
rcljoplin.py
    Index the joplin notes table (the path to the sqlite db is hard-coded inside the script)
rcljoplin.py [fetch|makesig] <udi> <url> <ipath>
    fetch subdoc data or make signature (query time). ipath must be set but is ignored
"""

def usage():
    msg(f"{usage_string}")
    sys.exit(1)

if len(sys.argv) == 1:
    index_joplin()
else:
    # cmd [fetch|makesig] udi url ipath
    if len(sys.argv) != 5:
        usage()
    cmd = sys.argv[1]
    udi = sys.argv[2]
    url = sys.argv[3]
    
    # no need for an rcldb for getdata or makesig.
    fetcher = joplin_indexer(None, joplindb)

    if cmd == 'fetch':
        print(f"{fetcher.getdata(joplindb, udi)}", end="")
    elif cmd == 'makesig':
        print(f"{fetcher.sigfromid(udi)}", end="")
    else:
        usage()

sys.exit(0)
