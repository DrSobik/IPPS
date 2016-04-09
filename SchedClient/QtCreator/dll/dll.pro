#-------------------------------------------------
#
# Project created by QtCreator 2013-07-16T11:23:03
#
#-------------------------------------------------

DEFINES += SCHEDCLIENT_LIBRARY

QT       += network xml
QT       -= gui

TARGET = SchedClient

TEMPLATE = lib

CONFIG -= staticlib
CONFIG += dll

release{
    OBJECTS_DIR = release/obj
    MOC_DIR = release/moc
}

debug{
    OBJECTS_DIR = debug/obj
    MOC_DIR = debug/moc
}


SOURCES += \
    ../../src/SchedClient.cpp \
    ../../src/SchedClientDLLInterface.cpp

HEADERS +=\
        ../../src/schedclient_global.h \
        ../../src/SchedClient.h \
    ../../src/SchedClientDLLInterface.h
