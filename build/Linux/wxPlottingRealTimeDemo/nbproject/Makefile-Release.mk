#
# Generated Makefile - do not edit!
#
# Real-time demo linking libwxPlotting.a
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
	${OBJECTDIR}/wxPlottingRealTimeDemo.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=`wx-config --cxxflags` -pthread
CXXFLAGS=`wx-config --cxxflags` -pthread

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
WXPLOTTING_LIB=../wxPlotting/dist/${CND_CONF}/${CND_PLATFORM}/libwxPlotting.a
LDLIBSOPTIONS=${WXPLOTTING_LIB} `wx-config --libs` -pthread

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/wxPlottingRealTimeDemo

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/wxPlottingRealTimeDemo: ${OBJECTFILES} ${WXPLOTTING_LIB}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/wxPlottingRealTimeDemo ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/wxPlottingRealTimeDemo.o: ../../../wxPlottingRealTimeDemo.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I../../.. -pthread -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/wxPlottingRealTimeDemo.o ../../../wxPlottingRealTimeDemo.cpp

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
