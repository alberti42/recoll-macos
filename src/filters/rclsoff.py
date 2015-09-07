#!/usr/bin/env python

import sys
import os
import subprocess
import rclexecm
import rclxslt
from zipfile import ZipFile

stylesheet_meta = '''<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:office="urn:oasis:names:tc:opendocument:xmlns:office:1.0" 
  xmlns:xlink="http://www.w3.org/1999/xlink" 
  xmlns:dc="http://purl.org/dc/elements/1.1/" 
  xmlns:meta="urn:oasis:names:tc:opendocument:xmlns:meta:1.0" 
  xmlns:ooo="http://openoffice.org/2004/office"
  exclude-result-prefixes="office xlink meta ooo dc"
  >

<xsl:output method="html" encoding="UTF-8"/>

<xsl:template match="/office:document-meta">
  <xsl:apply-templates select="office:meta/dc:description"/>
  <xsl:apply-templates select="office:meta/dc:subject"/>
  <xsl:apply-templates select="office:meta/dc:title"/>
  <xsl:apply-templates select="office:meta/meta:keyword"/>
  <xsl:apply-templates select="office:meta/dc:creator"/>
</xsl:template>

<xsl:template match="dc:title">
<title> <xsl:value-of select="."/> </title><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="dc:description">
  <meta>
  <xsl:attribute name="name">abstract</xsl:attribute>
  <xsl:attribute name="content">
     <xsl:value-of select="."/>
  </xsl:attribute>
  </meta><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="dc:subject">
  <meta>
  <xsl:attribute name="name">keywords</xsl:attribute>
  <xsl:attribute name="content">
     <xsl:value-of select="."/>
  </xsl:attribute>
  </meta><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="dc:creator">
  <meta>
  <xsl:attribute name="name">author</xsl:attribute>
  <xsl:attribute name="content">
     <xsl:value-of select="."/>
  </xsl:attribute>
  </meta><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="meta:keyword">
  <meta>
  <xsl:attribute name="name">keywords</xsl:attribute>
  <xsl:attribute name="content">
     <xsl:value-of select="."/>
  </xsl:attribute>
  </meta><xsl:text>
</xsl:text>
</xsl:template>

</xsl:stylesheet>
'''

stylesheet_content  = '''<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:text="urn:oasis:names:tc:opendocument:xmlns:text:1.0"
  exclude-result-prefixes="text"
>

<xsl:output method="html" encoding="UTF-8"/>

<xsl:template match="text:p">
  <p><xsl:apply-templates/></p><xsl:text>
  </xsl:text>
</xsl:template>

<xsl:template match="text:h">
<p><xsl:apply-templates/></p><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="text:s">
<xsl:text> </xsl:text>
</xsl:template>

<xsl:template match="text:line-break">
<br />
</xsl:template>

<xsl:template match="text:tab">
<xsl:text>    </xsl:text>
</xsl:template>

</xsl:stylesheet>
'''

class OOExtractor:
    def __init__(self, em):
        self.em = em
        self.currentindex = 0

    def extractone(self, params):
        if not params.has_key("filename:"):
            self.em.rclog("extractone: no mime or file name")
            return (False, "", "", rclexecm.RclExecM.eofnow)
        fn = params["filename:"]

        try:
            zip = ZipFile(fn)
        except Exception as err:
            self.em.rclog("unzip failed: " + str(err))
            return (False, "", "", rclexecm.RclExecM.eofnow)

        docdata = '<html><head><meta http-equiv="Content-Type"' \
                  'content="text/html; charset=UTF-8"></head><body>'

        try:
            metadata = zip.read("meta.xml")
            if metadata:
                res = rclxslt.apply_sheet_data(stylesheet_meta, metadata)
                docdata += res
        except:
            # To be checked. I'm under the impression that I get this when
            # nothing matches?
            #self.em.rclog("no/bad metadata in %s" % fn)
            pass

        try:
            content = zip.read("content.xml")
            if content:
                res = rclxslt.apply_sheet_data(stylesheet_content, content)
                docdata += res
            docdata += '</body></html>'
        except Exception as err:
            self.em.rclog("bad data in %s" % fn)
            return (False, "", "", rclexecm.RclExecM.eofnow)

        return (True, docdata, "", rclexecm.RclExecM.eofnow)
    
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
    # Check for unzip
    if not rclexecm.which("unzip"):
        print("RECFILTERROR HELPERNOTFOUND unzip")
        sys.exit(1)
    proto = rclexecm.RclExecM()
    extract = OOExtractor(proto)
    rclexecm.main(proto, extract)
