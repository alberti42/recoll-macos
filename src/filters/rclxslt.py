#!/usr/bin/env python

import sys

try:
    import libxml2
    import libxslt
except:
    print "RECFILTERROR HELPERNOTFOUND python:libxml2/python:libxslt1"
    sys.exit(1);

libxml2.substituteEntitiesDefault(1)

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

