TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on thread release

SOURCES	+= main.cpp \
	idxthread.cpp

FORMS	= recollmain.ui

IMAGES	= images/filenew \
	images/fileopen \
	images/filesave \
	images/print \
	images/undo \
	images/redo \
	images/editcut \
	images/editcopy \
	images/editpaste \
	images/searchfind

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
  LIBS += ../lib/librcl.a ../bincimapmime/libmime.a -L/usr/local/lib -lxapian -liconv
  INCLUDEPATH += ../common ../index ../query ../unac ../utils 
}

UNAME = $$system(uname -s)
contains( UNAME, [lL]inux ) {
	  LIBS -= -liconv
}

