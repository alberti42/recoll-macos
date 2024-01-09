#!/usr/bin/env python3

# 7-Zip file filter for Recoll

# Thanks to Recoll user Martin Ziegler
# This is a modified version of rclzip.py, with some help from rcltar.py
#
# Normally using py7zr https://github.com/miurahr/py7zr
#
# Else, but it does not work on all archives, may use:
#   Python pylzma library required. See http://www.joachim-bauch.de/projects/pylzma/ 

import sys
import os
import rclexecm
import rclnamefilter

usingpy7zr = False
try:
    from py7zr import SevenZipFile as Archive7z
    usingpy7zr = True
except:
    try:
        from py7zlib import Archive7z
    except:
        print("RECFILTERROR HELPERNOTFOUND python3:py7zr or python3:pylzma")
        sys.exit(1);

import rclconfig

class SevenZipExtractor:
    def __init__(self, em):
        self.currentindex = 0
        self.fp = None
        self.em = em
        self.namefilter = rclnamefilter.NameFilter(em)
            
    def extractone(self, ipath):
        #self.em.rclog("extractone: [%s]" % ipath)
        docdata = b''
        ok = False
        try:
            if usingpy7zr:
                docdata = self.sevenzdic[ipath].read()
            else:
                docdata = self.sevenzip.getmember(ipath).read()
            ok = True
        except Exception as err:
            self.em.rclog("extractone: failed: [%s]" % err)

        iseof = rclexecm.RclExecM.noteof
        if self.currentindex >= len(self.names) -1:
            iseof = rclexecm.RclExecM.eofnext
        return (ok, docdata, rclexecm.makebytes(ipath), iseof)

    def closefile(self):
        if self.fp:
            self.fp.close()
            
    ###### File type handler api, used by rclexecm ---------->
    def openfile(self, params):
        filename = params["filename"]
        self.currentindex = -1
        self.namefilter.setforlocation(filename)

        try:
            self.fp = open(filename, 'rb')
            self.sevenzip = Archive7z(self.fp)
            if usingpy7zr:
                self.sevenzdic = self.sevenzip.readall()
                self.names = [k[0] for k in self.sevenzdic.items()]
            else:
                self.names = self.sevenzip.getnames()
            return True
        except Exception as err:
            self.em.rclog("openfile: failed: [%s]" % err)
            return False

    def getipath(self, params):
        ipath = params["ipath"]
        ok, data, ipath, eof = self.extractone(ipath)
        if ok:
            return (ok, data, ipath, eof)
        # Not found. Maybe we need to decode the path?
        try:
            ipath = ipath.decode("utf-8")
            return self.extractone(ipath)
        except Exception as err:
            return (ok, data, ipath, eof)
        
    def getnext(self, params):
        if self.currentindex == -1:
            # Return "self" doc
            self.currentindex = 0
            self.em.setmimetype('text/plain')
            if len(self.names) == 0:
                self.closefile()
                eof = rclexecm.RclExecM.eofnext
            else:
                eof = rclexecm.RclExecM.noteof
            return (True, "", "", eof)

        while self.currentindex < len(self.names):
            entryname = self.names[self.currentindex]
            if self.namefilter.shouldprocess(entryname):
                break
            entryname = None
            self.currentindex += 1

        if entryname is None:
            self.closefile()
            return (False, "", "", rclexecm.RclExecM.eofnow)
                
        ret = self.extractone(entryname)
        self.currentindex += 1
        if ret[3] != rclexecm.RclExecM.noteof:
            self.closefile()
        return ret

# Main program: create protocol handler and extractor and run them
proto = rclexecm.RclExecM()
extract = SevenZipExtractor(proto)
rclexecm.main(proto, extract)
