
# config
TEMPLATE = app
TARGET = PlanSchedServer
unix:VERSION = 1.0.0
CONFIG -= staticlib
CONFIG -= debug_and_release app_bundle debug
CONFIG += release console
QT = core xml network

# sources
SOURCES += \
            ../../../../Common/Debug/DebugExt.cpp \
            ../../../../Common/RandExt/RandExt.cpp \
            ../src/PlanSchedServer/PlanSchedServer.cpp \
            ../src/main.cpp

#headers
HEADERS += \
            ../../../../Common/Algorithm/Algorithm.h \
            ../../../../Common/Assignable/Assignable.h \
            ../../../../Common/Clonable/Clonable.h \
            ../../../../Common/Comparable/Comparable.h \
            ../../../../Common/Debug/DebugExt.h \
            ../../../../Common/Driver/Driven.h \
            ../../../../Common/Driver/Driver.h \
            ../../../../Common/Exceptions/Exception.h \
            ../../../../Common/Exceptions/MsgException.h \
            ../../../../Common/Functor/Functor.h \
#            ../../../../Common/Include/Algorithm \
            ../../../../Common/Include/Assignable \
            ../../../../Common/Include/Clonable \
            ../../../../Common/Include/Comparable \
            ../../../../Common/Include/Driver \
            ../../../../Common/Include/Exceptions \
            ../../../../Common/Include/Functor \
            ../../../../Common/Include/MathExt \
            ../../../../Common/Include/Messages \
            ../../../../Common/Include/Object \
            ../../../../Common/Include/Operationable \
            ../../../../Common/Include/RandExt \
            ../../../../Common/Include/Runnable \
            ../../../../Common/Include/SavableRestorable \
            ../../../../Common/Include/SenderReceiver \
            ../../../../Common/Include/Signals \
            ../../../../Common/Include/SmartPointer \
            ../../../../Common/Include/Solver \
            ../../../../Common/Include/Stopable \
            ../../../../Common/Include/WritableReadable \
            ../../../../Common/Loader/Loader.h \
            ../../../../Common/MathExt/MathExt.h \
            ../../../../Common/Messages/Messages.h \
            ../../../../Common/Object/Object.h \
            ../../../../Common/Operationable/Operationable.h \
            ../../../../Common/RandExt/RandExt.h \
            ../../../../Common/RandExt/RandExt_CombinedRandGen.h \
            ../../../../Common/RandExt/RandExt_Interfaces.h \
            ../../../../Common/RandExt/RandExt_LCG.h \
            ../../../../Common/RandExt/RandExt_MersenneTwister.h \
            ../../../../Common/Runnable/Runnable.h \
            ../../../../Common/SavableRestorable/SavableRestorable.h \
            ../../../../Common/SenderReceiver/SenderReceiver.h \
            ../../../../Common/Signals/Signal.h \
            ../../../../Common/SmartPointer/SmartPointer.h \
            ../../../../Common/Solvers/Solver.h \
            ../../../../Common/Stopable/Stopable.h \
            ../../../../Common/WritableReadable/WritableReadable.h \
            ../src/PlanSchedServer/PlanSchedServer.h

INCLUDEPATH += ../../../../Common/Include ../../../../Common/Debug ../../Common/src/Include ../../Common/src ../src
win32:INCLUDEPATH += "c:/lib/LEMON/include"

# directories
release:DESTDIR = release
debug:DESTDIR = debug
OBJECTS_DIR=$$DESTDIR/obj
MOC_DIR = $$DESTDIR/moc
RCC_DIR = $$DESTDIR/rcc
UI_DIR = $$DESTDIR/ui

# compile defines
DEFINES += DEBUG DLL_EXPORT EDEBUG IDEBUG LS_MSG WDEBUG
win32:release:DEFINES += NO_XML_TEST_OUTPUT
win32:release:DEFINES -= WDEBUG

# compiler
unix{
    QMAKE_CC = gcc-5
    QMAKE_CXX = g++-5
}

# compiler flags
unix{
    QMAKE_CXXFLAGS += -std=c++14
}

win32{
    message("Setting compiler flags for Win")

#    QMAKE_CFLAGS_WARN_ON -= /W4
#    QMAKE_CFLAGS_WARN_ON += /W3
    QMAKE_CXXFLAGS_WARN_ON += /wd4267
    QMAKE_CXXFLAGS_WARN_ON += /wd4100
    QMAKE_CXXFLAGS_WARN_ON += /wd4521

    QMAKE_CFLAGS_RELEASE -= /MD
    QMAKE_CXXFLAGS_RELEASE -= /MD
    QMAKE_CFLAGS_DEBUG -= /MDd
    QMAKE_CXXFLAGS_DEBUG -= /MDd

    QMAKE_CFLAGS_RELEASE += /MT
    QMAKE_CXXFLAGS_RELEASE += /MT
    QMAKE_CFLAGS_DEBUG += /MTd
    QMAKE_CXXFLAGS_DEBUG += /MTd
    
    QMAKE_CXXFLAGS_WARN_ON = $$QMAKE_CXXFLAGS_WARN_ON

    equals(QT_MAJOR_VERSION, 4) {QMAKE_CXXFLAGS += -std=c++14}
    equals(QT_MAJOR_VERSION, 5) {CONFIG += c++14}
}

# libraries
unix:LIBS += -Wl,-rpath,../../Common/builds/Lin64bit/release ../../Common/builds/Lin64bit/release/libCommon.so.1.0.0
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../Common/builds/Win64bit_static/release/ -lCommon
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../Common/builds/Win64bit_static/debug/ -lCommon
win32:INCLUDEPATH += $$PWD/../../Common/builds/Win64bit_static/release
win32:DEPENDPATH += $$PWD/../../Common/builds/Win64bit_static/release
win32:LIBS += "c:/lib/LEMON/lib/lemon.lib"

# AFTERBUILD
unix{
    AFTERBUILDDIR = ../../../bin/$$TARGET
    QMAKE_POST_LINK = mkdir -p $$AFTERBUILDDIR &&  cp -f -d $$DESTDIR/$$TARGET $$AFTERBUILDDIR
}

win32{
    AFTERBUILDDIR=..\\..\\..\\bin\\$$TARGET
    QMAKE_POST_LINK=(IF NOT EXIST $$AFTERBUILDDIR mkdir $$AFTERBUILDDIR) & copy $$DESTDIR\\$${TARGET}.exe $$AFTERBUILDDIR /Y
}

