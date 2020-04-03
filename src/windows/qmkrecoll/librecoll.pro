#-------------------------------------------------
#
# Project created by QtCreator 2015-10-03T09:04:49
#
#-------------------------------------------------

QT       -= core gui

TARGET = librecoll
TEMPLATE = lib

DEFINES += LIBRECOLL_LIBRARY BUILDING_RECOLL
DEFINES += UNICODE
DEFINES += PSAPI_VERSION=1
DEFINES += READFILE_ENABLE_MINIZ
DEFINES += READFILE_ENABLE_MD5
DEFINES += READFILE_ENABLE_ZLIB
# VC only defines __WIN32, not __WIN32__ . For some reason xapian uses __WIN32__ which it actually defines in conf_post.h if __WIN32 is set. Reason: mystery.
DEFINES += __WIN32__

# This is necessary to avoid an undefined impl__xmlFree.
# See comment in libxml/xmlexports.h
DEFINES += LIBXML_STATIC

SOURCES += \
../../aspell/rclaspell.cpp \
../../bincimapmime/convert.cc \
../../bincimapmime/mime-parsefull.cc \
../../bincimapmime/mime-parseonlyheader.cc \
../../bincimapmime/mime-printbody.cc \
../../bincimapmime/mime.cc \
../../common/webstore.cpp \
../../common/cstr.cpp \
../../common/rclconfig.cpp \
../../common/rclinit.cpp \
../../common/syngroups.cpp \
../../common/textsplit.cpp \
../../common/unacpp.cpp \
../../common/utf8fn.cpp \
../../index/webqueue.cpp \
../../index/webqueuefetcher.cpp \
../../index/fetcher.cpp \
../../index/exefetcher.cpp \
../../index/fsfetcher.cpp \
../../index/fsindexer.cpp \
../../index/idxstatus.cpp \
../../index/indexer.cpp \
../../index/mimetype.cpp \
../../index/subtreelist.cpp \
../../internfile/extrameta.cpp \
../../internfile/htmlparse.cpp \
../../internfile/internfile.cpp \
../../internfile/mh_exec.cpp \
../../internfile/mh_execm.cpp \
../../internfile/mh_html.cpp \
../../internfile/mh_mail.cpp \
../../internfile/mh_mbox.cpp \
../../internfile/mh_text.cpp \
../../internfile/mh_xslt.cpp \
../../internfile/mimehandler.cpp \
../../internfile/myhtmlparse.cpp \
../../internfile/txtdcode.cpp \
../../internfile/uncomp.cpp \
../../query/docseq.cpp \
../../query/docseqdb.cpp \
../../query/docseqhist.cpp \
../../query/dynconf.cpp \
../../query/filtseq.cpp \
../../query/plaintorich.cpp \
../../query/recollq.cpp \
../../query/reslistpager.cpp \
../../query/sortseq.cpp \
../../query/wasaparse.cpp \
../../query/wasaparseaux.cpp \
../../rcldb/daterange.cpp \
../../rcldb/expansiondbs.cpp \
../../rcldb/rclabstract.cpp \
../../rcldb/rclabsfromtext.cpp \
../../rcldb/rcldb.cpp \
../../rcldb/rcldoc.cpp \
../../rcldb/rcldups.cpp \
../../rcldb/rclquery.cpp \
../../rcldb/rclterms.cpp \
../../rcldb/searchdata.cpp \
../../rcldb/searchdatatox.cpp \
../../rcldb/searchdataxml.cpp \
../../rcldb/stemdb.cpp \
../../rcldb/stoplist.cpp \
../../rcldb/synfamily.cpp \
../../rcldb/rclvalues.cpp \
../../rcldb/rclvalues.h \
../../unac/unac.cpp \
../../utils/appformime.cpp \
../../utils/base64.cpp \
../../utils/cancelcheck.cpp \
../../utils/chrono.cpp \
../../utils/circache.cpp \
../../utils/conftree.cpp \
../../utils/copyfile.cpp \
../../utils/cpuconf.cpp \
../../utils/dlib.cpp \
../../utils/ecrontab.cpp \
../../utils/utf8iter.cpp \
../../utils/zlibut.cpp \
../../utils/zlibut.h \
../../windows/execmd_w.cpp \
../../windows/fnmatch.c \
../../windows/wincodepages.cpp \
../../utils/fileudi.cpp \
../../utils/fstreewalk.cpp \
../../utils/hldata.cpp \
../../utils/idfile.cpp \
../../utils/log.cpp \
../../utils/md5.cpp \
../../utils/md5ut.cpp \
../../utils/mimeparse.cpp \
../../utils/miniz.cpp \
../../utils/pathut.cpp \
../../utils/pxattr.cpp \
../../utils/rclutil.cpp \
../../utils/readfile.cpp \
../../utils/smallut.cpp \
../../utils/strmatcher.cpp \
../../utils/transcode.cpp \
../../utils/wipedir.cpp \
../../windows/strptime.cpp

INCLUDEPATH += ../../common ../../index ../../internfile ../../query \
            ../../unac ../../utils ../../aspell ../../rcldb ../../qtgui \
            ../../xaposix ../../confgui ../../bincimapmime 

windows {
  contains(QMAKE_CC, gcc){
    # MingW
    QMAKE_CXXFLAGS += -std=c++11 -pthread -Wno-unused-parameter
    LIBS += C:/recolldeps/libxslt/libxslt-1.1.29/win32/bin.mingw/libxslt.a \
      C:/recolldeps/libxml2/libxml2-2.9.4+dfsg1/win32/bin.mingw/libxml2.a \
      c:/recolldeps/xapian-core-1.4.11/.libs/libxapian-30.dll \
      c:/recolldeps/zlib-1.2.8/zlib1.dll \
      -liconv -lshlwapi -lpsapi -lkernel32
    INCLUDEPATH += ../../windows \
      C:/recolldeps/xapian-core-1.4.15/include \
      C:/recolldeps/libxslt/libxslt-1.1.29/ \
      C:/recolldeps/libxml2/libxml2-2.9.4+dfsg1/include
    }
  contains(QMAKE_CC, cl){
    # Visual Studio
    LIBS += C:/users/bill/documents/recolldeps-vc/libxml2/libxml2-2.9.4+dfsg1/win32/bin.msvc/libxml2.lib \
      C:/users/bill/documents/recolldeps-vc/libxslt/libxslt-1.1.29/win32/bin.msvc/libxslt.lib \
      c:/users/bill/documents/recolldeps-vc/xapian-core-1.4.15/.libs/xapian.lib \
      c:/users/bill/documents/recolldeps-vc/zlib-1.2.11/zlib.lib \
      c:/users/bill/documents/recolldeps-vc/libiconv-for-windows/lib/libiconv.lib \
      -lshlwapi -lpsapi -lkernel32
    INCLUDEPATH += ../../windows \
      C:/users/bill/documents/recolldeps-vc/xapian-core-1.4.15/include \
      C:/users/bill/documents/recolldeps-vc/zlib-1.2.11/ \
      C:/users/bill/documents/recolldeps-vc/libxslt/libxslt-1.1.29/ \
     C:/users/bill/documents/recolldeps-vc/libxml2/libxml2-2.9.4+dfsg1/include \
     C:/users/bill/documents/recolldeps-vc/libiconv-for-windows/include
  }

}

unix {
    target.path = /usr/lib
    INSTALLS += target
}
