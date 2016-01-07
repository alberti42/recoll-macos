
01-2016:
Webhelp is not packaged on Debian. To setup the build:

 - Get a docbook-xsl tar distribution (e.g. docbook-xsl-1.79.1.tar.bz2)
 - Copy webhelp/xsl to the docbook dist:
   /usr/share/xml/docbook/stylesheet/docbook-xsl/webhelp/xsl
 - Possibly adjust
  DOCBOOK_DIST := /usr/share/xml/docbook/stylesheet/docbook-xsl/ 
  in Makefile (if other system with docbook installed elsewhere).

Did not try to get the search to work (needs lucene etc.)

