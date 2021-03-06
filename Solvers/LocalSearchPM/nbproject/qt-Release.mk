#############################################################################
# Makefile for building: libLocalSearchPM.so.1.0.0
# Generated by qmake (3.0) (Qt 5.5.1)
# Project:  nbproject/qt-Release.pro
# Template: lib
# Command: /usr/bin/qmake-qt5 VPATH=. -o qttmp-Release.mk nbproject/qt-Release.pro
#############################################################################

MAKEFILE      = qttmp-Release.mk

####### Compiler, tools and options

CC            = gcc-5
CXX           = g++-5
DEFINES       = -DDEBUG -DDLL_EXPORT -DEDEBUG -DIDEBUG -DLS_MSG -DWDEBUG -DQT_NO_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_XML_LIB -DQT_CORE_LIB
CFLAGS        = -pipe -O2 -Wall -W -D_REENTRANT -fPIC $(DEFINES)
CXXFLAGS      = -pipe -std=c++14 -O2 -Wall -W -D_REENTRANT -fPIC $(DEFINES)
INCPATH       = -Inbproject -I. -I../../../../Common/Include -I../../../../Common/Debug -I../../Common/src -I../../Common/src/Include -Isrc -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtXml -isystem /usr/include/qt5/QtCore -Ibuilds/Lin64bit/release/moc -I/usr/lib64/qt5/mkspecs/linux-g++
QMAKE         = /usr/bin/qmake-qt5
DEL_FILE      = rm -f
CHK_DIR_EXISTS= test -d
MKDIR         = mkdir -p
COPY          = cp -f
COPY_FILE     = cp -f
COPY_DIR      = cp -f -R
INSTALL_FILE  = install -m 644 -p
INSTALL_PROGRAM = install -m 755 -p
INSTALL_DIR   = cp -f -R
DEL_FILE      = rm -f
SYMLINK       = ln -f -s
DEL_DIR       = rmdir
MOVE          = mv -f
TAR           = tar -cf
COMPRESS      = gzip -9f
DISTNAME      = LocalSearchPM1.0.0
DISTDIR = /data/Projects/IPPS/CPPALG/Solvers/LocalSearchPM/builds/Lin64bit/release/obj/LocalSearchPM1.0.0
LINK          = g++
LFLAGS        = -Wl,-O1 -shared -Wl,-soname,libLocalSearchPM.so.1
LIBS          = $(SUBLIBS) -Wl,-rpath,../../Common/builds/Lin64bit/release ../../Common/builds/Lin64bit/release/libCommon.so.1.0.0 -lQt5Widgets -L/usr/lib64 -lQt5Gui -lQt5Xml -lQt5Core -lGL -lpthread 
AR            = ar cqs
RANLIB        = 
SED           = sed
STRIP         = 

####### Output directory

OBJECTS_DIR   = builds/Lin64bit/release/obj/

####### Files

SOURCES       = ../../../../Common/Debug/DebugExt.cpp \
		../../../../Common/RandExt/RandExt.cpp \
		src/DLLExportInterface/DLLExportInterface.cpp \
		src/LocalSearchPM/LocalSearchPM.cpp \
		src/newmain.cpp 
OBJECTS       = builds/Lin64bit/release/obj/DebugExt.o \
		builds/Lin64bit/release/obj/RandExt.o \
		builds/Lin64bit/release/obj/DLLExportInterface.o \
		builds/Lin64bit/release/obj/LocalSearchPM.o \
		builds/Lin64bit/release/obj/newmain.o
DIST          = /usr/lib64/qt5/mkspecs/features/spec_pre.prf \
		/usr/lib64/qt5/mkspecs/common/unix.conf \
		/usr/lib64/qt5/mkspecs/common/linux.conf \
		/usr/lib64/qt5/mkspecs/common/sanitize.conf \
		/usr/lib64/qt5/mkspecs/common/gcc-base.conf \
		/usr/lib64/qt5/mkspecs/common/gcc-base-unix.conf \
		/usr/lib64/qt5/mkspecs/common/g++-base.conf \
		/usr/lib64/qt5/mkspecs/common/g++-unix.conf \
		/usr/lib64/qt5/mkspecs/qconfig.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_bootstrap_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_clucene_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_concurrent.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_concurrent_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_core.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_core_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_dbus.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_dbus_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_designer.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_designer_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_designercomponents_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_eglfs_device_lib_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_gui.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_gui_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_help.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_help_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_network.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_network_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_opengl.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_opengl_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_openglextensions.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_openglextensions_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_platformsupport_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_printsupport.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_printsupport_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_qml.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_qml_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_qmldevtools_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_qmltest.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_qmltest_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_quick.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_quick_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_quickparticles_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_quickwidgets.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_quickwidgets_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_sql.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_sql_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_svg.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_svg_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_testlib.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_testlib_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_uiplugin.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_uitools.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_uitools_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_widgets.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_widgets_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_x11extras.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_x11extras_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_xcb_qpa_lib_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_xml.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_xml_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_xmlpatterns.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_xmlpatterns_private.pri \
		/usr/lib64/qt5/mkspecs/features/qt_functions.prf \
		/usr/lib64/qt5/mkspecs/features/qt_config.prf \
		/usr/lib64/qt5/mkspecs/linux-g++/qmake.conf \
		/usr/lib64/qt5/mkspecs/features/spec_post.prf \
		/usr/lib64/qt5/mkspecs/features/exclusive_builds.prf \
		/usr/lib64/qt5/mkspecs/features/default_pre.prf \
		/usr/lib64/qt5/mkspecs/features/resolve_config.prf \
		/usr/lib64/qt5/mkspecs/features/default_post.prf \
		/usr/lib64/qt5/mkspecs/features/warn_on.prf \
		/usr/lib64/qt5/mkspecs/features/qt.prf \
		/usr/lib64/qt5/mkspecs/features/resources.prf \
		/usr/lib64/qt5/mkspecs/features/moc.prf \
		/usr/lib64/qt5/mkspecs/features/unix/opengl.prf \
		/usr/lib64/qt5/mkspecs/features/uic.prf \
		/usr/lib64/qt5/mkspecs/features/unix/thread.prf \
		/usr/lib64/qt5/mkspecs/features/testcase_targets.prf \
		/usr/lib64/qt5/mkspecs/features/exceptions.prf \
		/usr/lib64/qt5/mkspecs/features/yacc.prf \
		/usr/lib64/qt5/mkspecs/features/lex.prf \
		nbproject/nbproject/qt-Release.pro ../../../../Common/Algorithm/Algorithm.h \
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
		src/DLLExportInterface/DLLExportInterface.h \
		src/LocalSearchPM/LocalSearchPM.h ../../../../Common/Debug/DebugExt.cpp \
		../../../../Common/RandExt/RandExt.cpp \
		src/DLLExportInterface/DLLExportInterface.cpp \
		src/LocalSearchPM/LocalSearchPM.cpp \
		src/newmain.cpp
QMAKE_TARGET  = LocalSearchPM
DESTDIR       = builds/Lin64bit/release/#avoid trailing-slash linebreak
TARGET        = libLocalSearchPM.so.1.0.0
TARGETA       = builds/Lin64bit/release/libLocalSearchPM.a
TARGET0       = libLocalSearchPM.so
TARGETD       = libLocalSearchPM.so.1.0.0
TARGET1       = libLocalSearchPM.so.1
TARGET2       = libLocalSearchPM.so.1.0


first: all
####### Implicit rules

.SUFFIXES: .o .c .cpp .cc .cxx .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o "$@" "$<"

####### Build rules

builds/Lin64bit/release/$(TARGET):  $(OBJECTS) $(SUBLIBS) $(OBJCOMP)  
	@test -d builds/Lin64bit/release/ || mkdir -p builds/Lin64bit/release/
	-$(DEL_FILE) $(TARGET) $(TARGET0) $(TARGET1) $(TARGET2)
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS) $(OBJCOMP)
	-ln -s $(TARGET) $(TARGET0)
	-ln -s $(TARGET) $(TARGET1)
	-ln -s $(TARGET) $(TARGET2)
	-$(DEL_FILE) builds/Lin64bit/release/$(TARGET)
	-$(MOVE) $(TARGET)  builds/Lin64bit/release/ 
	-$(DEL_FILE) builds/Lin64bit/release/$(TARGET0)
	-$(DEL_FILE) builds/Lin64bit/release/$(TARGET1)
	-$(DEL_FILE) builds/Lin64bit/release/$(TARGET2)
	-$(MOVE) $(TARGET0) builds/Lin64bit/release/ 
	-$(MOVE) $(TARGET1) builds/Lin64bit/release/ 
	-$(MOVE) $(TARGET2) builds/Lin64bit/release/ 
	mkdir -p ../../bin/DLL/Solvers/LocalSearchPM && cp -f -d builds/Lin64bit/release/lib* ../../bin/DLL/Solvers/LocalSearchPM



staticlib: $(TARGETA)

$(TARGETA):  $(OBJECTS) $(OBJCOMP) 
	-$(DEL_FILE) $(TARGETA) 
	$(AR) $(TARGETA) $(OBJECTS)

qttmp-Release.mk: nbproject/qt-Release.pro /usr/lib64/qt5/mkspecs/linux-g++/qmake.conf /usr/lib64/qt5/mkspecs/features/spec_pre.prf \
		/usr/lib64/qt5/mkspecs/common/unix.conf \
		/usr/lib64/qt5/mkspecs/common/linux.conf \
		/usr/lib64/qt5/mkspecs/common/sanitize.conf \
		/usr/lib64/qt5/mkspecs/common/gcc-base.conf \
		/usr/lib64/qt5/mkspecs/common/gcc-base-unix.conf \
		/usr/lib64/qt5/mkspecs/common/g++-base.conf \
		/usr/lib64/qt5/mkspecs/common/g++-unix.conf \
		/usr/lib64/qt5/mkspecs/qconfig.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_bootstrap_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_clucene_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_concurrent.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_concurrent_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_core.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_core_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_dbus.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_dbus_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_designer.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_designer_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_designercomponents_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_eglfs_device_lib_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_gui.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_gui_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_help.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_help_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_network.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_network_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_opengl.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_opengl_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_openglextensions.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_openglextensions_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_platformsupport_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_printsupport.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_printsupport_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_qml.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_qml_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_qmldevtools_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_qmltest.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_qmltest_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_quick.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_quick_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_quickparticles_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_quickwidgets.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_quickwidgets_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_sql.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_sql_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_svg.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_svg_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_testlib.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_testlib_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_uiplugin.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_uitools.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_uitools_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_widgets.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_widgets_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_x11extras.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_x11extras_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_xcb_qpa_lib_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_xml.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_xml_private.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_xmlpatterns.pri \
		/usr/lib64/qt5/mkspecs/modules/qt_lib_xmlpatterns_private.pri \
		/usr/lib64/qt5/mkspecs/features/qt_functions.prf \
		/usr/lib64/qt5/mkspecs/features/qt_config.prf \
		/usr/lib64/qt5/mkspecs/linux-g++/qmake.conf \
		/usr/lib64/qt5/mkspecs/features/spec_post.prf \
		/usr/lib64/qt5/mkspecs/features/exclusive_builds.prf \
		/usr/lib64/qt5/mkspecs/features/default_pre.prf \
		/usr/lib64/qt5/mkspecs/features/resolve_config.prf \
		/usr/lib64/qt5/mkspecs/features/default_post.prf \
		/usr/lib64/qt5/mkspecs/features/warn_on.prf \
		/usr/lib64/qt5/mkspecs/features/qt.prf \
		/usr/lib64/qt5/mkspecs/features/resources.prf \
		/usr/lib64/qt5/mkspecs/features/moc.prf \
		/usr/lib64/qt5/mkspecs/features/unix/opengl.prf \
		/usr/lib64/qt5/mkspecs/features/uic.prf \
		/usr/lib64/qt5/mkspecs/features/unix/thread.prf \
		/usr/lib64/qt5/mkspecs/features/testcase_targets.prf \
		/usr/lib64/qt5/mkspecs/features/exceptions.prf \
		/usr/lib64/qt5/mkspecs/features/yacc.prf \
		/usr/lib64/qt5/mkspecs/features/lex.prf \
		nbproject/qt-Release.pro \
		/usr/lib64/libQt5Widgets.prl \
		/usr/lib64/libQt5Gui.prl \
		/usr/lib64/libQt5Xml.prl \
		/usr/lib64/libQt5Core.prl
	$(QMAKE) VPATH=. -o qttmp-Release.mk nbproject/qt-Release.pro
/usr/lib64/qt5/mkspecs/features/spec_pre.prf:
/usr/lib64/qt5/mkspecs/common/unix.conf:
/usr/lib64/qt5/mkspecs/common/linux.conf:
/usr/lib64/qt5/mkspecs/common/sanitize.conf:
/usr/lib64/qt5/mkspecs/common/gcc-base.conf:
/usr/lib64/qt5/mkspecs/common/gcc-base-unix.conf:
/usr/lib64/qt5/mkspecs/common/g++-base.conf:
/usr/lib64/qt5/mkspecs/common/g++-unix.conf:
/usr/lib64/qt5/mkspecs/qconfig.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_bootstrap_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_clucene_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_concurrent.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_concurrent_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_core.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_core_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_dbus.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_dbus_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_designer.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_designer_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_designercomponents_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_eglfs_device_lib_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_gui.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_gui_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_help.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_help_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_network.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_network_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_opengl.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_opengl_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_openglextensions.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_openglextensions_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_platformsupport_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_printsupport.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_printsupport_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_qml.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_qml_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_qmldevtools_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_qmltest.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_qmltest_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_quick.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_quick_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_quickparticles_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_quickwidgets.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_quickwidgets_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_sql.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_sql_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_svg.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_svg_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_testlib.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_testlib_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_uiplugin.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_uitools.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_uitools_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_widgets.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_widgets_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_x11extras.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_x11extras_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_xcb_qpa_lib_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_xml.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_xml_private.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_xmlpatterns.pri:
/usr/lib64/qt5/mkspecs/modules/qt_lib_xmlpatterns_private.pri:
/usr/lib64/qt5/mkspecs/features/qt_functions.prf:
/usr/lib64/qt5/mkspecs/features/qt_config.prf:
/usr/lib64/qt5/mkspecs/linux-g++/qmake.conf:
/usr/lib64/qt5/mkspecs/features/spec_post.prf:
/usr/lib64/qt5/mkspecs/features/exclusive_builds.prf:
/usr/lib64/qt5/mkspecs/features/default_pre.prf:
/usr/lib64/qt5/mkspecs/features/resolve_config.prf:
/usr/lib64/qt5/mkspecs/features/default_post.prf:
/usr/lib64/qt5/mkspecs/features/warn_on.prf:
/usr/lib64/qt5/mkspecs/features/qt.prf:
/usr/lib64/qt5/mkspecs/features/resources.prf:
/usr/lib64/qt5/mkspecs/features/moc.prf:
/usr/lib64/qt5/mkspecs/features/unix/opengl.prf:
/usr/lib64/qt5/mkspecs/features/uic.prf:
/usr/lib64/qt5/mkspecs/features/unix/thread.prf:
/usr/lib64/qt5/mkspecs/features/testcase_targets.prf:
/usr/lib64/qt5/mkspecs/features/exceptions.prf:
/usr/lib64/qt5/mkspecs/features/yacc.prf:
/usr/lib64/qt5/mkspecs/features/lex.prf:
nbproject/qt-Release.pro:
/usr/lib64/libQt5Widgets.prl:
/usr/lib64/libQt5Gui.prl:
/usr/lib64/libQt5Xml.prl:
/usr/lib64/libQt5Core.prl:
qmake: FORCE
	@$(QMAKE) VPATH=. -o qttmp-Release.mk nbproject/qt-Release.pro

qmake_all: FORCE


all: qttmp-Release.mk builds/Lin64bit/release/$(TARGET)

dist: distdir FORCE
	(cd `dirname $(DISTDIR)` && $(TAR) $(DISTNAME).tar $(DISTNAME) && $(COMPRESS) $(DISTNAME).tar) && $(MOVE) `dirname $(DISTDIR)`/$(DISTNAME).tar.gz . && $(DEL_FILE) -r $(DISTDIR)

distdir: FORCE
	@test -d $(DISTDIR) || mkdir -p $(DISTDIR)
	$(COPY_FILE) --parents $(DIST) $(DISTDIR)/
	$(COPY_FILE) --parents ../../../../Common/Algorithm/Algorithm.h ../../../../Common/Assignable/Assignable.h ../../../../Common/Clonable/Clonable.h ../../../../Common/Comparable/Comparable.h ../../../../Common/Debug/DebugExt.h ../../../../Common/Driver/Driven.h ../../../../Common/Driver/Driver.h ../../../../Common/Exceptions/Exception.h ../../../../Common/Exceptions/MsgException.h ../../../../Common/Functor/Functor.h ../../../../Common/Include/Assignable ../../../../Common/Include/Clonable ../../../../Common/Include/Comparable ../../../../Common/Include/Driver ../../../../Common/Include/Exceptions ../../../../Common/Include/Functor ../../../../Common/Include/MathExt ../../../../Common/Include/Messages ../../../../Common/Include/Object ../../../../Common/Include/Operationable ../../../../Common/Include/RandExt ../../../../Common/Include/Runnable ../../../../Common/Include/SavableRestorable ../../../../Common/Include/SenderReceiver ../../../../Common/Include/Signals ../../../../Common/Include/SmartPointer ../../../../Common/Include/Solver ../../../../Common/Include/Stopable ../../../../Common/Include/WritableReadable ../../../../Common/MathExt/MathExt.h ../../../../Common/Messages/Messages.h ../../../../Common/Object/Object.h ../../../../Common/Operationable/Operationable.h ../../../../Common/RandExt/RandExt.h ../../../../Common/RandExt/RandExt_CombinedRandGen.h ../../../../Common/RandExt/RandExt_Interfaces.h ../../../../Common/RandExt/RandExt_LCG.h ../../../../Common/RandExt/RandExt_MersenneTwister.h ../../../../Common/Runnable/Runnable.h ../../../../Common/SavableRestorable/SavableRestorable.h ../../../../Common/SenderReceiver/SenderReceiver.h ../../../../Common/Signals/Signal.h ../../../../Common/SmartPointer/SmartPointer.h ../../../../Common/Solvers/Solver.h ../../../../Common/Stopable/Stopable.h ../../../../Common/WritableReadable/WritableReadable.h src/DLLExportInterface/DLLExportInterface.h src/LocalSearchPM/LocalSearchPM.h $(DISTDIR)/
	$(COPY_FILE) --parents ../../../../Common/Debug/DebugExt.cpp ../../../../Common/RandExt/RandExt.cpp src/DLLExportInterface/DLLExportInterface.cpp src/LocalSearchPM/LocalSearchPM.cpp src/newmain.cpp $(DISTDIR)/


clean: compiler_clean 
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) *~ core *.core


distclean: clean 
	-$(DEL_FILE) builds/Lin64bit/release/$(TARGET) 
	-$(DEL_FILE) builds/Lin64bit/release/$(TARGET0) builds/Lin64bit/release/$(TARGET1) builds/Lin64bit/release/$(TARGET2) $(TARGETA)
	-$(DEL_FILE) qttmp-Release.mk


####### Sub-libraries

mocclean: compiler_moc_header_clean compiler_moc_source_clean

mocables: compiler_moc_header_make_all compiler_moc_source_make_all

check: first

compiler_rcc_make_all:
compiler_rcc_clean:
compiler_moc_header_make_all:
compiler_moc_header_clean:
compiler_moc_source_make_all:
compiler_moc_source_clean:
compiler_uic_make_all:
compiler_uic_clean:
compiler_yacc_decl_make_all:
compiler_yacc_decl_clean:
compiler_yacc_impl_make_all:
compiler_yacc_impl_clean:
compiler_lex_make_all:
compiler_lex_clean:
compiler_clean: 

####### Compile

builds/Lin64bit/release/obj/DebugExt.o: ../../../../Common/Debug/DebugExt.cpp ../../../../Common/Debug/DebugExt.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o builds/Lin64bit/release/obj/DebugExt.o ../../../../Common/Debug/DebugExt.cpp

builds/Lin64bit/release/obj/RandExt.o: ../../../../Common/RandExt/RandExt.cpp ../../../../Common/RandExt/RandExt.h \
		../../../../Common/Include/Object \
		../../../../Common/Object/Object.h \
		../../../../Common/Include/Clonable \
		../../../../Common/Clonable/Clonable.h \
		../../../../Common/Include/MathExt \
		../../../../Common/MathExt/MathExt.h \
		../../../../Common/Include/Exceptions \
		../../../../Common/Exceptions/Exception.h \
		../../../../Common/Exceptions/MsgException.h \
		../../../../Common/Include/WritableReadable \
		../../../../Common/WritableReadable/WritableReadable.h \
		../../../../Common/Include/Messages \
		../../../../Common/Messages/Messages.h \
		../../../../Common/Include/Operationable \
		../../../../Common/Operationable/Operationable.h \
		../../../../Common/RandExt/RandExt_MersenneTwister.h \
		../../../../Common/Include/Driver \
		../../../../Common/Driver/Driver.h \
		../../../../Common/Driver/Driven.h \
		../../../../Common/RandExt/RandExt_Interfaces.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o builds/Lin64bit/release/obj/RandExt.o ../../../../Common/RandExt/RandExt.cpp

builds/Lin64bit/release/obj/DLLExportInterface.o: src/DLLExportInterface/DLLExportInterface.cpp src/DLLExportInterface/DLLExportInterface.h \
		../../../../Common/Include/Solver \
		../../../../Common/Solvers/Solver.h \
		../../../../Common/Include/Operationable \
		../../../../Common/Operationable/Operationable.h \
		../../Common/src/IPPSDefinitions \
		../../Common/src/SchedulingProblem \
		../../Common/src/Schedule \
		src/LocalSearchPM/LocalSearchPM.h \
		../../../../Common/Debug/DebugExt.h \
		../../../../Common/Include/RandExt \
		../../../../Common/RandExt/RandExt.h \
		../../../../Common/Include/Object \
		../../../../Common/Object/Object.h \
		../../../../Common/Include/Clonable \
		../../../../Common/Clonable/Clonable.h \
		../../../../Common/Include/MathExt \
		../../../../Common/MathExt/MathExt.h \
		../../../../Common/Include/Exceptions \
		../../../../Common/Exceptions/Exception.h \
		../../../../Common/Exceptions/MsgException.h \
		../../../../Common/Include/WritableReadable \
		../../../../Common/WritableReadable/WritableReadable.h \
		../../../../Common/Include/Messages \
		../../../../Common/Messages/Messages.h \
		../../../../Common/RandExt/RandExt_MersenneTwister.h \
		../../../../Common/Include/Driver \
		../../../../Common/Driver/Driver.h \
		../../../../Common/Driver/Driven.h \
		../../../../Common/RandExt/RandExt_Interfaces.h \
		../../../../Common/RandExt/RandExt_LCG.h \
		../../../../Common/RandExt/RandExt_CombinedRandGen.h \
		../../../../Common/Include/SmartPointer \
		../../../../Common/SmartPointer/SmartPointer.h \
		../../../../Common/Include/Comparable \
		../../../../Common/Comparable/Comparable.h \
		../../../../Common/Include/Loader \
		../../../../Common/Loader/Loader.h \
		../../../../Common/Loader/Loader_DLL.h \
		../../Common/src/Resources \
		../../Common/src/ProcessModel \
		../../Common/src/IterativeAlg \
		../../Common/src/Objective \
		../../Common/src/Scheduler
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o builds/Lin64bit/release/obj/DLLExportInterface.o src/DLLExportInterface/DLLExportInterface.cpp

builds/Lin64bit/release/obj/LocalSearchPM.o: src/LocalSearchPM/LocalSearchPM.cpp src/LocalSearchPM/LocalSearchPM.h \
		../../../../Common/Include/Solver \
		../../../../Common/Solvers/Solver.h \
		../../../../Common/Include/Operationable \
		../../../../Common/Operationable/Operationable.h \
		../../../../Common/Debug/DebugExt.h \
		../../../../Common/Include/RandExt \
		../../../../Common/RandExt/RandExt.h \
		../../../../Common/Include/Object \
		../../../../Common/Object/Object.h \
		../../../../Common/Include/Clonable \
		../../../../Common/Clonable/Clonable.h \
		../../../../Common/Include/MathExt \
		../../../../Common/MathExt/MathExt.h \
		../../../../Common/Include/Exceptions \
		../../../../Common/Exceptions/Exception.h \
		../../../../Common/Exceptions/MsgException.h \
		../../../../Common/Include/WritableReadable \
		../../../../Common/WritableReadable/WritableReadable.h \
		../../../../Common/Include/Messages \
		../../../../Common/Messages/Messages.h \
		../../../../Common/RandExt/RandExt_MersenneTwister.h \
		../../../../Common/Include/Driver \
		../../../../Common/Driver/Driver.h \
		../../../../Common/Driver/Driven.h \
		../../../../Common/RandExt/RandExt_Interfaces.h \
		../../../../Common/RandExt/RandExt_LCG.h \
		../../../../Common/RandExt/RandExt_CombinedRandGen.h \
		../../../../Common/Include/SmartPointer \
		../../../../Common/SmartPointer/SmartPointer.h \
		../../../../Common/Include/Comparable \
		../../../../Common/Comparable/Comparable.h \
		../../../../Common/Include/Loader \
		../../../../Common/Loader/Loader.h \
		../../../../Common/Loader/Loader_DLL.h \
		../../Common/src/IPPSDefinitions \
		../../Common/src/Resources \
		../../Common/src/ProcessModel \
		../../Common/src/IterativeAlg \
		../../Common/src/Objective \
		../../Common/src/SchedulingProblem \
		../../Common/src/Schedule \
		../../Common/src/Scheduler
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o builds/Lin64bit/release/obj/LocalSearchPM.o src/LocalSearchPM/LocalSearchPM.cpp

builds/Lin64bit/release/obj/newmain.o: src/newmain.cpp src/LocalSearchPM/LocalSearchPM.h \
		../../../../Common/Include/Solver \
		../../../../Common/Solvers/Solver.h \
		../../../../Common/Include/Operationable \
		../../../../Common/Operationable/Operationable.h \
		../../../../Common/Debug/DebugExt.h \
		../../../../Common/Include/RandExt \
		../../../../Common/RandExt/RandExt.h \
		../../../../Common/Include/Object \
		../../../../Common/Object/Object.h \
		../../../../Common/Include/Clonable \
		../../../../Common/Clonable/Clonable.h \
		../../../../Common/Include/MathExt \
		../../../../Common/MathExt/MathExt.h \
		../../../../Common/Include/Exceptions \
		../../../../Common/Exceptions/Exception.h \
		../../../../Common/Exceptions/MsgException.h \
		../../../../Common/Include/WritableReadable \
		../../../../Common/WritableReadable/WritableReadable.h \
		../../../../Common/Include/Messages \
		../../../../Common/Messages/Messages.h \
		../../../../Common/RandExt/RandExt_MersenneTwister.h \
		../../../../Common/Include/Driver \
		../../../../Common/Driver/Driver.h \
		../../../../Common/Driver/Driven.h \
		../../../../Common/RandExt/RandExt_Interfaces.h \
		../../../../Common/RandExt/RandExt_LCG.h \
		../../../../Common/RandExt/RandExt_CombinedRandGen.h \
		../../../../Common/Include/SmartPointer \
		../../../../Common/SmartPointer/SmartPointer.h \
		../../../../Common/Include/Comparable \
		../../../../Common/Comparable/Comparable.h \
		../../../../Common/Include/Loader \
		../../../../Common/Loader/Loader.h \
		../../../../Common/Loader/Loader_DLL.h \
		../../Common/src/IPPSDefinitions \
		../../Common/src/Resources \
		../../Common/src/ProcessModel \
		../../Common/src/IterativeAlg \
		../../Common/src/Objective \
		../../Common/src/SchedulingProblem \
		../../Common/src/Schedule \
		../../Common/src/Scheduler
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o builds/Lin64bit/release/obj/newmain.o src/newmain.cpp

####### Install

install:  FORCE

uninstall:  FORCE

FORCE:

