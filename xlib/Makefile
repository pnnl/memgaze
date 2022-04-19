# -*-Mode: makefile;-*-

#*BeginPNNLCopyright*********************************************************
#
# $HeadURL$
# $Id$
#
#***********************************************************EndPNNLCopyright*

#****************************************************************************
# Package defs
#****************************************************************************

include ../Makefile-defs.mk

# FIXME: correct and part of defs
#MEMGAZE_ROOT := $(shell pwd)/..

# FIXME: Part of defs
#MG_XLIB = $(MEMGAZE_ROOT)/xlib

# MG_XLIB_ROOT: Bulid files for MemGaze XLIB
#MG_XLIB_ROOT       = $(MG_XLIB)


#****************************************************************************
# Recursion
#****************************************************************************

# MK_SUBDIRS =

#****************************************************************************
# Xlib
#****************************************************************************

MG_XLIB_SPACK_ROOT = $(MG_XLIB_ROOT)/spack


xlib_spack:
	mkdir -p $(MG_XLIB_ROOT) && \
	  cd $(MG_XLIB_ROOT) && \
	  git clone -c feature.manyFiles=true https://github.com/spack/spack.git && \
	  cp $(MG_XLIB)/config.yaml $(MG_XLIB_SPACK_ROOT)/etc/spack/ && \
	  cp $(MG_XLIB)/memgaze.patch $(MG_XLIB_SPACK_ROOT)/var/spack/repos/builtin/packages/dyninst && \
	  cp $(MG_XLIB)/package.py $(MG_XLIB_SPACK_ROOT)/var/spack/repos/builtin/packages/dyninst && \
	  cp $(MG_XLIB)/packages.yaml $(MG_XLIB_SPACK_ROOT)/etc/spack/
	  #cp $(MG_XLIB_ROOT)/hpctoolkit/spack/packages.yaml $(MG_XLIB_SPACK_ROOT)/etc/spack/
	  #git clone https://github.com/hpctoolkit/hpctoolkit.git && \


xlib_dyninst_patch:
	rm $(MG_XLIB_SPACK_ROOT)/etc/spack/packages.yaml && \
	$(MG_XLIB_SPACK_ROOT)/bin/spack install --reuse --source --overwrite -y dyninst@12.0.1
#  patch binaryEdit.C dyninst.patch
#  rebuild dyninst

xlib_hpctoolkit:
	$(MG_XLIB_SPACK_ROOT)/bin/spack install --reuse  hpctoolkit@2022.01.15 -papi -mpi

xlib_build: xlib_spack #xlib_dyninst_patch
  ARCH="$(shell $(MG_XLIB_SPACK_ROOT)/bin/spack arch)" && \
	  cd $(MG_XLIB_ROOT) && \
	  $(MG_XLIB_SPACK_ROOT)/bin/spack install --reuse  --source hpctoolkit@2022.01.15 -papi -mpi


xlib_clean:
# delete build files

xlib_distclean: xlib_clean
# delete spack/etc

test:
	  echo $(MG_XLIB)
	  echo $(MG_XLIB_ROOT)


#****************************************************************************
# Template Rules
#****************************************************************************

include ../Makefile-template.mk

#****************************************************************************
# Local Rules
#****************************************************************************

install.local : xlib_build

info.local :

check.local :

clean : xlib_clean

distclean : xlib_distclean

all : xlib_spack xlib_hpctoolkit 
