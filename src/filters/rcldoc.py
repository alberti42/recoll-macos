#!/usr/bin/env python

import rclexecm
import re
import sys
import os

# Processing the output from antiword: create html header and tail, process
# continuation lines escape HTML special characters, accumulate the data
class WordProcessData:
    def __init__(self, em):
        self.em = em
        self.out = ""
        self.cont = ""
        self.gotdata = False
        # Line with continued word (ending in -)
        # we strip the - which is not nice for actually hyphenated word.
        # What to do ?
        self.patcont = re.compile('''[\w][-]$''')
        # Pattern for breaking continuation at last word start
        self.patws = re.compile('''([\s])([\w]+)(-)$''')

    def takeLine(self, line):
        if not self.gotdata:
            if line == "":
                return
            self.out = '<html><head><title></title>' + \
                       '<meta http-equiv="Content-Type"' + \
                       'content="text/html;charset=UTF-8">' + \
                       '</head><body><p>'
            self.gotdata = True

        if self.cont:
            line = self.cont + line
            self.cont = ""

        if line == "\f":
            self.out += "</p><hr><p>"
            return

        if self.patcont.search(line):
            # Break at last whitespace
            match = self.patws.search(line)
            if match:
                self.cont = line[match.start(2):match.end(2)]
                line = line[0:match.start(1)]
            else:
                self.cont = line
                line = ""

        if line:
            self.out += self.em.htmlescape(line) + "<br>"
        else:
            self.out += "<br>"

    def wrapData(self):
        if self.gotdata:
            self.out += "</p></body></html>"
        self.em.setmimetype("text/html")
        return self.out

# Null data accumulator. We use this when antiword has fail, and the
# data actually comes from rclrtf, rcltext or vwWare, which all
# output HTML
class WordPassData:
    def __init__(self, em):
        self.out = ""
        self.em = em
    def takeLine(self, line):
        self.out += line
    def wrapData(self):
        self.em.setmimetype("text/html")
        return self.out
        
# Filter for msword docs. Try antiword, and if this fails, check for
# an rtf or text document (.doc are sometimes like this). Also try
# vwWare if the doc is actually a word doc
class WordFilter:
    def __init__(self, em, td):
        self.em = em
        self.ntry = 0
        self.thisdir = td
        
    def hasControlChars(self, data):
        for c in data:
            if c < chr(32) and c != '\n' and c != '\t' and \
                   c !=  '\f' and c != '\r':
                return True
        return False

    def mimetype(self, fn):
        rtfprolog ="{\\rtf1"
        docprolog = b"\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1"
        try:
            f = open(fn, "r")
        except:
            return ""
        data = f.read(100)
        if data[0:6] == rtfprolog:
            return "text/rtf"
        elif data[0:8] == docprolog:
            return "application/msword"
        elif self.hasControlChars(data):
            return "application/octet-stream"
        else:
            return "text/plain"

    def getCmd(self, fn):
        '''Return command to execute and postprocessor according to
        our state: first try antiword, then others depending on mime
        identification. Do 2 tries at most'''
        if self.ntry == 0:
            self.ntry = 1
            return (["antiword", "-t", "-i", "1", "-m", "UTF-8"],
                    WordPassData(self.em))
        elif self.ntry == 1:
            ntry = 2
            # antiword failed. Check for an rtf file, or text and
            # process accordingly. It the doc is actually msword, try
            # wvWare.
            mt = self.mimetype(fn)
            if mt == "text/plain":
                return ([os.path.join(self.thisdir,"rcltext")],
                       WordPassData(self.em))
            elif mt == "text/rtf":
                return ([os.path.join(self.thisdir, "rclrtf")],
                        WordPassData(self.em))
            elif mt == "application/msword":
                return (["wvWare", "--nographics", "--charset=utf-8"],
                        WordPassData(self.em))
            else:
                return ([],None)
        else:
            return ([],None)
            
if __name__ == '__main__':
    thisdir = os.path.dirname(sys.argv[0])
    proto = rclexecm.RclExecM()
    filter = WordFilter(proto, thisdir)
    extract = rclexecm.Executor(proto, filter)
    rclexecm.main(proto, extract)
