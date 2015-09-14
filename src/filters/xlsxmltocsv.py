#!/usr/bin/env python

import sys
import xml.sax
sys.path.append(sys.path[0]+"/msodump.zip")
from msodumper.globals import error

dtt = True

if dtt:
    sepstring = "\t"
    dquote = ''
else:
    sepstring = ","
    dquote = '"'
    
class XlsXmlHandler(xml.sax.handler.ContentHandler):
    def __init__(self):
        self.output = ""
        
    def startElement(self, name, attrs):
        if name == "worksheet":
            if "name" in attrs:
                self.output += "%s\n" % attrs["name"].encode("UTF-8")
        elif name == "row":
            self.cells = dict()
        elif name == "label-cell" or name == "number-cell":
            if "value" in attrs:
                value = attrs["value"].encode("UTF-8")
            else:
                value = unicode()
            if "col" in attrs:
                self.cells[int(attrs["col"])] = value
            else:
                #??
                self.output += "%s%s" % (value.encode("UTF-8"), sepstring)
        elif name == "formula-cell":
            if "formula-result" in attrs and "col" in attrs:
                self.cells[int(attrs["col"])] = \
                             attrs["formula-result"].encode("UTF-8")
            
    def endElement(self, name, ):
        if name == "row":
            curidx = 0
            for idx, value in self.cells.iteritems():
                self.output += sepstring * (idx - curidx)
                self.output += "%s%s%s" % (dquote, value, dquote)
                curidx = idx
            self.output += "\n"
        elif name == "worksheet":
            self.output += "\n"


if __name__ == '__main__':
    try:
        handler = XlsXmlHandler()
        xml.sax.parse(sys.stdin, handler)
        print(handler.output)
    except BaseException as err:
        error("xml-parse: %s\n" % (str(sys.exc_info()[:2]),))
        sys.exit(1)

    sys.exit(0)
