# -*-Mode: makefile;-*-

#*BeginPNNLCopyright*********************************************************
#
# $HeadURL$
# $Id$
#
#***********************************************************EndPNNLCopyright*

MEMGAZE_ROOT := $(shell pwd)

MG_XLIB = $(MEMGAZE_ROOT)/xlib

# XLIB build root -- might be different
MG_XLIB_ROOT = $(MG_XLIB)
MG_XLIB_SPACK_ROOT = $(MG_XLIB_ROOT)/spack

all: xlib_build

xlib_spack:
	mkdir -p $(MG_XLIB_ROOT) && \
	  cd $(MG_XLIB_ROOT) && \
	  git clone -c feature.manyFiles=true https://github.com/spack/spack.git && \
	  git clone https://github.com/hpctoolkit/hpctoolkit.git && \
	  cp $(MG_XLIB)/config.yaml $(MG_XLIB_SPACK_ROOT)/etc/spack/ && \
	  cp $(MG_XLIB_ROOT)/hpctoolkit/spack/packages.yaml $(MG_XLIB_SPACK_ROOT)/etc/spack/


xlib_dyninst_patch:

xlib_hpctoolkit_xxx:

xlib_build: xlib_spack xlib_dyninst_patch
  ARCH="$(shell $(MG_XLIB_SPACK_ROOT)/bin/spack arch)" && \
	  cd $(MG_XLIB_ROOT) && \
	  $(MG_XLIB_SPACK_ROOT)/bin/spack install --reuse  hpctoolkit@2022.01.15 -papi -mpi


xlib_clean:
# delete build files

xlib_distclean: xlib_clean
# delete spack/etc


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
