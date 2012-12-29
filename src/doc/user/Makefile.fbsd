# 
# @(#$Id: Makefile,v 1.1 2006-01-04 11:09:53 dockes Exp $
#
#

MAINTAINER=jfd@recoll.org

DOC?= usermanual
LANGCODE?= en_US.ISO_8859-1
FORMATS?= html txt html-split pdf
JADEFLAGS+=	-V %generate-article-toc%

# The purpose for this is to replace the FreeBSD file which contains the 
# blurb about file being hosted on FreeBSD.org. To work, it also
# needs a modification to /usr/doc/share/mk/doc.docbook.mk. Around line
# 121, replace LANGUAGECATALOG= with LANGUAGECATALOG?=
LANGUAGECATALOG=/usr/doc/share/sgml/catalog

#INSTALL_COMPRESSED?= gz
INSTALL_ONLY_COMPRESSED?=

# 
# SRCS lists the individual SGML files that make up the document. Changes
# to any of these files will force a rebuild
#

# SGML content
SRCS=  usermanual.sgml

# ${.CURDIR}/../../..
DOC_PREFIX?= /usr/doc
.include "${DOC_PREFIX}/share/mk/doc.project.mk"
