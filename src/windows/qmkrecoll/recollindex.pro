
QT       -= core gui

TARGET = recollindex
TEMPLATE = app

DEFINES += BUILDING_RECOLL
DEFINES -= UNICODE
DEFINES -= _UNICODE
DEFINES += _MBCS
DEFINES += PSAPI_VERSION=1


SOURCES += \
../../index/recollindex.cpp \
../../index/rclmonprc.cpp \
../../index/rclmonrcv.cpp 

INCLUDEPATH += ../../common ../../index ../../internfile ../../query \
            ../../unac ../../utils ../../aspell ../../rcldb ../../qtgui \
            ../../xaposix ../../confgui ../../bincimapmime 

windows {
    contains(QMAKE_CC, gcc){
        # MingW
        QMAKE_CXXFLAGS += -std=c++11 -Wno-unused-parameter
    }
    contains(QMAKE_CC, cl){
        # Visual Studio
    }
  LIBS += \
    C:/recoll/src/windows/build-librecoll-Desktop_Qt_5_5_0_MinGW_32bit-Debug/debug/librecoll.dll \
    -lshlwapi -lpsapi -lkernel32

  INCLUDEPATH += ../../windows
}
