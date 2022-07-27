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

#LIBCXX_LIB = $(dir $(shell $(CXX) -print-file-name=libstdc++.so))

#****************************************************************************

# Run configure: $(HPCTK_SRC)/.spack/spack-build-02-configure-out.txt

#OLD: HPCTK_ROOT = /files0/kili337/IntelPT/spack/opt/spack/linux-centos7-skylake_avx512/gcc-7.1.0/hpctoolkit-2021.10.15-gxyejpvcuclo4pp7dust3ym3f7runqze
HPCTK_SRCS =
HPCTK_CXXFLAGS =
HPCTK_LDFLAGS = 
HPCTK_LDADD =
XERCES_LDFLAGS = 
LIBELF_LDFLAGS =
LIBIBERTY_LDFLAGS = 

#define CONSTANT_2_LOAD_RATIO 9
#define NPF_RATE -6
#define NPF_GROWTH_RATE 7
#define GROWTH_RATE -7
ifdef DEVELOP
  HPCTK_SRCS += MemgazeSource.cpp
  # $(HPCTK_DEV_SRC)/tool/hpcprof/args.cpp

  # HPCToolkit requires C++17
  HPCTK_CXXFLAGS += -I$(HPCTK_DEV_SRC) \
          -std=c++17 -pthread -fopenmp

  HPCTK_LDFLAGS += -L$(HPCTK_DEV_LIB) -Wl,-rpath $(HPCTK_DEV_LIB)

  # FIXME: LDADD from <hpctk>/src/tool/hpcprof/Makefile.am
  HPCTK_LDADD += $(HPCTKLIB_Profile)/$(HPCTKLIB_Profile_a) \
          $(HPCTKLIB_ProfileStandalone)/$(HPCTKLIB_ProfileStandalone_a) \
          $(HPCTKLIB_ProfLean)/$(HPCTKLIB_ProfLean_a) \
	  $(HPCTKLIB_SupportLean)/$(HPCTKLIB_SupportLean_a) \
          $(HPCTKLIB_Support)/$(HPCTKLIB_Support_a)

  XERCES_LDFLAGS += \
        -L$(XERCES_LIB) -Wl,-rpath $(XERCES_LIB)

  LIBELF_LDFLAGS += \
	-L$(LIBELF_LIB) -Wl,-rpath $(LIBELF_LIB)

  LIBIBIBERTY_LDFLAGS += \
	-L$(LIBIBERTY_LIB) -Wl,-rpath $(LIBIBERTY_LIB) 

endif

#****************************************************************************

mg_analyze := memgaze-analyze

MK_PROGRAMS_CXX = $(mg_analyze)
$(mg_analyze)_SRCS =
$(mg_analyze)_CXXFLAGS =
$(mg_analyze)_LDFLAGS =
$(mg_analyze)_LDADD =

ifdef DEVELOP
  $(mg_analyze)_SRCS += \
	  main.cpp \
	  Function.cpp \
	  Window.cpp \
	  $(HPCTK_SRCS)

  # HPCToolkit requires C++17, pthread, and omp
  $(mg_analyze)_CXXFLAGS += \
	        -g -O3 \
	        $(HPCTK_CXXFLAGS) \
	        -DDEVELOP

  $(mg_analyze)_LDFLAGS += \
          $(HPCTK_LDFLAGS) \
          $(XERCES_LDFLAGS) \
	  $(LIBELF_LDFLAGS) \
	  $(LIBIBERTY_LDFLAGS)

  # HPCToolkit requires -lstdc++fs and -lxerces-c
  $(mg_analyze)_LDADD += \
           $(HPCTK_LDADD) \
           -lstdc++fs \
           -lxerces-c \
	   -ldw \
	   -lelf \
	   -liberty
else
  $(mg_analyze)_SRCS += \
          main.cpp \
          Function.cpp \
          Window.cpp

  $(mg_analyze)_CXXFLAGS += \
          -g -O3

  $(mg_analyze)_LDFLAGS +=

  $(mg_analyze)_LDADD +=
endif


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
