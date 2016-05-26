#-------------------------------------------------
#
# Project created by QtCreator 2013-07-16T11:23:03
#
#-------------------------------------------------

#DEFINES += SCHEDCLIENT_LIBRARY

QT       += network xml
QT       -= gui

CONFIG   += console
CONFIG   -= app_bundle

TARGET = PlanSchedClient

TEMPLATE = app

release{
    OBJECTS_DIR = release/obj
    MOC_DIR = release/moc
    DESTDIR = release/
}

debug{
    OBJECTS_DIR = debug/obj
    MOC_DIR = debug/moc
    DESTDIR = debug/
}

SOURCES += \
                ../../src/PlanSchedClient/PlanSchedClient.cpp \
                ../../src/main.cpp \
                ../../src/PlanSchedClientDLLInterface/PlanSchedClientDLLInterface.cpp

HEADERS +=\
                ../../src/PlanSchedClientDefinitions/PlanSchedClientDefinitions.h \
                ../../src/PlanSchedClient/PlanSchedClient.h \
                ../../src/PlanSchedClientDLLInterface/PlanSchedClientDLLInterface.h

INCLUDEPATH += \
               ../../src/PlanSchedClientDefinitions \
               ../../src/PlanSchedClient\
               ../../src/PlanSchedClientDLLInterface
