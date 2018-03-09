# Copyright (C) 2014 J.F.Dockes
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
######################################

# Helper module for xslt-based filters

from __future__ import print_function

import sys

PY2 = sys.version < '3'

if PY2:
    try:
        import libxml2
        import libxslt
        libxml2.substituteEntitiesDefault(1)
    except:
        print("RECFILTERROR HELPERNOTFOUND python:libxml2/python:libxslt1")
        sys.exit(1);
    def apply_sheet_data(sheet, data):
        styledoc = libxml2.parseMemory(sheet, len(sheet))
        style = libxslt.parseStylesheetDoc(styledoc)
        doc = libxml2.parseMemory(data, len(data))
        result = style.applyStylesheet(doc, None)
        res = style.saveResultToString(result)
        style.freeStylesheet()
        doc.freeDoc()
        result.freeDoc()
        return res
    def apply_sheet_file(sheet, fn):
        styledoc = libxml2.parseMemory(sheet, len(sheet))
        style = libxslt.parseStylesheetDoc(styledoc)
        doc = libxml2.parseFile(fn)
        result = style.applyStylesheet(doc, None)
        res = style.saveResultToString(result)
        style.freeStylesheet()
        doc.freeDoc()
        result.freeDoc()
        return res
else:
    try:
        from lxml import etree
    except:
        print("RECFILTERROR HELPERNOTFOUND python3:lxml")
        sys.exit(1);
    def apply_sheet_data(sheet, data):
        styledoc = etree.fromstring(sheet)
        transform = etree.XSLT(styledoc)
        doc = etree.fromstring(data)
        return bytes(transform(doc))
    def apply_sheet_file(sheet, fn):
        styledoc = etree.fromstring(sheet)
        transform = etree.XSLT(styledoc)
        doc = etree.parse(fn)
        return bytes(transform(doc))

