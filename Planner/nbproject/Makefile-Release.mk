#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc-5
CCC=g++-5
CXX=g++-5
FC=gfortran
AS=as
QMAKE=qmake-qt5

# Macros
CND_PLATFORM=GNU_GCC5-Linux
CND_DLIB_EXT=so
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES=


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=/home/DrSobik/Projects/IPPS/CPPALG/Common/builds/Lin64bit/release/libCommon.so.1.0.0

nbproject/qt-${CND_CONF}.mk: nbproject/qt-${CND_CONF}.pro FORCE
	${QMAKE} VPATH=. -o qttmp-${CND_CONF}.mk nbproject/qt-${CND_CONF}.pro
	mv -f qttmp-${CND_CONF}.mk nbproject/qt-${CND_CONF}.mk

FORCE:

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS} nbproject/qt-${CND_CONF}.mk
	"${MAKE}" -f nbproject/qt-${CND_CONF}.mk builds/Lin64bit/release/Planner

${CND_BUILDDIR}/Release/%.o: nbproject/qt-${CND_CONF}.mk
	${MAKE} -f nbproject/qt-${CND_CONF}.mk "$@"

# Subprojects
.build-subprojects:
	cd /home/DrSobik/Projects/IPPS/CPPALG/Solvers/LocalSearchPM && ${MAKE}  -f Makefile CONF=Release
	cd /home/DrSobik/Projects/IPPS/CPPALG/Solvers/PriorityScheduler && ${MAKE}  -f Makefile CONF=Release
	cd /home/DrSobik/Projects/IPPS/CPPALG/Solvers/CombinedScheduler && ${MAKE}  -f Makefile CONF=Release
	cd /home/DrSobik/Projects/IPPS/CPPALG/Common && ${MAKE}  -f Makefile CONF=Release
	cd /home/DrSobik/Projects/IPPS/CPPALG/Solvers/SBHScheduler && ${MAKE}  -f Makefile CONF=Release
	cd /home/DrSobik/Projects/IPPS/CPPALG/Solvers/VNSScheduler && ${MAKE}  -f Makefile CONF=Release
	cd /home/DrSobik/Projects/IPPS/CPPALG/Solvers/VNSPlanner && ${MAKE}  -f Makefile CONF=Release

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS} nbproject/qt-${CND_CONF}.mk
	${MAKE} -f nbproject/qt-${CND_CONF}.mk distclean

# Subprojects
.clean-subprojects:
	cd /home/DrSobik/Projects/IPPS/CPPALG/Solvers/LocalSearchPM && ${MAKE}  -f Makefile CONF=Release clean
	cd /home/DrSobik/Projects/IPPS/CPPALG/Solvers/PriorityScheduler && ${MAKE}  -f Makefile CONF=Release clean
	cd /home/DrSobik/Projects/IPPS/CPPALG/Solvers/CombinedScheduler && ${MAKE}  -f Makefile CONF=Release clean
	cd /home/DrSobik/Projects/IPPS/CPPALG/Common && ${MAKE}  -f Makefile CONF=Release clean
	cd /home/DrSobik/Projects/IPPS/CPPALG/Solvers/SBHScheduler && ${MAKE}  -f Makefile CONF=Release clean
	cd /home/DrSobik/Projects/IPPS/CPPALG/Solvers/VNSScheduler && ${MAKE}  -f Makefile CONF=Release clean
	cd /home/DrSobik/Projects/IPPS/CPPALG/Solvers/VNSPlanner && ${MAKE}  -f Makefile CONF=Release clean
