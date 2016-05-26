# This file is generated automatically. Do not edit.
# Use project properties -> Build -> Qt -> Expert -> Custom Definitions.
TEMPLATE = app
DESTDIR = builds/Lin64bit/debug
TARGET = PlanSchedServer
VERSION = 1.0.0
CONFIG -= debug_and_release app_bundle lib_bundle
CONFIG += debug 
PKGCONFIG +=
QT = core gui network xml
SOURCES += ../../../Common/RandExt/RandExt.cpp /data/Projects/Common/Debug/DebugExt.cpp src/PlanSchedServer/PlanSchedServer.cpp src/main.cpp
HEADERS += ../../../Common/Acceptable/Acceptable.h ../../../Common/Algorithm/Algorithm.h ../../../Common/Assignable/Assignable.h ../../../Common/Changeable/Changeable.h ../../../Common/Clonable/Clonable.h ../../../Common/Comparable/Comparable.h ../../../Common/Driver/Driven.h ../../../Common/Driver/Driver.h ../../../Common/Exceptions/Exception.h ../../../Common/Exceptions/MsgException.h ../../../Common/Functor/Functor.h ../../../Common/Include/Acceptable ../../../Common/Include/Algorithm ../../../Common/Include/Assignable ../../../Common/Include/Changeable ../../../Common/Include/Clonable ../../../Common/Include/Comparable ../../../Common/Include/Driver ../../../Common/Include/Exceptions ../../../Common/Include/Functor ../../../Common/Include/Loader ../../../Common/Include/MathExt ../../../Common/Include/Messages ../../../Common/Include/Object ../../../Common/Include/Operationable ../../../Common/Include/Parser ../../../Common/Include/RandExt ../../../Common/Include/Runnable ../../../Common/Include/SavableRestorable ../../../Common/Include/SenderReceiver ../../../Common/Include/Signals ../../../Common/Include/SmartPointer ../../../Common/Include/Solver ../../../Common/Include/Stopable ../../../Common/Include/Variables ../../../Common/Include/WritableReadable ../../../Common/Loader/Loader.h ../../../Common/Loader/Loader_DLL.h ../../../Common/MathExt/MathExt.h ../../../Common/Messages/Messages.h ../../../Common/Object/Object.h ../../../Common/Operationable/Operationable.h ../../../Common/Parser/Parser.h ../../../Common/RandExt/RandExt.h ../../../Common/RandExt/RandExt_CombinedRandGen.h ../../../Common/RandExt/RandExt_Interfaces.h ../../../Common/RandExt/RandExt_LCG.h ../../../Common/RandExt/RandExt_MersenneTwister.h ../../../Common/Runnable/Runnable.h ../../../Common/SavableRestorable/SavableRestorable.h ../../../Common/SenderReceiver/SenderReceiver.h ../../../Common/Signals/Signal.h ../../../Common/SmartPointer/SmartPointer.h ../../../Common/Solvers/Solver.h ../../../Common/Stopable/Stopable.h ../../../Common/Variables/Variables.h ../../../Common/WritableReadable/WritableReadable.h /data/Projects/Common/Debug/DebugExt.h src/PlanSchedServer/PlanSchedServer.h
FORMS +=
RESOURCES +=
TRANSLATIONS +=
OBJECTS_DIR = build/Debug/GNU_GCC5-Linux
MOC_DIR = $$DESTDIR/moc
RCC_DIR = $$DESTDIR/moc
UI_DIR = $$DESTDIR/moc
QMAKE_CC = gcc-5
QMAKE_CXX = g++-5
DEFINES += DEBUG EDEBUG IDEBUG WDEBUG 
INCLUDEPATH += ../../../Common/Debug ../../../Common/Include ../Common 
LIBS += -Wl,-rpath,/home/DrSobik/Projects/IPPS/CPPALG/Common/builds/Lin64bit/release /home/DrSobik/Projects/IPPS/CPPALG/Common/builds/Lin64bit/release/libCommon.so.1.0.0  
equals(QT_MAJOR_VERSION, 4) {
QMAKE_CXXFLAGS += -std=c++14
}
OBJECTS_DIR=$$DESTDIR/obj
QMAKE_CXXFLAGS += -std=c++14
AFTERBUILDDIR = ../bin/$$TARGET
unix:QMAKE_POST_LINK = mkdir -p $$AFTERBUILDDIR &&  cp -f -d $$DESTDIR/$$TARGET $$AFTERBUILDDIR
