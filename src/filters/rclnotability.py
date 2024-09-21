#!/usr/bin/env python3

# Copyright (C) 2024 Alberti, Andrea
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
#
#
# Script name: rclnotability.py
# Author: Andrea Alberti

import sys
import rclexecm  # For communicating with recoll
from rclbasehandler import RclBaseHandler
from zipfile import ZipFile
try:
    import plistlib
except:
    print("RECFILTERROR HELPERNOTFOUND python3:plistlib")
    sys.exit(1);
try:
    import struct
except:
    print("RECFILTERROR HELPERNOTFOUND python3:struct")
    sys.exit(1);

####
_htmlprefix =b'''<html><head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
'''
####
_htmlmidfix = b'''</head><body><pre>
'''
####
_htmlsuffix = b'''
</pre></body></html>'''
####

HANDWRITINGINDEX="HandwritingIndex/index.plist"

def unpack_struct(value: bytes, unit: str = 'f') -> tuple:
    # Convert a string of bytes into either an array of 32-bit floats or ints.
    return struct.unpack(f'{len(value) // 4}{unit}', value)

class NotabilityHandler(RclBaseHandler):
    def __init__(self, em):
        super(NotabilityHandler, self).__init__(em)

    def parseNotabilityNote(self,filename,note):
        property_list_filename = None
        
        for name in note.namelist():
            if name.endswith(HANDWRITINGINDEX):
                property_list_filename = name
                break

        if property_list_filename == None:
            raise Exception(f"unable to find 'HandwritingIndex/index.plist' in {filename.decode("utf-8")}.")

        try:
            with note.open(property_list_filename, 'r') as session:
                property_list = plistlib.load(session)
        except Exception:
            raise Exception(f"unable to parse 'HandwritingIndex/index.plist' in {filename.decode("utf-8")}.")

        # Obtain the note title from the name of the folder containing the files
        note_title = property_list_filename[0:len(property_list_filename)-len(HANDWRITINGINDEX)-1]
                
        if not ("pages" in property_list and isinstance(property_list['pages'],dict)):
            raise Exception(f"list of 'pages' not found in {filename.decode("utf-8")}.")
        
        pages =  property_list['pages']

        try:
            pages_str = list(pages.keys())
            pages_num = [int(page) for page in pages_str]
            sorted_indexes=sorted(range(len(pages_num)), key=pages_num.__getitem__)
        except Exception:
            raise Exception(f"error in parsing page numbers in {filename.decode("utf-8")}.")

        pages_content = []

        for page_idx in sorted_indexes:
            page = pages[pages_str[page_idx]]
            if("text" in page):
               pages_content.append(page["text"])
        
        note_content = "\n\n".join(pages_content)

        # Start generating the HTML output
        output = _htmlprefix
        output += b'<meta name="title" content="' + \
              rclexecm.htmlescape(rclexecm.makebytes(note_title)) + b'">\n'
        output += _htmlmidfix
        output += rclexecm.htmlescape(note_content.encode('utf-8'))

        output += _htmlsuffix

        self.outputmimetype = "text/html"
            
        return output

    def html_text(self, filename):
        # self.em.log(f"Filename: {filename.decode('utf-8')}")
        f = None
        try:
            f = open(filename, 'rb')
            note = ZipFile(f)
            return self.parseNotabilityNote(filename, note)
        except Exception as err:
            #import traceback
            #traceback.print_exc()
            self.em.log("%s : %s" % (filename.decode('utf-8'), err))
            return (False, "", "", rclexecm.RclExecM.eofnow)
        finally:
            if f and not f.closed:
                f.close()

if __name__ == '__main__':
    proto = rclexecm.RclExecM()
    extract = NotabilityHandler(proto)
    rclexecm.main(proto, extract)
