
QT       -= core gui

TARGET = rclstartw
TEMPLATE = app

DEFINES += BUILDING_RECOLL
DEFINES += UNICODE
DEFINES += PSAPI_VERSION=1
DEFINES += __WIN32__

SOURCES += \
../windows/rclstartw.cpp

INCLUDEPATH += ../common ../index ../internfile ../query ../unac ../utils ../aspell \
    ../rcldb ../qtgui ../xaposix ../confgui ../bincimapmime 

windows {
    contains(QMAKE_CC, cl){
        # Visual Studio
    }
  LIBS += -lshlwapi -lpsapi -lkernel32
  INCLUDEPATH += ../windows
}
