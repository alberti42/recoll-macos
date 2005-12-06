# New ports collection makefile for:   recoll
# Date created:        5 December 2005
# Whom:                J.F. Dockes
#
# $FreeBSD$
#

PORTNAME=      recoll
PORTVERSION=   1.0.12
CATEGORIES=    deskutils
MASTER_SITES=  http://www.lesbonscomptes.com/recoll/

MAINTAINER=    jean-francois.dockes@wanadoo.fr
COMMENT=       A personal full text search package, based on QT and Xapian

LIB_DEPENDS=   xapian.8:${PORTSDIR}/databases/xapian-core \
	       qt-mt.3:${PORTSDIR}/x11-toolkits/qt33

GNU_CONFIGURE= yes
USE_GMAKE= yes

.include <bsd.port.mk>
