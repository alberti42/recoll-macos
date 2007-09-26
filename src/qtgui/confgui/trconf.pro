TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on thread release debug

HEADERS	+= confgui.h

SOURCES	+= main.cpp confgui.cpp

#FORMS	= 

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj

  DEFINES += RECOLL_DATADIR=\"/usr/local/share/recoll\"
  LIBS += ../../lib/librcl.a -lz

  INCLUDEPATH += ../../common ../../utils
#../index ../internfile ../query ../unac \
#	      ../aspell ../rcldb
  POST_TARGETDEPS = ../../lib/librcl.a
}

UNAME = $$system(uname -s)
contains( UNAME, [lL]inux ) {
          LIBS -= -liconv
}

