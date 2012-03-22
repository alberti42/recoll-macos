NULL = 

EXTRA_DIST = \
  bin/unity-recoll-daemon.in \
  data/recoll.lens.in \
  data/recoll-lens.svg \
  data/unity-lens-recoll.service.in \
  $(docs_DATA) \
  $(NULL)

DISTCLEANFILES = 

DISTCHECK_CONFIGURE_FLAGS = 

######################################################
# Documents
######################################################
docsdir = $(datadir)/doc/unity-lens-recoll
docs_DATA = \
  AUTHORS \
  ChangeLog \
  COPYING \
  NEWS \
  README \
  $(NULL)

######################################################
# Lens code
######################################################
lensdir = $(datadir)/unity-lens-recoll/recollscope
lens_PYTHON = \
  recollscope/__init__.py \
  recollscope/rclsearch.py \
  $(NULL)

libexec_SCRIPTS = \
  bin/unity-recoll-daemon \
  $(NULL)

######################################################
# Unity .lens file
######################################################
dotlensdir = $(datadir)/unity/lenses/recoll
dotlens_DATA = \
  data/recoll.lens \
  $(NULL)

######################################################
# Icons
######################################################
iconsdir = $(datadir)/unity-lens-recoll
icons_DATA = \
  data/recoll-lens.svg \
  $(NULL)

######################################################
# DBus .service file
######################################################
dbusdir = $(datadir)/dbus-1/services
dbus_DATA = \
  data/unity-lens-recoll.service \
  $(NULL)
