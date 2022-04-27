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

#****************************************************************************
# Recursion
#****************************************************************************

MK_SUBDIRS = \
  check


#****************************************************************************
# Memory Analysis
#****************************************************************************

#----------------------------------------------------------------------------
# Build
#----------------------------------------------------------------------------

CXX = g++ -std=c++11 -Wall -Wno-unused-variable

LIBCXX_LIB = $(dir $(shell $(CXX) -print-file-name=libstdc++.so))

#****************************************************************************

mg_analyze := memgaze-analyze

MK_PROGRAMS_CXX = $(mg_analyze)

$(mg_analyze)_SRCS = \
	main.cpp \
	Function.cpp \
	Window.cpp

$(mg_analyze)_CXXFLAGS = \
	-g -O3

$(mg_analyze)_LDFLAGS =

$(mg_analyze)_LIBS =

$(mg_analyze)_LDADD =

#****************************************************************************

# Spack-based: source + configure, 2021.10.15

HPCTOOLKIT_ROOT ?= /files0/kili337/IntelPT/spack/opt/spack/linux-centos7-skylake_avx512/gcc-7.1.0/hpctoolkit-2021.10.15-gxyejpvcuclo4pp7dust3ym3f7runqze

HPCTOOLKIT_LIB = $(HPCTOOLKIT_ROOT)/lib
HPCTOOLKIT_INC = $(HPCTOOLKIT_ROOT)/share/hpctoolkit/src/src
HPCTOOLKIT_ARGS = $(HPCTOOLKIT_ROOT)/share/hpctoolkit/src/src/tool/hpcprof

HPCTOOLKIT_CXXFLAGS = \
	-I$(HPCTOOLKIT_INC) \
	-I$(HPCTOOLKIT_ARGS) \

HPCTOOLKIT_LDFLAGS = \
        -L$(HPCTOOLKIT_LIB) -Wl,-rpath $(HPCTOOLKIT_LIB)

# FIXME: LDADD from <hpctk>/src/tool/hpcprof/Makefile.am
HPCTOOLKIT_LDADD =


#----------------------------------------------------------------------------

mg_hpctk := memgaze-hpctk

MK_PROGRAMS_CXX += # $(mg_hpctk)

$(mg_hpctk)_SRCS = \
	main-hpctk.cpp

$(mg_hpctk)_CXXFLAGS = \
	-g -O3 \
  $(HPCTOOLKIT_CXXFLAGS)

$(mg_hpctk)_LDFLAGS = \
  $(HPCTOOLKIT_LDFLAGS)

$(mg_hpctk)_LIBS =

$(mg_hpctk)_LDADD = \
  $(HPCTOOLKIT_LDADD)


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
