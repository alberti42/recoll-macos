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
    def __init__(self, em):
        self.em = em
        self.out = ""
        self.gotdata = 0
        self.xmldata = ""
        
    def takeLine(self, line):
        if not self.gotdata:
            self.out += '''<html><head>''' + \
                        '''<meta http-equiv="Content-Type" ''' + \
                        '''content="text/html;charset=UTF-8">''' + \
                        '''</head><body><pre>'''
            self.gotdata = True
        self.xmldata += line

    def wrapData(self):
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
        cmd = rclexecm.which("xls-dump.py")
        if cmd:
            # xls-dump.py often exits 1 with valid data. Ignore exit value
            return ([sys.executable, cmd, "--dump-mode=canonical-xml", \
                     "--utf-8", "--catch"],
                    XLSProcessData(self.em), rclexec1.Executor.opt_ignxval)
        else:
            return ([], None)

if __name__ == '__main__':
    if not rclexecm.which("ppt-dump.py"):
        print("RECFILTERROR HELPERNOTFOUND ppt-dump.py")
        sys.exit(1)
    proto = rclexecm.RclExecM()
    filter = XLSFilter(proto)
    extract = rclexec1.Executor(proto, filter)
    rclexecm.main(proto, extract)
