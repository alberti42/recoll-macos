# @(#$Id: Makefile,v 1.26 2006-01-19 12:01:42 dockes Exp $  (C) 2005 J.F.Dockes

all: mk/sysconf common/rclversion.h
	cd lib; ${MAKE}
	cd bincimapmime; ${MAKE}
	cd index; ${MAKE} recollindex
	cd qtgui; PATH=${PATH}:${QTDIR}/bin; export PATH; qmake recoll.pro; \
	 cat ../mk/sysconf Makefile | ${MAKE} -f - depth=..

mk/sysconf:
	./configure

common/rclversion.h: VERSION
	echo 'static const char *rclversion = "'`cat VERSION`'";' \
	   > common/rclversion.h

static:
	cd lib; ${MAKE}
	cd bincimapmime; ${MAKE}
	cd index; rm -f recollindex; \
	   ${MAKE} BSTATIC=-Wl,-Bstatic BDYNAMIC=-Wl,-Bdynamic recollindex
	cd qtgui; PATH=${PATH}:${QTDIR}/bin; export PATH; qmake recoll.pro; \
	   rm -f recoll;
	cat ../mk/sysconf Makefile | \
	   ${MAKE} -f - BSTATIC=-Wl,-Bstatic BDYNAMIC=-Wl,-Bdynamic depth=..

clean:
	cd common; ${MAKE} clean
	cd index; ${MAKE} clean
	cd lib; ${MAKE} clean
	cd bincimapmime; ${MAKE} clean
	cd query; ${MAKE} clean
	cd utils; ${MAKE} clean
        #cd doc/user; ${MAKE} clean
	@-cd qtgui; ${MAKE} clean
	rm -f qtgui/Makefile qtgui/preview/Makefile qtgui/recoll

distclean: clean
	rm -f mk/sysconf mk/localdefs sampleconf/recoll.conf \
	   recollinstall \
	   doc/user/*.html* doc/user/*.txt doc/user/HTML.manifest
	

install: all
	bindir=${bindir} datadir=${datadir} ./recollinstall ${prefix}

.PHONY: all static clean distclean install
