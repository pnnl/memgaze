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


#****************************************************************************
# Recursion
#****************************************************************************

# MK_SUBDIRS =


#****************************************************************************
# Xlib
#****************************************************************************

MG_XLIB_SPACK_ROOT = $(MG_XLIB_ROOT)/spack

XLIB_HPCTK = hpctoolkit@2022.01.15 -papi -mpi

xlib_build: xlib_spack xlib_dyninst_patch xlib_hpctoolkit
  ARCH="$(shell $(MG_XLIB_SPACK_ROOT)/bin/spack arch)" && \
	cd $(MG_XLIB_ROOT) && \
	$(MG_XLIB_SPACK_ROOT)/bin/spack install --reuse --source $(XLIB_HPCTK)


xlib_spack:
	mkdir -p $(MG_XLIB_ROOT) && \
	cd $(MG_XLIB_ROOT) && \
	if [ ! -d "$(MG_XLIB_SPACK_ROOT)" ]; then \
	  git clone -c feature.manyFiles=true https://github.com/spack/spack.git; \
	fi && \
	cp $(MG_XLIB)/config.yaml $(MG_XLIB_SPACK_ROOT)/etc/spack/ && \
	cp $(MG_XLIB)/memgaze.patch $(MG_XLIB_SPACK_ROOT)/var/spack/repos/builtin/packages/dyninst && \
	cp $(MG_XLIB)/package.py $(MG_XLIB_SPACK_ROOT)/var/spack/repos/builtin/packages/dyninst && \
	cp $(MG_XLIB)/packages.yaml $(MG_XLIB_SPACK_ROOT)/etc/spack/


xlib_dyninst_patch:
	rm $(MG_XLIB_SPACK_ROOT)/etc/spack/packages.yaml && \
	$(MG_XLIB_SPACK_ROOT)/bin/spack install --reuse --source --overwrite -y dyninst@12.0.1
#  patch binaryEdit.C dyninst.patch
#  rebuild dyninst


xlib_hpctoolkit:
	$(MG_XLIB_SPACK_ROOT)/bin/spack install --reuse $(XLIB_HPCTK)


xlib_clean:
# delete build files

xlib_distclean: xlib_clean
	$(RM) -r $(MG_XLIB_ROOT)


#****************************************************************************

MG_PERF = $(PERF_ROOT)/linux-5.5.9

perf:
	mkdir -p $(PERF_ROOT) && \
  cd $(PERF_ROOT) && \
  if [[ ! -d "$(MG_PERF)" ]]; then \
	  wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.5.9.tar.gz && \
	  tar -xf linux-5.5.9.tar.gz && rm linux-5.5.9.tar.gz && \
	  patch $(MG_PERF)/tools/perf/util/scripting-engines/trace-event-python.c $(MG_XLIB)/linux-perf/perf-script.patch; \
	fi && \
  cd $(MG_PERF)/tools/perf && make

perf_distclean:
	$(RM) -r $(PERF_ROOT)


#****************************************************************************
# Template Rules
#****************************************************************************

include ../Makefile-template.mk

#****************************************************************************
# Local Rules
#****************************************************************************

install.local : xlib_build perf

info.local :
	echo $(MG_XLIB)
	echo $(MG_XLIB_ROOT)

check.local :

clean : xlib_clean

distclean : xlib_distclean perf_distclean
