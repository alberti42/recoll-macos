TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on thread release debug

SOURCES	+= main.cpp \
	idxthread.cpp \
	plaintorich.cpp

FORMS	= recollmain.ui \
	advsearch.ui \
	preview/preview.ui \
	sort.ui \
	uiprefs.ui

IMAGES	= images/filenew \
	images/fileopen \
	images/filesave \
	images/print \
	images/undo \
	images/redo \
	images/editcut \
	images/editcopy \
	images/editpaste \
	images/searchfind \
	images/asearch \
	images/history \
	images/nextpage \
	images/prevpage \
	images/sortparms

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
  DEFINES += RECOLL_DATADIR=\"$(RECOLL_DATADIR)\"
  LIBS += ../lib/librcl.a ../bincimapmime/libmime.a \
            $(BSTATIC) $(LIBXAPIAN) $(LIBICONV) $(BDYNAMIC) \
           -lz
  INCLUDEPATH += ../common ../index ../query ../unac ../utils 
  POST_TARGETDEPS = ../lib/librcl.a
}

UNAME = $$system(uname -s)
contains( UNAME, [lL]inux ) {
          LIBS -= -liconv
}

TRANSLATIONS = recoll_fr.ts
