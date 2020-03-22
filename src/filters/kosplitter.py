#!/usr/bin/python3
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
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
########################################################

#
# Interface to the konlpy Korean text analyser: we receive text from
# our parent process and have it segmented by the analyser, then
# return the results. The analyser startup is very expensive (several
# seconds), which is why we can't just execute it from the main
# process.
#

import sys
import cmdtalk

from konlpy.tag import Okt

class Processor(object):
    def __init__(self, proto):
        self.proto = proto
        self.okt = Okt()

    def process(self, params):
        if 'data' not in params:
            return {'error':'No data field in parameters'}
        pos = self.okt.pos(params['data'])
        #proto.log("%s" % pos)
        output = ""
        for e in pos:
            if e[1] == 'Noun' or e[1] == 'Verb' or e[1] == 'Adjective' or \
               e[1] == 'Adverb':
                output += e[0] + '^'
        return {'data': output}

proto = cmdtalk.CmdTalk()
processor = Processor(proto)
proto.mainloop(processor)
