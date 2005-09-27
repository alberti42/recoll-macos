TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release

unix:LIBS	+= ../../lib/librcl.a ../../bincimapmime/libmime.a -L/usr/local/lib -lxapian -liconv -lfontconfig -lfreetype -lexpat -lz

unix:INCLUDEPATH	+= ../ ../../common ../../index ../../query ../../unac ../../utils

SOURCES	+= pvmain.cpp \
	../plaintorich.cpp

FORMS	= preview.ui

# 


unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
        }

