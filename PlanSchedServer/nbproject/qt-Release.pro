# This file is generated automatically. Do not edit.
# Use project properties -> Build -> Qt -> Expert -> Custom Definitions.
TEMPLATE = app
DESTDIR = builds/Lin64bit/release
TARGET = PlanSchedServer
VERSION = 1.0.0
CONFIG -= debug_and_release app_bundle lib_bundle
CONFIG += release 
PKGCONFIG +=
QT = core gui widgets network xml
SOURCES += ../../../Common/RandExt/RandExt.cpp /data/Projects/Common/Debug/DebugExt.cpp src/PlanSchedServer/PlanSchedServer.cpp src/main.cpp
HEADERS += ../../../Common/Assignable/Assignable.h ../../../Common/Clonable/Clonable.h ../../../Common/Comparable/Comparable.h ../../../Common/Exceptions/Exception.h ../../../Common/Exceptions/MsgException.h ../../../Common/Functor/Functor.h ../../../Common/Include/Assignable ../../../Common/Include/Clonable ../../../Common/Include/Comparable ../../../Common/Include/Exceptions ../../../Common/Include/Functor ../../../Common/Include/MathExt ../../../Common/Include/Messages ../../../Common/Include/Object ../../../Common/Include/Operationable ../../../Common/Include/RandExt ../../../Common/Include/Runnable ../../../Common/Include/SavableRestorable ../../../Common/Include/SenderReceiver ../../../Common/Include/Signals ../../../Common/Include/SmartPointer ../../../Common/Include/WritableReadable ../../../Common/MathExt/MathExt.h ../../../Common/Messages/Messages.h ../../../Common/Object/Object.h ../../../Common/Operationable/Operationable.h ../../../Common/RandExt/RandExt.h ../../../Common/Runnable/Runnable.h ../../../Common/SavableRestorable/SavableRestorable.h ../../../Common/SenderReceiver/SenderReceiver.h ../../../Common/Signals/Signal.h ../../../Common/SmartPointer/SmartPointer.h ../../../Common/WritableReadable/WritableReadable.h /data/Projects/Common/Debug/DebugExt.h src/PlanSchedServer/PlanSchedServer.h
FORMS +=
RESOURCES +=
TRANSLATIONS +=
OBJECTS_DIR = build/Release/GNU-Linux
MOC_DIR = $$DESTDIR/moc
RCC_DIR = $$DESTDIR/moc
UI_DIR = $$DESTDIR/moc
QMAKE_CC = gcc-5
QMAKE_CXX = g++-5
DEFINES += DEBUG EDEBUG IDEBUG WDEBUG 
INCLUDEPATH += ../../../Common/Debug ../../../Common/Include ../Common 
LIBS += 
equals(QT_MAJOR_VERSION, 4) {
QMAKE_CXXFLAGS += -std=c++11
}
equals(QT_MAJOR_VERSION, 5) {
CONFIG += c++11
}
OBJECTS_DIR = $$DESTDIR/obj
