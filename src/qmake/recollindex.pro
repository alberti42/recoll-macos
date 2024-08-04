
QT       -= core gui

TARGET = recollindex
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app

DEFINES += BUILDING_RECOLL

SOURCES += \
../index/indexer.cpp \
../index/checkindexed.cpp \
../index/checkindexed.h \
../index/checkretryfailed.cpp \
../index/fsindexer.cpp \
../index/fsindexer.h \
../index/rclmonprc.cpp \
../index/rclmonrcv.cpp \
../index/recollindex.cpp \
../index/webqueue.cpp \
../index/webqueue.h

INCLUDEPATH += ../common ../index ../internfile ../query ../unac ../utils ../aspell \
    ../rcldb ../qtgui ../xaposix ../confgui ../bincimapmime 

windows {
  DEFINES += UNICODE
  DEFINES += PSAPI_VERSION=1
  DEFINES += RCL_MONITOR
  DEFINES += __WIN32__

  contains(QMAKE_CC, cl){
    # MSVC
    QCBUILDLOC = Desktop_Qt_6_6_3_MSVC2019_64bit
    RECOLLDEPS = ../../../recolldeps/msvc
    DEFINES += USING_STATIC_LIBICONV
    PRE_TARGETDEPS = \
      ../build-librecoll-$$QCBUILDLOC-Release/release/recoll.lib
    LIBS += \
      -L../build-librecoll-$$QCBUILDLOC-Release/release -lrecoll \
      $$RECOLLDEPS/libxml2/libxml2-2.9.4+dfsg1/win32/bin.msvc/libxml2.lib \
      $$RECOLLDEPS/libxslt/libxslt-1.1.29/win32/bin.msvc/libxslt.lib \
      -L../build-libxapian-$$QCBUILDLOC-Release/release -llibxapian \
      $$RECOLLDEPS/libmagic/src/lib/libmagic.lib \
      $$RECOLLDEPS/regex/libregex.lib \
      $$RECOLLDEPS/zlib-1.2.11/zdll.lib \
      -L$$RECOLLDEPS/wlibiconv/build-libiconv-$$QCBUILDLOC-Release/release -liconv \
      -lShell32 -lrpcrt4 -lws2_32 -luser32 -lshlwapi -lpsapi -lkernel32
  }

  INCLUDEPATH += ../windows
  SOURCES += ../windows/getopt.cc
}

unix:!mac {
    QCBUILDLOC=Desktop
    SOURCES += \
      ../utils/execmd.cpp \
      ../utils/netcon.cpp \
      ../utils/rclionice.cpp
    PRE_TARGETDEPS = \
      ../build-librecoll-$$QCBUILDLOC-Release/librecoll.so
    LIBS += \
      -L../build-librecoll-$$QCBUILDLOC-Release/ -lrecoll \
      -lxapian -lxslt -lxml2 -lmagic -lz -lX11
}

mac {
  QCBUILDLOC=Qt_6_6_3_for_macOS
  QMAKE_APPLE_DEVICE_ARCHS = x86_64 arm64
  QMAKE_CXXFLAGS += -std=c++11 -pthread -Wno-unused-parameter
  DEFINES += RECOLL_AS_MAC_BUNDLE
  SOURCES += \
    ../utils/execmd.cpp \
    ../utils/netcon.cpp \
    ../utils/rclionice.cpp
  PRE_TARGETDEPS = $$PWD/build/librecoll/$$QCBUILDLOC-Release/librecoll.a
  LIBS += \
     $$PWD/build/librecoll/$$QCBUILDLOC-Release/librecoll.a \
     $$PWD/build/libxapian/$$QCBUILDLOC-Release/liblibxapian.a \
     -lxslt -lxml2 -liconv -lz
}

