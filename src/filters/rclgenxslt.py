#!/usr/bin/env python3
# Copyright (C) 2018 J.F.Dockes
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
from __future__ import print_function

import sys
import rclexecm
import rclxslt
import gzip

class XSLTExtractor:
    def __init__(self, em, stylesheet, gzip=False):
        self.em = em
        self.currentindex = 0
        self.stylesheet = stylesheet
        self.dogz = gzip


    def extractone(self, params):
        if "filename:" not in params:
            self.em.rclog("extractone: no mime or file name")
            return (False, "", "", rclexecm.RclExecM.eofnow)
        fn = params["filename:"]
        try:
            if self.dogz:
                data = gzip.open(fn, 'rb').read()
            else:
                data = open(fn, 'rb').read()
            docdata = rclxslt.apply_sheet_data(self.stylesheet, data)
        except Exception as err:
            self.em.rclog("%s: bad data: %s" % (fn, err))
            return (False, "", "", rclexecm.RclExecM.eofnow)

        return (True, docdata, "", rclexecm.RclExecM.eofnext)
    

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
