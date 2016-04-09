#-------------------------------------------------
#
# Project created by QtCreator 2013-07-16T12:26:48
#
#-------------------------------------------------

QT       += core network xml
QT       -= gui

TARGET = SchedServer
TEMPLATE = app

CONFIG   += console
CONFIG   -= app_bundle

release{
    OBJECTS_DIR = release/obj
    MOC_DIR = release/moc
}

debug{
    OBJECTS_DIR = debug/obj
    MOC_DIR = debug/moc
}


SOURCES += ../src/main.cpp \
    ../src/SchedServer.cpp

HEADERS += \
    ../src/SchedServer.h
