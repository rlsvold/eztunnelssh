
TEMPLATE = app
TARGET = ezTunnelSSH
DESTDIR = .
QT *= network
CONFIG -= debug_and_release debug release
CONFIG *= debug_and_release
CONFIG *= warn_on thread
INCLUDEPATH += ..
DEPENDPATH += ..
MOC_DIR += GeneratedFiles
OBJECTS_DIR += GeneratedFiles
UI_DIR += GeneratedFiles

#Include file(s)
include(../ATSkeleton.pri)

#Windows resource file
win32:RC_FILE = ../ATSkeleton.rc

RESOURCES = ../res/resource.qrc

