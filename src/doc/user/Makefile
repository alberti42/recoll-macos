


# Wherever docbook.xsl and chunk.xsl live.
# NOTE: THIS IS HARDCODED inside custom.xsl (for changing the output
# charset), which needs to change if the stylesheet location changes.
# Necessity of custom.xsl:
#   http://www.sagehill.net/docbookxsl/OutputEncoding.html

# Fbsd
#XSLDIR="/usr/local/share/xsl/docbook/"
# Mac
#XSLDIR="/opt/local/share/xsl/docbook-xsl/"
#Linux
XSLDIR="/usr/share/xml/docbook/stylesheet/docbook-xsl/"


# Options common to the single-file and chunked versions
commonoptions=--stringparam section.autolabel 1 \
  --stringparam section.autolabel.max.depth 2 \
  --stringparam section.label.includes.component.label 1 \
  --stringparam toc.max.depth 3 \
  --stringparam autotoc.label.in.hyperlink 0 \
  --stringparam abstract.notitle.enabled 1 \
  --stringparam html.stylesheet docbook-xsl.css \
  --stringparam generate.toc "book toc,title,figure,table,example,equation"


# index.html chunk format target replaced by nicer webhelp (needs separate
# make) in webhelp/ subdir
all: usermanual.html webh usermanual.pdf

webh:
	make -C webhelp
        
usermanual.html: usermanual.xml recoll.conf.xml
	xsltproc --xinclude ${commonoptions} \
            -o tmpfile.html custom.xsl $<
	-tidy -indent tmpfile.html > usermanual.html
	rm -f tmpfile.html

index.html: usermanual.xml recoll.conf.xml
	xsltproc ${commonoptions} \
            --stringparam use.id.as.filename 1 \
            --stringparam root.filename index \
            "${XSLDIR}/html/chunk.xsl" $<

usermanual.pdf: usermanual.xml recoll.conf.xml
	dblatex --xslt-opts="--xinclude" -tpdf $<

UTILBUILDS=/home/dockes/tmp/builds/medocutils/
recoll-conf-xml:
	$(UTILBUILDS)/confxml --docbook \
        --idprefix=RCL.INSTALL.CONFIG.RECOLLCONF  \
        ../../sampleconf/recoll.conf  > recoll.conf.xml

usermanual-rst: recoll-conf-xml
	tail -n +2 recoll.conf.xml > rcl-conf-tail.xml
	sed -e '/xi:include/r rcl-conf-tail.xml' \
		< usermanual.xml > full-man.xml
	sed -i -e '/xi:include/d' -e '/<?xml version="1.0"?>/d' full-man.xml
	test -d sphinx || mkdir sphinx
	pandoc -s --toc -f docbook -t rst full-man.xml  > sphinx/usermanual.rst
	(cd sphinx;make html)

clean:
	rm -f RCL.*.html usermanual.pdf usermanual.html index.html tmpfile.html
