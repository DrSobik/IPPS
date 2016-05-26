# This file is generated automatically. Do not edit.
# Use project properties -> Build -> Qt -> Expert -> Custom Definitions.
TEMPLATE = app
DESTDIR = builds/Lin64bit/debug
TARGET = SchedClient
VERSION = 1.0.0
CONFIG -= debug_and_release app_bundle lib_bundle
CONFIG += debug 
PKGCONFIG +=
QT = core gui widgets network
SOURCES += src/QtSSLIssue.cpp src/SchedClient.cpp src/SchedClientDLLInterface.cpp src/main.cpp
HEADERS += src/QtSSLIssue.h src/SchedClient.h src/SchedClientDLLInterface.h src/schedclient_global.h
FORMS +=
RESOURCES +=
TRANSLATIONS +=
OBJECTS_DIR = build/Debug/GNU_GCC5-Linux
MOC_DIR = $$DESTDIR/moc
RCC_DIR = $$DESTDIR/rcc
UI_DIR = $$DESTDIR/ui
QMAKE_CC = gcc-5
QMAKE_CXX = g++-5
DEFINES += 
INCLUDEPATH += 
LIBS += 
equals(QT_MAJOR_VERSION, 4) {
QMAKE_CXXFLAGS += -std=c++14
}
OBJECTS_DIR=$$DESTDIR/obj
QMAKE_CXXFLAGS += -std=c++14
