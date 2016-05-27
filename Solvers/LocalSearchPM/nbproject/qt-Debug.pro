# This file is generated automatically. Do not edit.
# Use project properties -> Build -> Qt -> Expert -> Custom Definitions.
TEMPLATE = lib
DESTDIR = builds/Lin64bit/debug
TARGET = LocalSearchPM
VERSION = 1.0.0
CONFIG -= debug_and_release app_bundle lib_bundle
CONFIG += dll debug 
PKGCONFIG +=
QT = core gui widgets xml
SOURCES += ../../../../Common/Debug/DebugExt.cpp ../../../../Common/RandExt/RandExt.cpp src/DLLExportInterface/DLLExportInterface.cpp src/LocalSearchPM/LocalSearchPM.cpp src/newmain.cpp
HEADERS += ../../../../Common/Algorithm/Algorithm.h ../../../../Common/Assignable/Assignable.h ../../../../Common/Clonable/Clonable.h ../../../../Common/Comparable/Comparable.h ../../../../Common/Debug/DebugExt.h ../../../../Common/Driver/Driven.h ../../../../Common/Driver/Driver.h ../../../../Common/Exceptions/Exception.h ../../../../Common/Exceptions/MsgException.h ../../../../Common/Functor/Functor.h ../../../../Common/Include/Assignable ../../../../Common/Include/Clonable ../../../../Common/Include/Comparable ../../../../Common/Include/Driver ../../../../Common/Include/Exceptions ../../../../Common/Include/Functor ../../../../Common/Include/MathExt ../../../../Common/Include/Messages ../../../../Common/Include/Object ../../../../Common/Include/Operationable ../../../../Common/Include/RandExt ../../../../Common/Include/Runnable ../../../../Common/Include/SavableRestorable ../../../../Common/Include/SenderReceiver ../../../../Common/Include/Signals ../../../../Common/Include/SmartPointer ../../../../Common/Include/Solver ../../../../Common/Include/Stopable ../../../../Common/Include/WritableReadable ../../../../Common/MathExt/MathExt.h ../../../../Common/Messages/Messages.h ../../../../Common/Object/Object.h ../../../../Common/Operationable/Operationable.h ../../../../Common/RandExt/RandExt.h ../../../../Common/RandExt/RandExt_CombinedRandGen.h ../../../../Common/RandExt/RandExt_Interfaces.h ../../../../Common/RandExt/RandExt_LCG.h ../../../../Common/RandExt/RandExt_MersenneTwister.h ../../../../Common/Runnable/Runnable.h ../../../../Common/SavableRestorable/SavableRestorable.h ../../../../Common/SenderReceiver/SenderReceiver.h ../../../../Common/Signals/Signal.h ../../../../Common/SmartPointer/SmartPointer.h ../../../../Common/Solvers/Solver.h ../../../../Common/Stopable/Stopable.h ../../../../Common/WritableReadable/WritableReadable.h src/DLLExportInterface/DLLExportInterface.h src/LocalSearchPM/LocalSearchPM.h
FORMS +=
RESOURCES +=
TRANSLATIONS +=
OBJECTS_DIR = build/Debug/GNU_GCC5-Linux
MOC_DIR = $$DESTDIR/moc
RCC_DIR = $$DESTDIR/rcc
UI_DIR = $$DESTDIR/ui
QMAKE_CC = gcc-5
QMAKE_CXX = g++-5
DEFINES += DEBUG DLL_EXPORT EDEBUG IDEBUG LS_MSG WDEBUG 
INCLUDEPATH += ../../../../Common/Include ../../../../Common/Debug ../../Common/src ../../Common/src/Include src 
LIBS += 
equals(QT_MAJOR_VERSION, 4) {
QMAKE_CXXFLAGS += -std=c++14
}
OBJECTS_DIR=$$DESTDIR/obj
QMAKE_CXXFLAGS += -std=c++14
AFTERBUILDDIR = ../../bin/DLL/Solvers/$$TARGET
unix:QMAKE_POST_LINK = mkdir -p $$AFTERBUILDDIR &&  cp -f -d $$DESTDIR/lib* $$AFTERBUILDDIR
