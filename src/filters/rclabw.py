#!/usr/bin/env python3
# Copyright (C) 2014 J.F.Dockes
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

stylesheet_all = '''<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:ab="http://www.abisource.com/awml.dtd" 
  exclude-result-prefixes="ab"
  >

<xsl:output method="html" encoding="UTF-8"/>

<xsl:template match="/">
<html>
  <head>
    <xsl:apply-templates select="ab:abiword/ab:metadata"/>
  </head>
  <body>

    <!-- This is for the older abiword format with no namespaces -->
    <xsl:for-each select="abiword/section">
      <xsl:apply-templates select="p"/>
    </xsl:for-each>

    <!-- Newer namespaced format -->
    <xsl:for-each select="ab:abiword/ab:section">
      <xsl:for-each select="ab:p">
        <p><xsl:value-of select="."/></p><xsl:text>
        </xsl:text>
      </xsl:for-each>
    </xsl:for-each>

  </body>
</html>
</xsl:template>

<xsl:template match="p">
  <p><xsl:value-of select="."/></p><xsl:text>
      </xsl:text>
</xsl:template>

<xsl:template match="ab:metadata">
    <xsl:for-each select="ab:m">
      <xsl:choose>
        <xsl:when test="@key = 'dc.creator'">
	  <meta>
	    <xsl:attribute name="name">author</xsl:attribute>
	    <xsl:attribute name="content">
	    <xsl:value-of select="."/>
	    </xsl:attribute>
          </meta><xsl:text>
	    </xsl:text>
        </xsl:when>
        <xsl:when test="@key = 'abiword.keywords'">
	  <meta>
	    <xsl:attribute name="name">keywords</xsl:attribute>
	    <xsl:attribute name="content">
	    <xsl:value-of select="."/>
	    </xsl:attribute>
          </meta><xsl:text>
	    </xsl:text>
        </xsl:when>
        <xsl:when test="@key = 'dc.subject'">
	  <meta>
	    <xsl:attribute name="name">keywords</xsl:attribute>
	    <xsl:attribute name="content">
	    <xsl:value-of select="."/>
	    </xsl:attribute>
          </meta><xsl:text>
	    </xsl:text>
        </xsl:when>
        <xsl:when test="@key = 'dc.description'">
	  <meta>
	    <xsl:attribute name="name">abstract</xsl:attribute>
	    <xsl:attribute name="content">
	    <xsl:value-of select="."/>
	    </xsl:attribute>
          </meta><xsl:text>
	    </xsl:text>
        </xsl:when>
        <xsl:when test="@key = 'dc.title'">
	  <title><xsl:value-of select="."/></title><xsl:text>
	    </xsl:text>
        </xsl:when>
        <xsl:otherwise>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
'''


class ABWExtractor:
    def __init__(self, em):
        self.em = em
        self.currentindex = 0

    def extractone(self, params):
        if "filename:" not in params:
            self.em.rclog("extractone: no mime or file name")
            return (False, "", "", rclexecm.RclExecM.eofnow)
        fn = params["filename:"]
        try:
            data = open(fn, 'rb').read()
            docdata = rclxslt.apply_sheet_data(stylesheet_all, data)
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


if __name__ == '__main__':
    proto = rclexecm.RclExecM()
    extract = ABWExtractor(proto)
    rclexecm.main(proto, extract)
