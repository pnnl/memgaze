# -*-Mode: makefile;-*-

#*BeginPNNLCopyright*********************************************************
#
# $HeadURL$
# $Id: 
#
#***********************************************************EndPNNLCopyright*

#****************************************************************************
# $HeadURL$
#
# Ozgur Ozan Kilic, Nathan Tallent
#****************************************************************************

include ./Makefile-defs.mk

#****************************************************************************

CXX = g++ -std=c++11
#CXX = g++ -std=c++0x -Wall -Wno-unused-variable

LIBCXX_LIB = $(dir $(shell $(CXX) -print-file-name=libstdc++.so))

#****************************************************************************

footprint_PT = fp_intel_pt.x

#----------------------------------------------------------------------------

$(footprint_PT)_SRCS = \
	main.cpp \
        Function.cpp \
        Window.cpp

$(footprint_PT)_CXXFLAGS = \
        -g -O3 \

$(footprint_PT)_LDFLAGS =

$(footprint_PT)_LIBS =

$(footprint_PT)_LDADD =

#****************************************************************************

MK_PROGRAMS_CXX = $(footprint_PT)

include ./Makefile-template.mk

