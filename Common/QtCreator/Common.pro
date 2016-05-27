
# config
TEMPLATE = lib
TARGET = Common
unix:VERSION = 1.0.0
CONFIG -= staticlib
CONFIG -= debug_and_release app_bundle debug
CONFIG += dll release 
QT = core xml

# sources
SOURCES += \
            ../../../../Common/Debug/DebugExt.cpp \
            ../../../../Common/RandExt/RandExt.cpp \
            ../src/BOM/BillOfMaterials.cpp \
            ../src/BOP/BillOfProcesses.cpp \
            ../src/DLLExportInterface/DLLExportInterface.cpp \
            ../src/FlowFactor/FlowFactor.cpp \
            ../src/IPPSProblem/IPPSProblem.cpp \
            ../src/Item/Item.cpp \
            ../src/IterativeAlg/IterativeAlg.cpp \
            ../src/Objective/Objective.cpp \
            ../src/Operation/Operation.cpp \
            ../src/Order/Order.cpp \
            ../src/Plan/Plan.cpp \
            ../src/PlanSched/PlanSched.cpp \
            ../src/Planner/Planner.cpp \
            ../src/ProcessModel/ProcessModel.cpp \
            ../src/ProcessModelManager/ProcessModelManager.cpp \
            ../src/Product/Product.cpp \
            ../src/Protocol/Protocol.cpp \
            ../src/Resources/Resources.cpp \
            ../src/Route/Route.cpp \
            ../src/Saveable/Saveable.cpp \
            ../src/Schedule/Schedule.cpp \
            ../src/Scheduler/Scheduler.cpp \
            ../src/SchedulingProblem/SchedulingProblem.cpp \
            ../src/TGSelection/TGSelection.cpp \
            ../src/newmain.cpp

#headers
HEADERS += \
            ../../../../Common/Acceptable/Acceptable.h \
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
            ../../../../Common/MathExt/MathExt.h \
            ../../../../Common/Messages/Messages.h \
            ../../../../Common/Object/Object.h \
            ../../../../Common/Operationable/Operationable.h \
            ../../../../Common/Parser/Parser.h \
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
            ../../../../Common/Variables/Variables.h \
            ../../../../Common/WritableReadable/WritableReadable.h \
            ../src/BOM/BillOfMaterials.h \
            ../src/BOP/BillOfProcesses.h \
            ../src/DLLExportInterface/DLLExportInterface.h \
            ../src/FlowFactor/FlowFactor.h \
            ../src/IPPSDefinitions/IPPSDefinitions.h \
            ../src/IPPSProblem/IPPSProblem.h \
            ../src/Include/BillOfMaterials \
            ../src/Include/BillOfProcesses \
            ../src/Include/FlowFactor \
            ../src/Include/IPPSDefinitions \
            ../src/Include/IPPSProblem \
            ../src/Include/Item \
            ../src/Include/IterativeAlg \
            ../src/Include/Objective \
            ../src/Include/Operation \
            ../src/Include/Order \
            ../src/Include/Plan \
            ../src/Include/PlanSched \
            ../src/Include/Planner \
            ../src/Include/ProcessModel \
            ../src/Include/ProcessModelManager \
            ../src/Include/Product \
            ../src/Include/Protocol \
            ../src/Include/Resources \
            ../src/Include/Route \
            ../src/Include/Saveable \
            ../src/Include/Schedule \
            ../src/Include/Scheduler \
            ../src/Include/SchedulingProblem \
            ../src/Include/TGSelection \
            ../src/Item/Item.h \
            ../src/IterativeAlg/IterativeAlg.h \
            ../src/Objective/Objective.h \
            ../src/Operation/Operation.h \
            ../src/Order/Order.h \
            ../src/Plan/Plan.h \
            ../src/PlanSched/PlanSched.h \
            ../src/Planner/Planner.h \
            ../src/ProcessModel/ProcessModel.h \
            ../src/ProcessModelManager/ProcessModelManager.h \
            ../src/Product/Product.h \
            ../src/Protocol/Protocol.h \
            ../src/Resources/Resources.h \
            ../src/Route/Route.h \
            ../src/Saveable/Saveable.h \
            ../src/Schedule/Schedule.h \
            ../src/Scheduler/Scheduler.h \
            ../src/SchedulingProblem/SchedulingProblem.h \
            ../src/TGSelection/TGSelection.h \
            ../src/Include/DLLExportInterface

INCLUDEPATH += ../../../../../Common/Include ../../../../../Common/Debug ../src/Include
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

    QMAKE_CXXFLAGS_WARN_ON = $$QMAKE_CXXFLAGS_WARN_ON

    QMAKE_CFLAGS_RELEASE -= /MD
    QMAKE_CXXFLAGS_RELEASE -= /MD
    QMAKE_CFLAGS_DEBUG -= /MDd
    QMAKE_CXXFLAGS_DEBUG -= /MDd

    QMAKE_CFLAGS_RELEASE += /MT
    QMAKE_CXXFLAGS_RELEASE += /MT
    QMAKE_CFLAGS_DEBUG += /MTd
    QMAKE_CXXFLAGS_DEBUG += /MTd

    equals(QT_MAJOR_VERSION, 4) {QMAKE_CXXFLAGS += -std=c++14}
    equals(QT_MAJOR_VERSION, 5) {CONFIG += c++14}
}

# libraries
win32:LIBS += "c:/lib/LEMON/lib/lemon.lib"

# AFTERBUILD
unix{
    AFTERBUILDDIR = ../../../bin/DLL/$$TARGET
    QMAKE_POST_LINK = mkdir -p $$AFTERBUILDDIR &&  cp -f -d $$DESTDIR/lib* $$AFTERBUILDDIR
}

win32{
    AFTERBUILDDIR=..\\..\\..\\bin\\DLL\\$$TARGET
    QMAKE_POST_LINK=(IF NOT EXIST $$AFTERBUILDDIR mkdir $$AFTERBUILDDIR) & copy $$DESTDIR\\*.dll $$AFTERBUILDDIR /Y
}
