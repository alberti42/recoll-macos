#!/usr/bin/python3
"""Using the Recoll Python API to index the notes table from a Joplin database"""

# This is an external indexer, not a Recoll document handler. It manages the documents inside the
# index all by itself. The documents are marked with a backend identifier (JOPLIN) so that the main
# indexer will leave them alone. See the manual section about the indexing API for more information.
#
# The Joplin notes are processed as standalone documents, not embedded ones (subdocuments of a main
# file). This is possible because the notes have update timestamps so that we can decide
# individually if they need a reindex. See rclmbox.py for an example with subdocs.
#
# Configuration files: Note that the mimeconf and mimeview additions, and the backends file need to
# exist for any index which would add the Joplin one as an external index.
#
# rcljoplin.py -c will take care to update the chosen configuration for you. Here follows a
# description anyway. As of Recoll 1.37.1, the data exists in the default configuration (no change
# needed, except for creating the "backends" file to activate the backend).
#
# mimeconf: this tells recoll that the data from this indexer, to which we give a specific MIME
# type, should be turned into text by a null operation. We could also choose to format the notes in
# HTML in this script, in which case, change text/plain to text/html.
#
#    [index]
#    application/x-joplin-note = internal text/plain
#
# mimeview: this tells recoll to open notes by using the URL (which was created by this indexer as a
# browser-appropriate link to a joplin note). Can't use the default entry because it badly encodes
# the URL (%U) for some reason which I can't remember.
#
#    xallexcepts+ = application/x-joplin-note
#    [view]
#    application/x-joplin-note = xdg-open %u
#
# backends: this is the link between recoll and this script. The rcljoplin.py script is no installed
# with the standard Recoll doc. handlers, so that there is no need for a path to the script. One
# could be used if the script was modified and stored elsewhere.
# Recoll will add parameters to the base commands.
#
#    [JOPLIN]
#    fetch = rcljoplin.py fetch
#    makesig = rcljoplin.py makesig
#    index = rcljoplin.py index
#
# By default the script looks for the Joplin db in ~/.config/joplin-desktop/database.sqlite
# This can be changed by setting "joplindbpath" in recoll.conf

import sys
import os
import sqlite3
from getopt import getopt
import signal

from recoll import recoll
from recoll import rclconfig
from recoll import conftree

def msg(s):
    print(f"{s}", file=sys.stderr)

# Signal termination handling: try to cleanly close the index
rcldb = None
def handler(signum, frame):
    logmsg(f"Got signal")
    if rcldb:
        rcldb.close()
    logmsg(f"Exiting")
    sys.exit(1)

signal.signal(signal.SIGINT, handler)
signal.signal(signal.SIGTERM, handler)

class joplin_indexer:
    def __init__(self, rclconfdir, sqfile):
        self.rclconfdir = rclconfdir
        self.sqconn = sqlite3.connect(sqfile)

    def _sig(self, note):
        """Create update verification value for note: updated_time looks ok"""
        return str(note["updated_time"])

    def sigfromid(self, id):
        #msg(f"sigfromid: {id}")
        c = self.sqconn.cursor()
        stmt = "SELECT updated_time FROM notes WHERE id = ?"
        c.execute(stmt, (id,))
        r = c.fetchone()
        if r:
            return self._sig({"updated_time":r[0]})
        return ""
        
    def _udi(self, note):
        """Create unique document identifier for message. This should
        be shorter than 150 bytes, which we optimistically don't check
        here, as we just use the note id (which we also assume to be globally unique)"""
        return note["id"]

    # Walk the table, check if the index is up to date for each note, update if needed.
    def index(self):
        global rcldb
        rcldb = recoll.connect(confdir=self.rclconfdir, writable=1)
        # Important: this marks all non-joplin documents as present. Else our call to purge() would
        # delete them. This can't happen in the recollindex process (because we're using our own db
        # instance).
        rcldb.preparePurge("JOPLIN")

        cols = ["id", "title", "body", "updated_time", "author"]
        c = self.sqconn.cursor()
        stmt = "SELECT " + ",".join(cols) + " FROM notes"
        c.execute(stmt)
        for r in c:
            note = dict((nm,val) for nm, val in zip(cols, r))
            if not rcldb.needUpdate(self._udi(note), self._sig(note)):
                #msg(f"Index is up to date for {note['id']}")
                continue
            #msg(f"Indexing {str(note)[0:200]}")
            self._index_note(rcldb, note)

        rcldb.purge()
        rcldb.close()
        
    # Index one note record
    def _index_note(self, rcldb, note):
        doc = recoll.Doc()

        # Misc standard recoll fields
        # it appears that the joplin updated_time is in mS, we want secs
        doc.mtime = str(note["updated_time"])[0:-3]
        doc.title = note["title"]
        doc.author = note["author"]

        # Main document text and MIME type
        doc.text = note["body"]
        doc.dbytes = str(len(doc.text.encode('UTF-8')))
        doc.mimetype = "application/x-joplin-note"
        
        # Store data for later "up to date" checks
        doc.sig = self._sig(note)
        
        # The rclbes field is the link between the index data and this
        # script when used at query time
        doc.rclbes = "JOPLIN"

        # These get stored inside the index, returned at query
        # time, and used for opening the note (as set in mimeview)
        doc.url = f"joplin://x-callback-url/openNote?id={note['id']}"

        # The udi is the unique document identifier, later used if we
        # want to e.g. delete the document index data (and other ops).
        udi = self._udi(note)

        rcldb.addOrUpdate(udi, doc)


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


# Update the current recoll configuration with the joplin bits.
def update_config(confdir):
    scriptpath = os.path.realpath(__file__)
    # mimeconf
    conf = conftree.ConfSimple(os.path.join(confdir, "mimeconf"), readonly=False)
    conf.set("application/x-joplin-note", "internal text/plain", "index")
    # mimeview
    conf = conftree.ConfSimple(os.path.join(confdir, "mimeview"), readonly=False)
    xalle = conf.get("xallexcepts+")
    if not xalle:
        xalle = "application/x-joplin-note"
    elif xalle.find("application/x-joplin-note") == -1:
        xalle += " application/x-joplin-note"
    conf.set("xallexcepts+", xalle)
    conf.set("application/x-joplin-note", "xdg-open %u", "view")
    # backends
    conf = conftree.ConfSimple(os.path.join(confdir, "backends"), readonly=False)
    conf.set("fetch", scriptpath + " fetch", "JOPLIN")
    conf.set("makesig", scriptpath + " makesig", "JOPLIN")
    conf.set("index", scriptpath + " index", "JOPLIN")



########
# Main program. This is called from cron or the command line for indexing, or called from the recoll
# main code with a specific command line for retrieving data, checking up-to-date-ness (for
# previewing mostly), or updating the index.

usage_string="""Usage:
rcljoplin.py config Update the recoll configuration to include the Joplin parameters.
rcljoplin.py index
    Index the joplin notes table (the path to the sqlite db is hard-coded inside the script)
rcljoplin.py <fetch|makesig> <udi> <url> <ipath>
    fetch subdoc data or make signature (query time). ipath must be set but is ignored
"""

# Note: the -c option is only for command line tests, the recoll process always sets the config in
# RECOLL_CONFDIR
def usage(f=sys.stderr):
    print(f"{usage_string}", file=f)
    sys.exit(1)

confdir = None
try:
    options, args = getopt(sys.argv[1:], "hc:", ["help", "config="])
except Exception as err:
    print(err, file=sys.stderr)
    usage()
for o,a in options:
    if o in ("-h", "--help"):
        usage(sys.stdout)
    elif o in ("-c", "--config"):
        confdir = a
        
if len(args) == 0:
    usage()
cmd = args[0]

rclconfig = rclconfig.RclConfig(argcnf=confdir)
confdir = rclconfig.getConfDir()

joplindb = rclconfig.getConfParam("joplindbpath")
if not joplindb:
    joplindb = os.path.expanduser("~/.config/joplin-desktop/database.sqlite")

if cmd == "index":
    indexer = joplin_indexer(confdir, joplindb)
    indexer.index()
    sys.exit(0)
elif cmd == "config":
    update_config(confdir)
    sys.exit(0)
        
# cmd [fetch|makesig] udi url ipath.
# We ignore url and ipath
if len(args) != 4:
    usage()

udi = sys.argv[2]

fetcher = joplin_indexer(confdir, joplindb)
if cmd == 'fetch':
    print(f"{fetcher.getdata(joplindb, udi)}", end="")
elif cmd == 'makesig':
    print(f"{fetcher.sigfromid(udi)}", end="")
else:
    usage()

sys.exit(0)
