TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on thread release

SOURCES	+= main.cpp \
	idxthread.cpp

FORMS	= recollmain.ui \
	advsearch.ui

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
    }

UNAME = $$system(uname -s)
contains( UNAME, [lL]inux ) {
	  }

