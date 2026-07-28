#
# Generated Makefile - do not edit!
#
# Static library: libwxPlotting.a
# Sources at repository root (../../../).
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
AR=ar

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
	${OBJECTDIR}/seriesrenderer.o \
	${OBJECTDIR}/wxPlotting.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=`wx-config --cxxflags`
CXXFLAGS=`wx-config --cxxflags`

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libwxPlotting.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libwxPlotting.a: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libwxPlotting.a
	${AR} -rv ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libwxPlotting.a ${OBJECTFILES}
	$(RANLIB) ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libwxPlotting.a

${OBJECTDIR}/seriesrenderer.o: ../../../seriesrenderer.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I../../.. -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/seriesrenderer.o ../../../seriesrenderer.cpp

${OBJECTDIR}/wxPlotting.o: ../../../wxPlotting.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I../../.. -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/wxPlotting.o ../../../wxPlotting.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} -r ${CND_DISTDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
