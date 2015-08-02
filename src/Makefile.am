
CXXFLAGS ?= @CXXFLAGS@
LIBXAPIAN=@LIBXAPIAN@
XAPIANCXXFLAGS=@XAPIANCXXFLAGS@
LIBICONV=@LIBICONV@
INCICONV=@INCICONV@
LIBFAM = @LIBFAM@
RCLLIBVERSION=@RCLLIBVERSION@
X_CFLAGS=@X_CFLAGS@
X_PRE_LIBS=@X_PRE_LIBS@
X_LIBS=@X_LIBS@
X_EXTRA_LIBS=@X_EXTRA_LIBS@
X_LIBX11=@X_LIBX11@
DEFS=@DEFS@

COMMONCPPFLAGS = -I. \
    -I$(top_srcdir)/aspell \
    -I$(top_srcdir)/bincimapmime \
    -I$(top_srcdir)/common \
    -I$(top_srcdir)/index \
    -I$(top_srcdir)/internfile \
    -I$(top_srcdir)/rcldb \
    -I$(top_srcdir)/unac \
    -I$(top_srcdir)/utils 

AM_CPPFLAGS = -Wall -Wno-unused \
    $(COMMONCPPFLAGS) \
    $(INCICONV) \
    $(XAPIANCXXFLAGS) \
    $(X_CFLAGS) \
    -DRECOLL_DATADIR=\"${pkgdatadir}\" \
    -D_GNU_SOURCE \
    $(DEFS)

ACLOCAL_AMFLAGS = -I m4

if NOTHREADS
    LIBTHREADS=
else
    LIBTHREADS= $(LIBSYSTHREADS)
endif

librcldir = $(libdir)/recoll
librcl_LTLIBRARIES = librecoll.la

librecoll_la_SOURCES = \
aspell/aspell-local.h \
aspell/rclaspell.cpp \
aspell/rclaspell.h \
bincimapmime/config.h \
bincimapmime/convert.cc \
bincimapmime/convert.h \
bincimapmime/iodevice.cc \
bincimapmime/iodevice.h \
bincimapmime/iofactory.cc \
bincimapmime/iofactory.h \
bincimapmime/mime-inputsource.h \
bincimapmime/mime-parsefull.cc \
bincimapmime/mime-parseonlyheader.cc \
bincimapmime/mime-printbody.cc \
bincimapmime/mime-utils.h \
bincimapmime/mime.cc \
bincimapmime/mime.h \
common/beaglequeuecache.cpp \
common/beaglequeuecache.h \
common/cstr.cpp \
common/cstr.h \
common/rclconfig.cpp \
common/rclconfig.h \
common/rclinit.cpp \
common/rclinit.h \
common/rclversion.h \
common/textsplit.cpp \
common/textsplit.h \
common/unacpp.cpp \
common/unacpp.h \
common/unordered_defs.h \
common/uproplist.h \
index/beaglequeue.cpp \
index/beaglequeue.h \
index/bglfetcher.cpp \
index/bglfetcher.h \
index/checkretryfailed.cpp \
index/checkretryfailed.h \
index/fetcher.cpp \
index/fetcher.h \
index/fsfetcher.cpp \
index/fsfetcher.h \
index/fsindexer.cpp \
index/fsindexer.h \
index/indexer.cpp \
index/indexer.h \
index/mimetype.cpp \
index/mimetype.h \
index/rclmon.h \
index/recollindex.h \
index/subtreelist.cpp \
index/subtreelist.h \
internfile/Filter.h \
internfile/extrameta.cpp \
internfile/extrameta.h \
internfile/htmlparse.cpp \
internfile/htmlparse.h \
internfile/indextext.h \
internfile/internfile.cpp \
internfile/internfile.h \
internfile/mh_exec.cpp \
internfile/mh_exec.h \
internfile/mh_execm.cpp \
internfile/mh_execm.h \
internfile/mh_html.cpp \
internfile/mh_html.h \
internfile/mh_mail.cpp \
internfile/mh_mail.h \
internfile/mh_mbox.cpp \
internfile/mh_mbox.h \
internfile/mh_symlink.h \
internfile/mh_text.cpp \
internfile/mh_text.h \
internfile/mh_unknown.h \
internfile/mimehandler.cpp \
internfile/mimehandler.h \
internfile/myhtmlparse.cpp \
internfile/myhtmlparse.h \
internfile/txtdcode.cpp \
internfile/uncomp.cpp \
internfile/uncomp.h \
query/docseq.cpp \
query/docseq.h \
query/docseqdb.cpp \
query/docseqdb.h \
query/docseqdocs.h \
query/docseqhist.cpp \
query/docseqhist.h \
query/dynconf.cpp \
query/dynconf.h \
query/filtseq.cpp \
query/filtseq.h \
query/plaintorich.cpp \
query/plaintorich.h \
query/recollq.cpp \
query/recollq.h \
query/reslistpager.cpp \
query/reslistpager.h \
query/sortseq.cpp \
query/sortseq.h \
query/wasaparse.h \
query/wasaparse.ypp \
query/wasaparseaux.cpp \
query/wasaparserdriver.h \
query/wasatorcl.h \
rcldb/daterange.cpp \
rcldb/daterange.h \
rcldb/expansiondbs.cpp \
rcldb/expansiondbs.h \
rcldb/rclabstract.cpp \
rcldb/rcldb.cpp \
rcldb/rcldb.h \
rcldb/rcldb_p.h \
rcldb/rcldoc.cpp \
rcldb/rcldoc.h \
rcldb/rcldups.cpp \
rcldb/rclquery.cpp \
rcldb/rclquery.h \
rcldb/rclquery_p.h \
rcldb/rclterms.cpp \
rcldb/searchdata.cpp \
rcldb/searchdata.h \
rcldb/searchdatatox.cpp \
rcldb/searchdataxml.cpp \
rcldb/stemdb.cpp \
rcldb/stemdb.h \
rcldb/stoplist.cpp \
rcldb/stoplist.h \
rcldb/synfamily.cpp \
rcldb/synfamily.h \
rcldb/termproc.h \
rcldb/xmacros.h \
unac/unac.cpp \
unac/unac.h \
unac/unac_version.h \
utils/appformime.cpp \
utils/appformime.h \
utils/base64.cpp \
utils/base64.h \
utils/cancelcheck.h \
utils/circache.cpp \
utils/circache.h \
utils/closefrom.cpp \
utils/closefrom.h \
utils/conftree.cpp \
utils/conftree.h \
utils/copyfile.cpp \
utils/copyfile.h \
utils/cpuconf.cpp \
utils/cpuconf.h \
utils/debuglog.cpp \
utils/debuglog.h \
utils/ecrontab.cpp \
utils/ecrontab.h \
utils/execmd.cpp \
utils/execmd.h \
utils/fileudi.cpp \
utils/fileudi.h \
utils/fstreewalk.cpp \
utils/fstreewalk.h \
utils/hldata.h \
utils/idfile.cpp \
utils/idfile.h \
utils/md5.cpp \
utils/md5.h \
utils/md5ut.cpp \
utils/md5ut.h \
utils/mimeparse.cpp \
utils/mimeparse.h \
utils/netcon.cpp \
utils/netcon.h \
utils/pathut.cpp \
utils/pathut.h \
utils/ptmutex.h \
utils/pxattr.cpp \
utils/pxattr.h \
utils/rclionice.cpp \
utils/rclionice.h \
utils/readfile.cpp \
utils/readfile.h \
utils/refcntr.h \
utils/smallut.cpp \
utils/smallut.h \
utils/strmatcher.cpp \
utils/strmatcher.h \
utils/transcode.cpp \
utils/transcode.h \
utils/utf8iter.h \
utils/wipedir.cpp \
utils/wipedir.h \
utils/workqueue.h \
utils/x11mon.cpp \
utils/x11mon.h

BUILT_SOURCES = query/wasaparse.cpp query/wasaparse.h

# We use -release: the lib is only shared
# between recoll programs from the same release.
# -version-info $(VERSION_INFO)
librecoll_la_LDFLAGS = -release $(VERSION) \
    -Wl,--no-undefined -Wl,--warn-unresolved-symbols

librecoll_la_LIBADD = $(LIBXAPIAN) $(LIBICONV) $(X_LIBX11) $(LIBTHREADS)

bin_PROGRAMS = recollindex

recollindex_SOURCES = \
                    index/recollindex.cpp \
                    index/rclmonprc.cpp \
                    index/rclmonrcv.cpp 

recollindex_LDFLAGS = 
recollindex_LDADD = librecoll.la

defconfdir = $(pkgdatadir)/examples
defconf_DATA = \
sampleconf/fragbuts.xml \
sampleconf/fields \
sampleconf/recoll.conf \
sampleconf/mimeconf \
sampleconf/recoll.qss \
sampleconf/mimemap \
sampleconf/mimeview 

filterdir = $(pkgdatadir)/filters
filter_DATA = filters/rcl* \
filters/ppt-dump.py \
filters/xls-dump.py \
filters/xlsxmltocsv.py \
filters/msodump.zip \
desktop/hotrecoll.py \
python/recoll/recoll/rclconfig.py 


install-data-hook: 
	(cd $(filterdir); \
	chmod a+x rcl* ppt-dump.py xls-dump.py xlsxmltocsv.py hotrecoll.py; \
	chmod 0644 msodump.zip rclexecm.py rcllatinstops.zip rclconfig.py) 

