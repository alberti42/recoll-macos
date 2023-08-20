#!/usr/bin/python3
# Copyright (C) 2023 J.F.Dockes
#
# License: GPL 2.1
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#
# Report the longest documents (by term count), and the biggest directories (by term counts also)

import sys
import os

import xapian
import rclconfig

# Number of biggest files we show
nfilesprinted = 10
# Depth of paths for which we compute occupation. e.g. /home/me/doc/science would be at depth 4
dirdepth = 4
# Number of directories for which we print the totals
ndirsprinted = 20

def msg(s):
    print(f"{s}", file=sys.stderr)
    
# Retrieve named value(s) from a recoll document data record.
# The record format is a sequence of nm=value lines
def get_attributes(xdb, xdocid, flds, decode=True):
    #msg(f"get_attributes xdocid {xdocid} flds {flds} decode {decode}")
    doc = xdb.get_document(xdocid)
    data = doc.get_data()
    res = []
    for fld in flds:
        s = data.find(fld + b"=")
        if s == -1:
            res.append(None)
        else:
            e = data.find(b"\n", s)
            if decode:
                res.append(data[s+len(fld)+1:e].decode('UTF-8'))
            else:
                res.append(data[s+len(fld)+1:e])
    return res

# Sample code. Not called and does nothing at the moment. Sample code for looking at doc terms in
# more detail
def doc_details(xdb, xdocid):
    #xdoc = xdb.get_document(xdocid)
    #for term in xdoc.termlist():
        #msg(f"TERM {term.term}")
        #for position in xdb.positionlist(xdocid, term.term):
            #poscount +=1
    #msg(f"Positions count {poscount}")
    pass
    

if __name__ == '__main__':
    if len(sys.argv) != 1:
        msg("Usage: rcldocsbytermcount.py")
        msg(" Set RECOLL_CONFDIR to use a non-default configuration")
        sys.exit(1)

    config = rclconfig.RclConfig()
    dbdir = config.getDbDir()
    xdb = xapian.Database(dbdir)

    alldoclengths = []

    # Walk the Xapian index whole document list (postlist for empty term). Take note of the document
    # term counts.
    for doc in xdb.postlist(""):
        xdocid = int(doc.docid)
        if xdocid % 10000 == 0:
            msg(f"xdocid {xdocid}")
        try:
            url = get_attributes(xdb, xdocid, [b"url",], decode=False)[0]
        except Exception as ex:
            msg(f"get_attributes failed: {ex}")
            continue
        alldoclengths.append((doc.doclength, url))

    # Sort by document term count, highest first
    alldoclengths.sort(reverse=True)

    # Walk the sorted list, print the "nfilesprinted" longest documents sizes and paths, accumulate
    # the sizes for directories at depth "dirdepth"
    dirsizes = {}
    i = 0
    for e in alldoclengths:
        path = e[1][7:]
        if i < nfilesprinted:
            print(f"{e[0]} {path}")
            i += 1
        path = os.path.dirname(path)
        #print(f"PATH {path}")
        l = path.split(b"/")
        if len(l) < dirdepth:
            dir = path
        else:
            dir = b"/".join(l[0:dirdepth+1])
        #print(f"DIR {dir}")
        if dir in dirsizes:
            dirsizes[dir] += e[0]
        else:
            dirsizes[dir] = e[0]

    # Print the 20 biggest directories
    print("\nDirectory sizes:")
    dirsizes  = sorted(dirsizes.items(), key = lambda entry: entry[1], reverse = True)
    for dir, sz in dirsizes[0:ndirsprinted]:
        print(f"{sz} {dir}")
