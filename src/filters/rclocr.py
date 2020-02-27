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

# Running OCR programs for Recoll. This is excecuted from,
# e.g. rclpdf.py if pdftotext returns no data.
#
# The script tries to retrieve the data from the ocr cache, else it
# runs the configured OCR program and updates the cache. In both cases it writes
# the resulting text to stdout.

import os
import sys
import rclconfig
import rclocrcache
import importlib.util

def _deb(s):
    print("rclocr: %s" % s, file=sys.stderr)
    
def Usage():
    _deb("Usage: rclocr.py <imagefilename>")
    sys.exit(1)

if len(sys.argv) != 2:
    Usage()

path = sys.argv[1]

config = rclconfig.RclConfig()
cache = rclocrcache.OCRCache(config)

incache, data = cache.get(path)
if incache:
    sys.stdout.buffer.write(data)
    sys.exit(0)
    
#### Data not in cache

# Retrieve configured OCR program names and try to load the
# corresponding module
ocrprogs = config.getConfParam("ocrprogs")
if not ocrprogs:
    _deb("No ocrprogs variable in recoll configuration")
    sys.exit(1)
#_deb("ocrprogs: %s" % ocrprogs)
proglist = ocrprogs.split(" ")
ok = False
for ocrprog in proglist:
    try:
        modulename = "rclocr" + ocrprog
        ocr = importlib.import_module(modulename)
        if ocr.ocrpossible(config, path):
            ok = True
            break
    except Exception as err:
        _deb("While loading %s: got: %s" % (modulename, err))
        pass

if not ok:
    _deb("No OCR module could be loaded")
    sys.exit(1)

#_deb("Using ocr module %s" % modulename)

# The OCR module will retrieve its specific parameters from the
# configuration
status, data = ocr.runocr(config, path)

if not status:
    _deb("runocr failed")
    sys.exit(1)
    
cache.store(path, data)
sys.stdout.buffer.write(data)
sys.exit(0)

