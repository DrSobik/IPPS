#-------------------------------------------------
#
# Project created by QtCreator 2014-04-09T17:48:05
#
#-------------------------------------------------

# compiler flags
equals(QT_MAJOR_VERSION, 4) {QMAKE_CXXFLAGS += -std=c++11}
equals(QT_MAJOR_VERSION, 5) {CONFIG += c++11} 

#QMAKE_TARGET.arch = x86_64

#QMAKE_CXXFLAGS -= -m32
#QMAKE_CXXFLAGS += -m64
win32{

    message("Setting compiler flags for Win")

#    QMAKE_CFLAGS_WARN_ON -= /W4
#    QMAKE_CFLAGS_WARN_ON += /W3
    QMAKE_CXXFLAGS_WARN_ON += /wd4267
    QMAKE_CXXFLAGS_WARN_ON += /wd4100
    QMAKE_CXXFLAGS_WARN_ON += /wd4521

    QMAKE_CXXFLAGS_WARN_ON = $$QMAKE_CXXFLAGS_WARN_ON
}

contains(QMAKE_HOST.arch,x86_64){
    message("HOST is 64 bit")
}


!contains(QMAKE_TARGET.arch,x86_64){
    message("Building 32 bit")
}else{
    message("Building 64 bit")
}

# adjustments
QT       += core
QT       += gui
QT       += xml
QT       += network

#win32{
#
#  release{
#
#    DESTDIR = ./release/Win
#
#  }
#
#  debug{
#
#    DESTDIR = ./debug/Win
#
#  }
#
#}

#unix{
#
#  release{
#
#    DESTDIR = ./release/Lin
#
#  }
#
#  debug{
#
#    DESTDIR = ./debug/Lin
#
#  }
#
#}


TARGET = Planner

CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app

DEFINES += DEBUG
DEFINES += IDEBUG
DEFINES += WDEBUG
DEFINES += EDEBUG
DEFINES += WINDOWS
DEFINES += LS_MSG

win32{

    #INCLUDEPATH += "c:/msys64/lib/LEMON/include"

    #LIBS += "c:/msys64/lib/LEMON/lib/liblemon.a"

    INCLUDEPATH += "c:/lib/LEMON/include"

    LIBS += "c:/lib/LEMON/lib/lemon.lib"

    release{

        DEFINES += NO_XML_TEST_OUTPUT

        DEFINES -= WDEBUG
    }
}

# Directory for object code
#unix:OBJECTS_DIR = $$DESTDIR/obj
#win32:OBJECTS_DIR = $$DESTDIR/obj

# Directory for MOC-files
#unix:MOC_DIR = $$DESTDIR/moc
#win32:MOC_DIR = $$DESTDIR/moc

# where to put intermediate files
debug{
  OBJECTS_DIR = debug/obj
  MOC_DIR = debug/moc
}

# where to put intermediate files
release{
  OBJECTS_DIR = release/obj
  MOC_DIR = release/moc
}

# Include path
INCLUDEPATH += ../../Common \
               ../../../../Common/Debug \
               ../../../../Common/Include \
               ../../../../LEMON/include

# Source files
SOURCES += ../../../../Common/Debug/DebugExt.cpp \
	../../../../Common/RandExt/RandExt.cpp \
	../../Common/BiDirScheduler.cpp \
	../../Common/BillOfMaterials.cpp \
	../../Common/BillOfProcesses.cpp \
	../../Common/CPScheduler.cpp \
	../../Common/Clonable.cpp \
	../../Common/DispatchScheduler.cpp \
#	../../Common/GAPlanner.cpp \
	../../Common/Item.cpp \
	../../Common/IterativeAlg.cpp \
	../../Common/LocalSearchPM.cpp \
	../../Common/LocalSearchSM.cpp \
	../../Common/Objective.cpp \
	../../Common/OneDirScheduler.cpp \
	../../Common/Operation.cpp \
	../../Common/Order.cpp \
	../../Common/Plan.cpp \
	../../Common/Planner.cpp \
	../../Common/ProcessModel.cpp \
	../../Common/ProcessModelManager.cpp \
	../../Common/Product.cpp \
	../../Common/Protocol.cpp \
	../../Common/Resources.cpp \
	../../Common/Route.cpp \
	../../Common/SBHScheduler.cpp \
	../../Common/Saveable.cpp \
	../../Common/Schedule.cpp \
	../../Common/Scheduler.cpp \
	../../Common/TGScheduler.cpp \
	../../Common/TGSelection.cpp \
	../../Common/TGVNSScheduler.cpp \
	../../Common/TrivialScheduler.cpp \
	../../Common/VNSPlanner.cpp \
	../../Common/VNSScheduler.cpp \
	../src/PlannerAgent/PlannerAgent.cpp \
	../src/main.cpp 

# Header files
HEADERS += ../../../../Common/Assignable/Assignable.h \
	../../../../Common/Clonable/Clonable.h \
	../../../../Common/Comparable/Comparable.h \
	../../../../Common/Debug/DebugExt.h \
	../../../../Common/Exceptions/Exception.h \
	../../../../Common/Exceptions/MsgException.h \
	../../../../Common/Functor/Functor.h \
	../../../../Common/Include/Assignable \
	../../../../Common/Include/Clonable \
	../../../../Common/Include/Comparable \
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
	../../../../Common/Include/SmartPointer \
	../../../../Common/Include/WritableReadable \
	../../../../Common/MathExt/MathExt.h \
	../../../../Common/Messages/Messages.h \
	../../../../Common/Object/Object.h \
	../../../../Common/Operationable/Operationable.h \
	../../../../Common/RandExt/RandExt.h \
	../../../../Common/Runnable/Runnable.h \
	../../../../Common/SavableRestorable/SavableRestorable.h \
	../../../../Common/SenderReceiver/SenderReceiver.h \
	../../../../Common/SmartPointer/SmartPointer.h \
	../../../../Common/WritableReadable/WritableReadable.h \
	../../Common/BiDirScheduler.h \
	../../Common/BillOfMaterials.h \
	../../Common/BillOfProcesses.h \
	../../Common/CPScheduler.h \
	../../Common/Clonable.h \
	../../Common/DispatchScheduler.h \
#	../../Common/GAPlanner.h \
	../../Common/Item.h \
	../../Common/IterativeAlg.h \
	../../Common/LocalSearchPM.h \
	../../Common/LocalSearchSM.h \
	../../Common/Objective.h \
	../../Common/OneDirScheduler.h \
	../../Common/Operation.h \
	../../Common/Order.h \
	../../Common/Plan.h \
	../../Common/Planner.h \
	../../Common/ProcessModel.h \
	../../Common/ProcessModelManager.h \
	../../Common/Product.h \
	../../Common/Protocol.h \
	../../Common/Resources.h \
	../../Common/Route.h \
	../../Common/SBHScheduler.h \
	../../Common/Saveable.h \
	../../Common/Schedule.h \
	../../Common/Scheduler.h \
	../../Common/TGScheduler.h \
	../../Common/TGSelection.h \
	../../Common/TGVNSScheduler.h \
	../../Common/TrivialScheduler.h \
	../../Common/VNSPlanner.h \
	../../Common/VNSScheduler.h \
	../src/PlannerAgent/PlannerAgent.h





