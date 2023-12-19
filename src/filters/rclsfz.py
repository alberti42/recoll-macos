#!/usr/bin/env python3
# Copyright (C) 2022 J.F.Dockes
#
# License: GPL 2.1
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

# Indexer for "SingleFileZ" files. These are single-file web archives stored in a file with a
# .zip.html extension, which manages to be both sort of an html and sort of a zip file. These could
# be processed just by adding .zip.html = application/zip in mimemap, but then we'd also index all
# the useless aux files in there. Use this filter to just extract the index.html

import rclexecm
from rclbasehandler import RclBaseHandler
import sys
from zipfile import ZipFile


class SFZExtractor(RclBaseHandler):
    def __init__(self, em):
        super(SFZExtractor, self).__init__(em)
    def html_text(self, fn):
        zipf = ZipFile(fn.decode('utf-8'))
        #self.em.rclog(f"ZipFile({fn}) ok")
        data = zipf.read("index.html")
        return data

if __name__ == '__main__':
    proto = rclexecm.RclExecM()
    extract = SFZExtractor(proto)
    rclexecm.main(proto, extract)
