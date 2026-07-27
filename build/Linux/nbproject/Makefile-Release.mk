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
	${OBJECTDIR}/_ext/29dd86f/seriesrenderer.o \
	${OBJECTDIR}/_ext/29dd86f/wxPlotting.o \
	${OBJECTDIR}/_ext/29dd86f/wxPlottingApp.o


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
LDLIBSOPTIONS=`wx-config --libs`  

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/wxPlotting

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/wxPlotting: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/wxPlotting ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/_ext/29dd86f/seriesrenderer.o: ../../seriesrenderer.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/29dd86f
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I../.. -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/29dd86f/seriesrenderer.o ../../seriesrenderer.cpp

${OBJECTDIR}/_ext/29dd86f/wxPlotting.o: ../../wxPlotting.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/29dd86f
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I../.. -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/29dd86f/wxPlotting.o ../../wxPlotting.cpp

${OBJECTDIR}/_ext/29dd86f/wxPlottingApp.o: ../../wxPlottingApp.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/29dd86f
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I../.. -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/29dd86f/wxPlottingApp.o ../../wxPlottingApp.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
