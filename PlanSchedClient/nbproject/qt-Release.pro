# This file is generated automatically. Do not edit.
# Use project properties -> Build -> Qt -> Expert -> Custom Definitions.
TEMPLATE = app
DESTDIR = builds/Lin64bit/app/release
TARGET = PlanSchedClient
VERSION = 1.0.0
CONFIG -= debug_and_release app_bundle lib_bundle
CONFIG += release 
PKGCONFIG +=
QT = core gui widgets network
SOURCES += src/PlanSchedClient/PlanSchedClient.cpp src/PlanSchedClientDLLInterface/PlanSchedClientDLLInterface.cpp src/QtSSLIssue/QtSSLIssue.cpp src/main.cpp
HEADERS += src/PlanSchedClient/PlanSchedClient.h src/PlanSchedClientDLLInterface/PlanSchedClientDLLInterface.h src/PlanSchedClientDefinitions/PlanSchedClientDefinitions.h src/QtSSLIssue/QtSSLIssue.h
FORMS +=
RESOURCES +=
TRANSLATIONS +=
OBJECTS_DIR = build/Release/GNU_GCC5-Linux
MOC_DIR = $$DESTDIR/moc
RCC_DIR = $$DESTDIR/rcc
UI_DIR = $$DESTDIR/ui
QMAKE_CC = gcc-5
QMAKE_CXX = g++-5
DEFINES += 
INCLUDEPATH += src/PlanSchedClient src/PlanSchedClientDLLInterface src/PlanSchedClientDefinitions 
LIBS += 
equals(QT_MAJOR_VERSION, 4) {
QMAKE_CXXFLAGS += -std=c++14
}
OBJECTS_DIR=$$DESTDIR/obj
CONFIG += c++14
QMAKE_CXXFLAGS += -std=c++14
