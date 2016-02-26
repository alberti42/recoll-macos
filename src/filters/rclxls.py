#!/usr/bin/env python2

# Extractor for Excel files.
# Mso-dumper is not compatible with Python3. We use sys.executable to
# start the actual extractor, so we need to use python2 too.

import rclexecm
import rclexec1
import xlsxmltocsv
import re
import sys
import os
import xml.sax

class XLSProcessData:
    def __init__(self, em, ishtml = False):
        self.em = em
        self.out = ""
        self.gotdata = 0
        self.xmldata = ""
        self.ishtml = ishtml
        
    def takeLine(self, line):
        if self.ishtml:
            self.out += line + "\n"
            return
        if not self.gotdata:
            self.out += '''<html><head>''' + \
                        '''<meta http-equiv="Content-Type" ''' + \
                        '''content="text/html;charset=UTF-8">''' + \
                        '''</head><body><pre>'''
            self.gotdata = True
        self.xmldata += line

    def wrapData(self):
        if self.ishtml:
            return self.out
        handler =  xlsxmltocsv.XlsXmlHandler()
        data = xml.sax.parseString(self.xmldata, handler)
        self.out += self.em.htmlescape(handler.output)
        return self.out + '''</pre></body></html>'''

class XLSFilter:
    def __init__(self, em):
        self.em = em
        self.ntry = 0

    def reset(self):
        self.ntry = 0
        pass
            
    def getCmd(self, fn):
        if self.ntry:
            return ([], None)
        self.ntry = 1
        # Some HTML files masquerade as XLS
        try:
            data = open(fn, 'rb').read(512)
            if data.find('html') != -1 or data.find('HTML') != -1:
                return ("cat", XLSProcessData(self.em, True))
        except Exception as err:
            self.em.rclog("Error reading %s:%s" % (fn, str(err)))
            pass
        cmd = rclexecm.which("xls-dump.py")
        if cmd:
            # xls-dump.py often exits 1 with valid data. Ignore exit value
            return ([sys.executable, cmd, "--dump-mode=canonical-xml", \
                     "--utf-8", "--catch"],
                    XLSProcessData(self.em), rclexec1.Executor.opt_ignxval)
        else:
            return ([], None)

if __name__ == '__main__':
    if not rclexecm.which("xls-dump.py"):
        print("RECFILTERROR HELPERNOTFOUND ppt-dump.py")
        sys.exit(1)
    proto = rclexecm.RclExecM()
    filter = XLSFilter(proto)
    extract = rclexec1.Executor(proto, filter)
    rclexecm.main(proto, extract)
