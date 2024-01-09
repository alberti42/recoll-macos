#!/usr/bin/env python3

# Tar-file filter for Recoll
# Thanks to Recoll user Martin Ziegler
# This is a modified version of /usr/share/recoll/filters/rclzip.py
# It works not only for tar-files, but automatically for gzipped and
# bzipped tar-files at well.

import os
import rclexecm
import rclnamefilter

try:
    import tarfile
except:
    print("RECFILTERROR HELPERNOTFOUND python3:tarfile")
    sys.exit(1);

class TarExtractor:
    def __init__(self, em):
        self.currentindex = 0
        self.em = em
        self.namen = []
        self.namefilter = rclnamefilter.NameFilter(em)

    def extractone(self, ipath):
        docdata = b''
        try:
            info = self.tar.getmember(ipath)
            if info.size > self.em.maxmembersize:
                # skip
                docdata = b''
                self.em.rclog("extractone: entry %s size %d too big" % (ipath, info.size))
                docdata = b'' # raise TarError("Member too big")
            else:
                docdata = self.tar.extractfile(ipath).read()
            ok = True
        except Exception as err:
            ok = False
        iseof = rclexecm.RclExecM.noteof
        if self.currentindex >= len(self.namen) -1:
            iseof = rclexecm.RclExecM.eofnext
        # We use fsencode, not makebytes, to convert to bytes. The latter would fail if the ipath
        # was actually binary, because it tries to encode to utf-8 but python3 had used fsdecode for
        # converting to str, and the result is not encodable to utf-8 (get: "surrogates
        # not allowed). We use fsdecode in getipath to revert the process.
        return (ok, docdata, os.fsencode(ipath), iseof)

    def closefile(self):
        self.tar = None

    def openfile(self, params):
        self.currentindex = -1
        filename = params["filename"]
        self.namefilter.setforlocation(filename)
        try:
            self.tar = tarfile.open(name=filename, mode='r')
            #self.namen = [ y.name for y in filter(lambda z:z.isfile(),self.tar.getmembers())]
            self.namen = [ y.name for y in [z for z in self.tar.getmembers() if z.isfile()]]
            return True
        except:
            return False

    def getipath(self, params):
        ipath = os.fsdecode(params["ipath"])
        ok, data, ipath, eof = self.extractone(ipath)
        if ok:
            return (ok, data, ipath, eof)
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
            if len(self.namen) == 0:
                self.closefile()
                eof = rclexecm.RclExecM.eofnext
            else:
                eof = rclexecm.RclExecM.noteof
            return (True, "", "", eof)

        while self.currentindex < len(self.namen):
            entryname = self.namen[self.currentindex]
            if self.namefilter.shouldprocess(entryname):
                break
            entryname = None
            self.currentindex += 1

        if entryname is None:
            self.namen=[]
            self.closefile()
            return (False, "", "", rclexecm.RclExecM.eofnow)

        ret = self.extractone(entryname)
        self.currentindex += 1
        if ret[3] != rclexecm.RclExecM.noteof:
            self.closefile()
        return ret


proto = rclexecm.RclExecM()
extract = TarExtractor(proto)
rclexecm.main(proto, extract)
