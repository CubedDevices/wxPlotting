#
# Generated Makefile - do not edit!
#
# Demo application linking libwxPlotting.a
#

# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/wxPlottingApp.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=`wx-config --cxxflags`
CXXFLAGS=`wx-config --cxxflags`

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
WXPLOTTING_LIB=../wxPlotting/dist/${CND_CONF}/${CND_PLATFORM}/libwxPlotting.a
LDLIBSOPTIONS=${WXPLOTTING_LIB} `wx-config --libs`

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/wxPlottingDemo

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/wxPlottingDemo: ${OBJECTFILES} ${WXPLOTTING_LIB}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/wxPlottingDemo ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/wxPlottingApp.o: ../../../wxPlottingApp.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I../../.. -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/wxPlottingApp.o ../../../wxPlottingApp.cpp

# Subprojects — build the static library first
.build-subprojects:
	cd ../wxPlotting && ${MAKE} CONF=${CND_CONF}

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} -r ${CND_DISTDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:
	cd ../wxPlotting && ${MAKE} CONF=${CND_CONF} clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
