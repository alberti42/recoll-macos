#!/usr/bin/env python3
# Copyright (C) 2016 J.F.Dockes
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the
# Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

# Base for extractor classes. With some common generic implementations
# for the boilerplate functions.

from __future__ import print_function

import os
import sys
import rclexecm

class RclBaseHandler(object):
    def __init__(self, em):
        self.em = em


    def extractone(self, params):
        #self.em.rclog("extractone %s %s" % (params["filename:"], \
        #params["mimetype:"]))
        if not "filename:" in params:
            self.em.rclog("extractone: no file name")
            return (False, "", "", rclexecm.RclExecM.eofnow)
        fn = params["filename:"]

        try:
            html = self.html_text(fn)
        except Exception as err:
            self.em.rclog("RclBaseDumper: %s : %s" % (fn, err))
            return (False, "", "", rclexecm.RclExecM.eofnow)

        self.em.setmimetype('text/html')
        return (True, html, "", rclexecm.RclExecM.eofnext)
        

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
