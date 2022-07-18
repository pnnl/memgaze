# -*-Mode: makefile;-*-

#*BeginPNNLCopyright*********************************************************
#
# $HeadURL$
# $Id$
#
#***********************************************************EndPNNLCopyright*

#****************************************************************************
# Ozgur Ozan Kilic, Nathan Tallent
#****************************************************************************


#****************************************************************************
# Package defs
#****************************************************************************

include ../Makefile-defs.mk
#include ../xlib/lib/hpctoolkit-develop/share/hpctoolkit/src/src/Makefile.in

#****************************************************************************
# Recursion
#****************************************************************************

MK_SUBDIRS = \
  check \
  loc-anlys


#****************************************************************************
# Memory Analysis
#****************************************************************************

#----------------------------------------------------------------------------
# Build
#----------------------------------------------------------------------------

CXX = g++ -std=c++11 -Wall -Wno-unused-variable

LIBCXX_LIB = $(dir $(shell $(CXX) -print-file-name=libstdc++.so))

#****************************************************************************

# Run configure: $(HPCTK_SRC)/.spack/spack-build-02-configure-out.txt

#OLD: HPCTK_ROOT = /files0/kili337/IntelPT/spack/opt/spack/linux-centos7-skylake_avx512/gcc-7.1.0/hpctoolkit-2021.10.15-gxyejpvcuclo4pp7dust3ym3f7runqze
#HPCToolkit requires C++17
HPCTK_CXXFLAGS = -I$(HPCTK_DEV_SRC) \
	-std=c++17 -pthread -fopenmp

HPCTK_SRCS = MemgazeSource.cpp \
	#$(HPCTK_DEV_SRC)/tool/hpcprof/args.cpp

HPCTK_LDFLAGS = -L$(HPCTK_DEV_LIB) -Wl,-rpath $(HPCTK_DEV_LIB) 

# FIXME: LDADD from <hpctk>/src/tool/hpcprof/Makefile.am
HPCTK_LDADD = $(HPCTKLIB_Profile)/$(HPCTKLIB_Profile_a) \
        $(HPCTKLIB_ProfileStandalone)/$(HPCTKLIB_ProfileStandalone_a) \
        $(HPCTKLIB_ProfLean)/$(HPCTKLIB_ProfLean_a) \
        $(HPCTKLIB_Support)/$(HPCTKLIB_Support_a)

#$(HPCTKLIB_SupportLean)/$(HPCTKLIB_SupportLean_a) 
	#$(HPCTKLIB_Binutils)/$(HPCTKLIB_Binutils_a) \
	#$(HPCTKLIB_Prof)/$(HPCTKLIB_Prof_a)

#LIBELF_LDFLAGS= \
	-L$(LIBELF_LIB) -Wl,-rpath $(LIBELF_LIB) \
	-ldw \
	-lelf

#LIBIBERTY_LDFLAGS= \
	-L$(LIBIBERTY_LIB) -Wl,-rpath $(LIBIBERTY_LIB) \
	-liberty

XERCES_LDFLAGS = \
	-L$(XERCES_LIB) -Wl,-rpath $(XERCES_LIB)

#****************************************************************************

mg_analyze := memgaze-analyze

MK_PROGRAMS_CXX = $(mg_analyze)

$(mg_analyze)_SRCS = \
	main.cpp \
	Function.cpp \
	Window.cpp \
	$(HPCTK_SRCS)

#HPCToolkit requires C++17, pthread, and omp
$(mg_analyze)_CXXFLAGS = \
	-g -O3 \
	$(HPCTK_CXXFLAGS)	

$(mg_analyze)_LDFLAGS = \
	$(HPCTK_LDFLAGS) \
	$(XERCES_LDFLAGS)
#$(LIBIBERTY_LDFLAGS) \
#$(LIBELF_LDFLAGS)

$(mg_analyze)_LIBS =

# HPCToolkit requires -lstdc++fs
$(mg_analyze)_LDADD = \
	$(HPCTK_LDADD) \
	-lstdc++fs \
	-lxerces-c

#$(LZMA_LIB) \
#$(LIBIBERTY_LDFLAGS) \
#------------ UNCOMMENT BELOW TO SEPARATE MemgazeSource BUILD ------------

#mg_hpctk := mg_hpctk

#MK_PROGRAMS_CXX += $(mg_hpctk)

#$(mg_hpctk)_SRCS = \
	MemgazeSource.cpp $(HPCTK_DEV_SRC)/tool/hpcprof/args.cpp

# -g -O3 already exist in $(mg_analyze)_CXXFLAGS
#$(mg_hpctk)_CXXFLAGS = \
	-std=c++17 -pthread -fopenmp \
  	$(HPCTK_CXXFLAGS)

#$(mg_hpctk)_LDFLAGS = \
  	$(HPCTK_LDFLAGS) \
	$(LIBELF_LDFLAGS) \
	$(XERCES_LDFLAGS)

#$(mg_hpctk)_LIBS = \

#(mg_hpctk)_LDADD = \
  	$(HPCTK_LDADD) \
  	$(LIBIBERTY_LDFLAGS) \
 	$(LZMA_LIB) \
	-lstdc++fs

#****************************************************************************
# Template Rules
#****************************************************************************

include ../Makefile-template.mk


#****************************************************************************
# Local Rules
#****************************************************************************


info.local :

install.local :
	$(INSTALL) -d $(PREFIX_LIBEXEC)
	$(INSTALL) memgaze-analyze $(PREFIX_LIBEXEC)

check.local :
