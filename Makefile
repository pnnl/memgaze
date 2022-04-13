# -*-Mode: makefile;-*-

#*BeginPNNLCopyright*********************************************************
#
# $HeadURL$
# $Id$
#
#***********************************************************EndPNNLCopyright*

MEMGAZE := $(shell pwd)

MEMGAZE_XLIB_ROOT = $(MEMGAZE)/xlib

SPACK := $(MEMGAZE_XLIB_ROOT)/spack/bin/spack

all: xlib_clone xlib_build

xlib_clone:
	mkdir -p $(MEMGAZE_XLIB_ROOT) && \
	  cd $(MEMGAZE_XLIB_ROOT) && \
	  git clone -c feature.manyFiles=true https://github.com/spack/spack.git &&
	  git clone https://github.com/hpctoolkit/hpctoolkit.git

dyninst:

hpctoolkit:

xlib_build:  dyninst
	cd $(MEMGAZE_XLIB_ROOT) && \
	  cp ../config.yaml spack/etc/spack/ && \
	  cp hpctoolkit/spack/packages.yaml  spack/etc/spack/ && \
	  ARCH="$(shell $(SPACK) arch)" && \
	  $(SPACK) install --reuse  hpctoolkit@2022.01.15 -papi -mpi



#TAR = tar
#TARNM = perfect-suite-1.0.1
#
#
#
#dist :
#nm_cur=`basename ${PWD}` ; \
#nm_new=$(TARNM) ; \
#cd .. ; \
#if [[ ! -e $${nm_new} ]] ; then ln -s $${nm_cur} $${nm_new} ; fi ; \
#${TAR} zcvf $${nm_new}.tar.gz \
#-h \
#--exclude=".git" \
#--exclude=".svn" \
#--exclude="suite/sar/tools" \
#--exclude="suite/wami/tools" \
#--exclude="doc/src" \
#$${nm_new}
