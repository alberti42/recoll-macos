#!/usr/bin/env python

# Python-based Image Tag extractor for Recoll. This is less thorough than the 
# Perl-based rclimg script, but useful if you don't want to have to install Perl
# (e.g. on Windows).
#
# Uses pyexiv2. Also tried Pillow, found it useless for tags.
#

import sys
import os
import rclexecm
import re

try:
    import pyexiv2
except:
    print "RECFILTERROR HELPERNOTFOUND python:pyexiv2"
    sys.exit(1);

khexre = re.compile('.*\.0[xX][0-9a-fA-F]+$')

pyexiv2_titles = {
    'Xmp.dc.subject',
    'Xmp.lr.hierarchicalSubject',
    'Xmp.MicrosoftPhoto.LastKeywordXMP',
    }

# Keys for which we set meta tags
meta_pyexiv2_keys = {
    'Xmp.dc.subject',
    'Xmp.lr.hierarchicalSubject',
    'Xmp.MicrosoftPhoto.LastKeywordXMP',
    'Xmp.digiKam.TagsList',
    'Exif.Photo.DateTimeDigitized',
    'Exif.Photo.DateTimeOriginal',
    'Exif.Image.DateTime',
    }

exiv2_dates = ['Exif.Photo.DateTimeOriginal',
               'Exif.Image.DateTime', 'Exif.Photo.DateTimeDigitized']

class ImgTagExtractor:
    def __init__(self, em):
        self.em = em
        self.currentindex = 0

    def extractone(self, params):
        #self.em.rclog("extractone %s" % params["filename:"])
        ok = False
        if not params.has_key("filename:"):
            self.em.rclog("extractone: no file name")
            return (ok, docdata, "", rclexecm.RclExecM.eofnow)
        filename = params["filename:"]

        try:
            metadata = pyexiv2.ImageMetadata(filename)
            metadata.read()
            keys = metadata.exif_keys + metadata.iptc_keys + metadata.xmp_keys
            mdic = {}
            for k in keys:
                # we skip numeric keys and undecoded makernote data
                if k != 'Exif.Photo.MakerNote' and not khexre.match(k):
                    mdic[k] = str(metadata[k].raw_value)
        except Exception, err:
            self.em.rclog("extractone: extract failed: [%s]" % err)
            return (ok, "", "", rclexecm.RclExecM.eofnow)

        docdata = "<html><head>\n"

        ttdata = set()
        for k in pyexiv2_titles:
            if k in mdic:
                ttdata.add(self.em.htmlescape(mdic[k]))
        if ttdata:
            title = ""
            for v in ttdata:
                v = v.replace('[', '').replace(']', '').replace("'", "")
                title += v + " "
            docdata += '<title>' + title + '</title>\n'

        for k in exiv2_dates:
            if k in mdic:
                # Recoll wants: %Y-%m-%d %H:%M:%S.
                # We get 2014:06:27 14:58:47
                dt = mdic[k].replace(':', '-', 2)
                docdata += '<meta name="date" content="' + dt + '">\n'
                break

        for k,v in mdic.iteritems():
            if k ==  'Xmp.digiKam.TagsList':
                docdata += '<meta name="keywords" content="' + \
                           self.em.htmlescape(mdic[k]) + '">\n'

        docdata += "</head><body>\n"
        for k,v in mdic.iteritems():
            docdata += k + " : " + self.em.htmlescape(mdic[k]) + "<br />\n"
        docdata += "</body></html>"

        self.em.setmimetype("text/html")

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

if __name__ == '__main__':
    proto = rclexecm.RclExecM()
    extract = ImgTagExtractor(proto)
    rclexecm.main(proto, extract)
