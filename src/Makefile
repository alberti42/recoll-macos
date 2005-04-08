all:
	cd lib;make
	cd bincimapmime;make
	cd index;make
	cd qtgui;qmake recoll.pro ; rm -f recoll;make
clean:
	cd common;make clean
	cd index;make clean
	cd lib;make clean
	cd bincimapmime;make clean
	cd qtgui;rm -f recoll;make clean
	cd query;make clean
	cd utils;make clean

