#!/usr/bin/env python3
#################################
# Copyright (C) 2020 J.F.Dockes
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the
#   Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
########################################################

# Caching OCR'd data

# OCR is extremely slow. The cache stores 2 kinds of objects:
# - Path files are named from the hash of the image path and contain
#   the image data hash and the modification time and size of the
#   image at the time the OCR'd data was stored in the cache
# - Data files are named with the hash of the image data and contain
#   the OCR'd data
# When retrieving data from the cache:
#  - We first use the image file size and modification time: if an
#    entry exists for the imagepath/mtime/size triplet, and is up to
#    date, the corresponding data is obtained from the data file and
#    returned.
#  - Else we then use the image data: if an entry exists for the
#    computed hashed value of the data, it is returned. This allows
#    moving files around without needing to run OCR again, but of
#    course, it is more expensive than the first step
#
#  If we need to use the second step, as a side effect, a path file is
#  created or updated so that the data will be found with the first
#  step next time around.

import sys
import os
import hashlib

def deb(s):
    print("%s" %s, file=sys.stderr)
    
class OCRCache(object):
    def __init__(self, conf):
        self.config = conf
        self.cachedir = conf.getConfParam("ocrcachedir")
        if not self.cachedir:
            self.cachedir = os.path.join(self.config.getConfDir(), "ocrcache")
        self.objdir = os.path.join(self.cachedir, "objects")
        if not os.path.exists(self.objdir):
            os.makedirs(self.objdir)

    # Compute sha1 of path, as two parts of 2 and 38 chars
    def _hashpath(self, data):
        if type(data) != type(b""):
            data = data.encode('utf-8')
            m = hashlib.sha1()
            m.update(data)
            h = m.hexdigest()
        return h[0:2], h[2:]

    # Compute sha1 of path data contents, as two parts of 2 and 38 chars
    def _hashdata(self, path):
        #deb("Hashing DATA")
        m = hashlib.sha1()
        with open(path, "rb") as f:
            while True:
                d = f.read(8192)
                if not d:
                    break
                m.update(d)
                h = m.hexdigest()
        return h[0:2], h[2:]

    # Try to read the stored attributes for a given path: data hash,
    # modification time and size. If this fails, the path itself is
    # not cached (but the data still might be, maybe the file was moved)
    def _cachedpathattrs(self, path):
        pd,pf = self._hashpath(path)
        o = os.path.join(self.objdir, pd, pf)
        if not os.path.exists(o):
            return False, None, None, None, None
        line = open(o, "r").read()
        dd,df,tm,sz = line.split()
        tm = int(tm)
        sz = int(sz)
        return True, dd, df, tm, sz

    # Compute the path hash, and get the mtime and size for given
    # path, for updating the cache path file
    def _newpathattrs(self, path):
        pd,pf = self._hashpath(path)
        tm = int(os.path.getmtime(path))
        sz = int(os.path.getsize(path))
        return pd, pf, tm, sz
    
    # Check if the cache appears up to date for a given path, only
    # using the modification time and size. Return the data file path
    # elements if we get a hit.
    def _pathincache(self, path):
        ret, od, of, otm, osz = self._cachedpathattrs(path)
        if not ret:
            return False, None, None
        pd, pf, ntm, nsz = self._newpathattrs(path)
        #deb(" tm %d  sz %d" % (ntm, nsz))
        #deb("otm %d osz %d" % (otm, osz))
        if otm != ntm or osz != nsz:
            return False, None, None
        return True, od, of

    # Check if cache appears up to date for path (no data check),
    # return True/False
    def pathincache(self, path):
        ret, dd, df = self._pathincache(path)
        return ret
    
    # Compute the data file name for path. Expensive: we compute the data hash.
    # Return both the data file path and path elements (for storage in path file)
    def _datafilename(self, path):
        d, f = self._hashdata(path)
        return os.path.join(self.objdir, d, f), d, f

    # Check if the data for path is in cache: expensive, needs to
    # compute the hash for the path's data contents. Returns True/False
    def dataincache(self, path):
        return os.path.exists(self._datafilename(path)[0])

    # Create path file with given elements.
    def _updatepathfile(self, pd, pf, dd, df, tm, sz):
        dir = os.path.join(self.objdir, pd)
        if not os.path.exists(dir):
            os.makedirs(dir)
        pfile = os.path.join(dir, pf)
        with open(pfile, "w") as f:
            f.write("%s %s %d %d\n" % (dd, df, tm, sz))

    # Store data for path. Only rewrite an existing data file if told
    # to do so: this is only useful if we are forcing an OCR re-run.
    def store(self, path, datatostore, force=False):
        dd,df = self._hashdata(path)
        pd, pf, tm, sz = self._newpathattrs(path)
        self._updatepathfile(pd, pf, dd, df, tm, sz)
        dir = os.path.join(self.objdir, dd)
        if not os.path.exists(dir):
            os.makedirs(dir)
        dfile = os.path.join(dir, df)
        if force or not os.path.exists(dfile):
            #deb("Storing data")
            with open(dfile, "wb") as f:
                f.write(datatostore)
        return True

    # Retrieve cached OCR'd data for image path. Possibly update the
    # path file as a side effect (case where the image has moved, but
    # the data has not changed).
    def get(self, path):
        pincache, dd, df = self._pathincache(path)
        if pincache:
            dfn = os.path.join(self.objdir, dd, df)
        else:
            dfn, dd, df = self._datafilename(path)

        if not os.path.exists(dfn):
            return False, b""

        if not pincache:
            # File has moved. create/Update path file for next time
            pd, pf, tm, sz = self._newpathattrs(path)
            self._updatepathfile(pd, pf, dd, df, tm, sz)

        return True, open(dfn, "rb").read()



if __name__ == '__main__':
    import rclconfig

    conf = rclconfig.RclConfig()
    cache = OCRCache(conf)
    path = sys.argv[1]
    deb("Using %s" % path)
    
    deb("== CACHE tests")
    ret = cache.pathincache(path)
    s = "" if ret else " not"
    deb("path for %s%s in cache" % (path, s))

    #ret = cache.dataincache(path)
    #s = "" if ret else " not"
    #deb("data for %s%s in cache" % (path, s))

    if False:
        deb("== STORE tests")
        cache.store(path, b"my OCR'd text is one line\n", force=False)

    deb("== GET tests")
    incache, data = cache.get(path)
    if incache:
        deb("Data from cache [%s]" % data)
    else:
        deb("Data was not found in cache")
        
