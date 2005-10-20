all:
	test -f mk/sysconf || ./configure
	cd lib; $(MAKE)
	cd bincimapmime; $(MAKE)
	cd index; $(MAKE)
	cd qtgui; qmake recoll.pro; rm -f recoll; $(MAKE)

clean:
	cd common; $(MAKE) clean
	cd index; $(MAKE) clean
	cd lib; $(MAKE) clean
	cd bincimapmime; $(MAKE) clean
	cd qtgui; rm -f recoll; $(MAKE) clean
	cd query; $(MAKE) clean
	cd utils; $(MAKE) clean

