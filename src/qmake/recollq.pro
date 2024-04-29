
QT       -= core gui

TARGET = recollq
CONFIG += console c++17
CONFIG -= app_bundle
TEMPLATE = app

DEFINES += BUILDING_RECOLL

SOURCES += \
../query/recollqmain.cpp

INCLUDEPATH += ../common ../index ../internfile ../query ../unac ../utils ../aspell \
    ../rcldb ../qtgui ../xaposix ../confgui ../bincimapmime 

windows {
  DEFINES += UNICODE
  DEFINES += PSAPI_VERSION=1
  DEFINES += __WIN32__

  contains(QMAKE_CC, cl){
    # Visual Studio
    QCBUILDLOC = Desktop_Qt_6_6_3_MSVC2019_64bit
    RECOLLDEPS = ../../../recolldeps/msvc
    SOURCES += ../windows/getopt.cc
    PRE_TARGETDEPS = \
      ../build-librecoll-$$QCBUILDLOC-Release/release/recoll.lib
    LIBS += \
      -L../build-librecoll-$$QCBUILDLOC-Release/release -lrecoll \
      $$RECOLLDEPS/libxml2/libxml2-2.9.4+dfsg1/win32/bin.msvc/libxml2.lib \
      $$RECOLLDEPS/libxslt/libxslt-1.1.29/win32/bin.msvc/libxslt.lib \
      -L../build-libxapian-$$QCBUILDLOC-Release/release -llibxapian \
      -L$$RECOLLDEPS/wlibiconv/build-libiconv-$$QCBUILDLOC-Release/release/ -liconv \
      $$RECOLLDEPS/libmagic/src/lib/libmagic.lib \
      $$RECOLLDEPS/regex/libregex.lib \
      $$RECOLLDEPS/zlib-1.2.11/zdll.lib \
      -lrpcrt4 -lws2_32 -luser32 -lshell32 -lshlwapi -lpsapi -lkernel32
  }

  INCLUDEPATH += ../windows
}

unix:!mac {
    QCBUILDLOC=Desktop
    SOURCES += \
      ../utils/execmd.cpp \
      ../utils/netcon.cpp

    PRE_TARGETDEPS = \
      ../build-librecoll-$$QCBUILDLOC-Release/librecoll.so
    LIBS += \
      -L../build-librecoll-$$QCBUILDLOC-Release/ -lrecoll \
      -lxapian -lxslt -lxml2 -lmagic -lz -lX11
}

mac {
  QCBUILDLOC=Qt_6_4_2_for_macOS
  QMAKE_APPLE_DEVICE_ARCHS = x86_64 arm64
  QMAKE_CXXFLAGS += -std=c++11 -pthread -Wno-unused-parameter
  DEFINES += RECOLL_AS_MAC_BUNDLE
  SOURCES += \
    ../utils/execmd.cpp \
    ../utils/netcon.cpp \
    ../utils/rclionice.cpp
  PRE_TARGETDEPS = ../build-librecoll-$$QCBUILDLOC-Release/librecoll.a
  LIBS += \
     ../build-librecoll-$$QCBUILDLOC-Release/librecoll.a \
     ../build-libxapian-$$QCBUILDLOC-Release/liblibxapian.a \
     -lxslt -lxml2 -liconv -lz
}
