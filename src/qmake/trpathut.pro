
QT       -= core gui

TARGET = pathut
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

DEFINES += BUILDING_RECOLL
DEFINES += UNICODE
DEFINES += PSAPI_VERSION=1
DEFINES += __WIN32__

SOURCES += \
../testmains/trpathut.cpp

INCLUDEPATH += ../common ../index ../internfile ../query \
            ../unac ../utils ../aspell ../rcldb ../qtgui \
            ../xaposix ../confgui ../bincimapmime 

windows {

  contains(QMAKE_CC, cl){
    # Visual Studio
    RECOLLDEPS = ../../recolldeps/msvc
    LIBS += \
      -L../build-librecoll-Desktop_Qt_5_14_1_MSVC2017_32bit-Release/release \
        -llibrecoll \
      $$RECOLLDEPS/libxml2/libxml2-2.9.4+dfsg1/win32/bin.msvc/libxml2.lib \
      $$RECOLLDEPS/libxslt/libxslt-1.1.29/win32/bin.msvc/libxslt.lib \
      -L../build-libxapian-Desktop_Qt_5_14_1_MSVC2017_32bit-Release/release \
        -llibxapian \
      -L$$RECOLLDEPS/build-libiconv-Desktop_Qt_5_14_1_MSVC2017_32bit-Release/release/ \
        -llibiconv \
      $$RECOLLDEPS/zlib-1.2.11/zdll.lib \
      -lrpcrt4 -lws2_32 -luser32 -lshell32 \
      -lshlwapi -lpsapi -lkernel32
  }

  INCLUDEPATH += ../windows
}

mac {
# Never built on mac. Just in case, don't forget:
  DEFINES += RECOLL_AS_MAC_BUNDLE
}
