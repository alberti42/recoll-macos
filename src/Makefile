# @(#$Id: Makefile,v 1.14 2005-11-12 14:31:39 dockes Exp $  (C) 2005 J.F.Dockes

# Default values. These can be overridden on the command line ie: 
# make PREFIX=/usr
PREFIX=/usr/local
INSTALL = install -c

all: mk/sysconf
	cd lib; ${MAKE}
	cd bincimapmime; ${MAKE}
	cd index; rm -f recollindex; ${MAKE} recollindex
	cd qtgui; qmake recoll.pro; rm -f recoll; \
	   cat ../mk/sysconf Makefile | \
	       ${MAKE} -f -

mk/sysconf:
	./configure

static:
	cd lib; ${MAKE}
	cd bincimapmime; ${MAKE}
	cd index; rm -f recollindex; \
	   ${MAKE} BSTATIC=-Wl,-Bstatic BDYNAMIC=-Wl,-Bdynamic
	cd qtgui; qmake recoll.pro; rm -f recoll \
	   cat ../mk/sysconf Makefile | ${MAKE} -f - \
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
	rm -f mk/sysconf 

install: all
	./recollinstall ${PREFIX}

installme:
	./recollinstall
