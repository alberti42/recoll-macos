# @(#$Id: Makefile,v 1.21 2005-12-06 15:10:36 dockes Exp $  (C) 2005 J.F.Dockes

# Default values. These can be overridden on the command line ie: 
# make prefix=/usr
prefix=/usr/local

all: mk/sysconf common/rclversion.h
	cd lib; ${MAKE}
	cd bincimapmime; ${MAKE}
	cd index; rm -f recollindex; ${MAKE} recollindex
	cd qtgui; qmake recoll.pro; rm -f recoll; \
	   cat ../mk/sysconf Makefile | \
	       ${MAKE} -f -

mk/sysconf:
	./configure

common/rclversion.h: VERSION
	(echo -n "static const char *rclversion= \"" && \
	 echo -n `cat VERSION` && \
	 echo "\";") > common/rclversion.h

static:
	cd lib; ${MAKE}
	cd bincimapmime; ${MAKE}
	cd index; rm -f recollindex; \
	   ${MAKE} BSTATIC=-Wl,-Bstatic BDYNAMIC=-Wl,-Bdynamic recollindex
	cd qtgui; qmake recoll.pro; rm -f recoll; \
	   cat ../mk/sysconf ../mk/localdefs Makefile | ${MAKE} -f - \
	       BSTATIC=-Wl,-Bstatic BDYNAMIC=-Wl,-Bdynamic

clean:
	cd common; ${MAKE} clean
	cd index; ${MAKE} clean
	cd lib; ${MAKE} clean
	cd bincimapmime; ${MAKE} clean
	cd query; ${MAKE} clean
	cd utils; ${MAKE} clean
	@-cd qtgui; ${MAKE} clean
	rm -f qtgui/Makefile qtgui/preview/Makefile qtgui/recoll

distclean: clean
	rm -f mk/sysconf mk/localdefs sampleconf/recoll.conf recollinstall

install: all
	./recollinstall ${prefix}

.PHONY: all static clean distclean install
