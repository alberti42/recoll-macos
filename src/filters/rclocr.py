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

# Running OCR programs for Recoll

import os
import sys
import rclconfig
import rclocrcache
import importlib.util

def deb(s):
    print("%s" % s, file=sys.stderr)
    
def Usage():
    deb("Usage: rclocr.py <imagefilename>")
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

# Retrieve known ocr program names and try to load the corresponding module
ocrprogs = config.getConfParam("ocrprogs")
if not ocrprogs:
    deb("No ocrprogs variable")
    sys.exit(1)
deb("ocrprogs: %s" % ocrprogs)
proglist = ocrprogs.split(" ")
ok = False
for ocrprog in proglist:
    try:
        modulename = "rclocr" + ocrprog
        ocr = importlib.import_module(modulename)
        if ocr.ocrpossible(path):
            ok = True
            break
    except Exception as err:
        deb("While loading %s: got: %s" % (modulename, err))
        pass

if not ok:
    deb("No OCR module could be loaded")
    sys.exit(1)

deb("Using ocr module %s" % modulename)

data = ocr.runocr(config, path)

cache.store(path, data)
sys.stdout.buffer.write(data)
sys.exit(0)

