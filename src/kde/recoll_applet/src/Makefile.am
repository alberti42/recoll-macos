INCLUDES= $(all_includes)
METASOURCES = AUTO

lib_LTLIBRARIES = recoll_applet_panelapplet.la

recoll_applet_panelapplet_la_SOURCES = recoll_applet.cpp kpixmapcombo.cpp
recoll_applet_panelapplet_la_LDFLAGS = -module -avoid-version $(all_libraries)
recoll_applet_panelapplet_la_LIBADD = $(LIB_KDEUI) 

recoll_applet_DATA = recoll_applet.desktop
recoll_appletdir = $(kde_datadir)/kicker/applets

messages: rc.cpp
	$(EXTRACTRC) `find . -name \*.ui -o -name \*.rc` > rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/recoll_applet.pot
noinst_HEADERS = kpixmapcombo.h
