#!/usr/bin/env python2
# Copyright (C) 2015 J.F.Dockes
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
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

# Transform XML output from xls-dump.py into csv format.
#
# Note: this would be difficult to make compatible with python 3 <=
# 3.4 because of the use of % interpolation on what should be bytes.
# # % terpolation for bytes is available as of python 3.5, which is
# the minimum version supported.


from __future__ import print_function

import sys
import xml.sax

dtt = True

if dtt:
    sepstring = b"\t"
    dquote = b""
else:
    sepstring = b","
    dquote = b'"'

class XlsXmlHandler(xml.sax.handler.ContentHandler):
    def __init__(self):
        self.output = b''
        
    def startElement(self, name, attrs):
        if name == "worksheet":
            if "name" in attrs:
                self.output += b"%s\n" % attrs["name"].encode("UTF-8")
        elif name == "row":
            self.cells = dict()
        elif name == "label-cell" or name == "number-cell":
            if "value" in attrs:
                value = attrs["value"].encode("UTF-8")
            else:
                value = b''
            if "col" in attrs:
                self.cells[int(attrs["col"])] = value
            else:
                #??
                self.output += b"%s%s" % (value.encode("UTF-8"), sepstring)
        elif name == "formula-cell":
            if "formula-result" in attrs and "col" in attrs:
                self.cells[int(attrs["col"])] = \
                             attrs["formula-result"].encode("UTF-8")
            
    def endElement(self, name, ):
        if name == "row":
            curidx = 0
            for idx, value in self.cells.items():
                self.output += sepstring * (idx - curidx)
                self.output += b"%s%s%s" % (dquote, value, dquote)
                curidx = idx
            self.output += b"\n"
        elif name == "worksheet":
            self.output += b"\n"


if __name__ == '__main__':
    try:
        handler = XlsXmlHandler()
        xml.sax.parse(sys.stdin, handler)
        print(handler.output)
    except BaseException as err:
        print("xml-parse: %s\n" % (str(sys.exc_info()[:2]),), file=sys.stderr)
        sys.exit(1)

    sys.exit(0)
