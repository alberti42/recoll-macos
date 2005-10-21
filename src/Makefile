
all: mk/sysconf
	cd lib; $(MAKE)
	cd bincimapmime; $(MAKE)
	cd index; $(MAKE)
	cd qtgui; qmake recoll.pro; rm -f recoll; $(MAKE)

mk/sysconf:
	./configure

static:
	rm -f index/recollindex qtgui/recoll
	cd lib; $(MAKE)
	cd bincimapmime; $(MAKE)
	cd index; $(MAKE) BSTATIC=-Wl,-Bstatic BDYNAMIC=-Wl,-Bdynamic
	cd qtgui; qmake recoll.pro; $(MAKE) BSTATIC=-Wl,-Bstatic BDYNAMIC=-Wl,-Bdynamic

clean:
	cd common; $(MAKE) clean
	cd index; $(MAKE) clean
	cd lib; $(MAKE) clean
	cd bincimapmime; $(MAKE) clean
	cd qtgui; rm -f recoll; $(MAKE) clean
	cd query; $(MAKE) clean
	cd utils; $(MAKE) clean
	rm -rf qtgui/Makefile qtgui/preview/Makefile
