#-------------------------------------------------
#
# Project created by QtCreator 2013-07-16T11:23:03
#
#-------------------------------------------------

DEFINES += SCHEDCLIENT_LIBRARY

QT       += network xml
QT       -= gui

TARGET = PlanSchedClient

TEMPLATE = lib

CONFIG -= staticlib
CONFIG += dll

release{
    OBJECTS_DIR = release/obj
    MOC_DIR = release/moc
    DESTDIR = release
}

debug{
    OBJECTS_DIR = debug/obj
    MOC_DIR = debug/moc
    DESTDIR = debug
}


SOURCES += \
                ../../src/PlanSchedClient/PlanSchedClient.cpp \
                ../../src/PlanSchedClientDLLInterface/PlanSchedClientDLLInterface.cpp

HEADERS +=\
                ../../src/PlanSchedClientDefinitions/PlanSchedClientDefinitions.h \
                ../../src/PlanSchedClient/PlanSchedClient.h \
                ../../src/PlanSchedClientDLLInterface/PlanSchedClientDLLInterface.h

INCLUDEPATH += \
               ../../src/PlanSchedClientDefinitions \
               ../../src/PlanSchedClient\
               ../../src/PlanSchedClientDLLInterface

win32:AFTERBUILDDIR=..\\..\\..\\..\\bin\\$$TARGET
win32:QMAKE_POST_LINK=(IF NOT EXIST $$AFTERBUILDDIR mkdir $$AFTERBUILDDIR) & copy $$DESTDIR\\*.dll $$AFTERBUILDDIR /Y
