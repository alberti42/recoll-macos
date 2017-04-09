#!/usr/bin/env python2

# Wrapping a text file. Recoll does it internally in most cases, but
# this is for use by another filter.

from __future__ import print_function

import rclexecm
import sys

class TxtDump:
    def __init__(self, em):
        self.em = em

    def extractone(self, params):
        #self.em.rclog("extractone %s %s" % (params["filename:"], \
        #params["mimetype:"]))
        if not "filename:" in params:
            self.em.rclog("extractone: no file name")
            return (False, "", "", rclexecm.RclExecM.eofnow)

        fn = params["filename:"]
        # No charset, so recoll will have to use its config to guess it
        txt = b'<html><head><title></title></head><body><pre>'
        try:
            f = open(fn, "rb")
            txt += self.em.htmlescape(f.read())
        except Exception as err:
            self.em.rclog("TxtDump: %s : %s" % (fn, err))
            return (False, "", "", rclexecm.RclExecM.eofnow)
            
        txt += b'</pre></body></html>'
        return (True, txt, "", rclexecm.RclExecM.eofnext)
        
    ###### File type handler api, used by rclexecm ---------->
    def openfile(self, params):
        self.currentindex = 0
        return True

    def getipath(self, params):
        return self.extractone(params)
        
    def getnext(self, params):
        if self.currentindex >= 1:
            return (False, "", "", rclexecm.RclExecM.eofnow)
        else:
            ret= self.extractone(params)
            self.currentindex += 1
            return ret

if __name__ == '__main__':
    proto = rclexecm.RclExecM()
    extract = TxtDump(proto)
    rclexecm.main(proto, extract)
