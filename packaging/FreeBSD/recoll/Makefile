# New ports collection makefile for:   recoll
# Date created:        5 November 2005
# Whom:                J.F. Dockes
#
# $FreeBSD$
#

PORTNAME=      recoll
PORTVERSION=   1.0.5
CATEGORIES=    deskutils
MASTER_SITES=  http://www.lesbonscomptes.com/recoll/

MAINTAINER=    jean-francois.dockes@wanadoo.fr
COMMENT=       A personal xapian-based text search utility, with a qt GUI

USE_GMAKE= yes
LIB_DEPENDS=   xapian.8:${PORTSDIR}/databases/xapian-core \
	       qt-mt.3:${PORTSDIR}/x11-toolkits/qt33

.include <bsd.port.mk>
