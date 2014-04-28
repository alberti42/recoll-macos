#!/bin/sh

# A script to produce the Recoll manual with an xml toolchain.
# Tools used:
#  - xsltproc
#  - The docbook-xsl styleets
#  - dblatex for producing the PDF.
#
# Limitations:
#   - Does not produce the links to the whole/chunked versions at the top
#     of the document
#   - The anchor names from the source text are converted to uppercase
#     by the sgml toolchain. This does not happen with the xml
#     toolchain, which means that external links like
#     usermanual.html#RCL.CONFIG.INDEXING won't work because fragments
#     are case-sensitive. This has been solved by converting all ids
#     inside the source file to upper-case. DON'T REINTRODUCE
#     lower-case IDS

# Wherever docbook.xsl and chunk.xsl live
# Fbsd
#XSLDIR="/usr/local/share/xsl/docbook/"
# Mac
#XSLDIR="/opt/local/share/xsl/docbook-xsl/"
#Linux
XSLDIR="/usr/share/xml/docbook/stylesheet/docbook-xsl/"

dochunky=1
test $# -eq 1 && dochunky=0

# Options common to the single-file and chunked versions
commonoptions="--stringparam section.autolabel 1 \
  --stringparam section.autolabel.max.depth 3 \
  --stringparam section.label.includes.component.label 1 \
  --stringparam autotoc.label.in.hyperlink 0 \
  --stringparam abstract.notitle.enabled 1 \
  --stringparam html.stylesheet docbook-xsl.css \
  --stringparam generate.toc \"book toc,title,figure,table,example,equation\" \
"

# Do the chunky thing
if test $dochunky -ne 0 ; then 
  eval xsltproc $commonoptions \
    --stringparam use.id.as.filename 1 \
    --stringparam root.filename index \
    "$XSLDIR/html/chunk.xsl" \
    usermanual.xml
fi

# Produce the single file version
eval xsltproc $commonoptions \
    -o usermanual.html \
    "$XSLDIR/html/docbook.xsl" \
    usermanual.xml

tidy -indent usermanual.html > tmpfile 
mv -f tmpfile usermanual.html

# And the pdf with dblatex
dblatex usermanual.xml 
