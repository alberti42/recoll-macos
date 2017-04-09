#!/usr/bin/env python2
# Copyright (C) 2016 J.F.Dockes
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
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
# 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

# Recoll DJVU extractor

from __future__ import print_function

import os
import sys
import re
import rclexecm
import subprocess

class DJVUExtractor:
    def __init__(self, em):
        self.currentindex = 0
        self.djvused = None
        self.djvutxt = None
        self.em = em

    def extractone(self, params):
        self.em.setmimetype('text/html')

        # Extract metadata
        if self.djvused:
            try:
                metadata = subprocess.check_output([self.djvused, self.filename,
                                                    "-e", "select 1;print-meta"])
            except Exception as e:
                self.em.rclog("djvused failed: %s" % e)
        author = ""
        title = ""
        metadata = metadata.decode('UTF-8', 'replace')
        for line in metadata.split('\n'):
            line = line.split('"')
            if len(line) >= 2:
                nm = line[0].strip()
                if nm == "author":
                    author = ' '.join(line[1:])
                elif nm == "title":
                    title = ' '.join(line[1:])

        # Main text
        try:
            txtdata = subprocess.check_output([self.djvutxt, "--escape", self.filename])
        except Exception as e:
            self.em.rclog("djvused failed: %s" % e)
            return (False, "", "", rclexecm.RclExecM.eofnow)
        txtdata = txtdata.decode('UTF-8', 'replace')

        data = '''<html><head><title>''' + self.em.htmlescape(title) + '''</title>'''
        data += '''<meta http-equiv="Content-Type" '''
        data += '''content="text/html;charset=UTF-8">'''
        if author:
            data += '''<meta name="author" content="''' + \
                    self.em.htmlescape(author) + '''">'''
        data += '''</head><body><pre>'''

        data += self.em.htmlescape(txtdata)
        data += '''</pre></body></html>'''
        return (True, data, "", rclexecm.RclExecM.eofnext)

    ###### File type handler api, used by rclexecm ---------->
    def openfile(self, params):
        self.filename = params["filename:"]
        self.currentindex = 0
        #self.em.rclog("openfile: [%s]" % self.filename)

        if not self.djvutxt:
            self.djvutxt = rclexecm.which("djvutxt")
            if not self.djvutxt:
                print("RECFILTERROR HELPERNOTFOUND djvutxt")
                sys.exit(1);
            self.djvused = rclexecm.which("djvused")

        return True

    def getipath(self, params):
        return self.extractone(params)
        return (ok, data, ipath, eof)
        
    def getnext(self, params):
        if self.currentindex >= 1:
            return (False, "", "", rclexecm.RclExecM.eofnow)
        else:
            ret= self.extractone(params)
            self.currentindex += 1
            return ret

# Main program: create protocol handler and extractor and run them
proto = rclexecm.RclExecM()
extract = DJVUExtractor(proto)
rclexecm.main(proto, extract)
