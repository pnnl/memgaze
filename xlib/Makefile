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

XLIB_SPACK = $(MG_XLIB_ROOT)/spack

SPACK_HPCTK = hpctoolkit@2022.01.15 -papi -mpi


xlib: xlib_spack xlib_hpctoolkit xlib_dyninst_patch
	@$(PRINTF) $(_msg_infoGrp)


xlib_spack:
	@$(PRINTF) $(_msg_infoGrp)
	mkdir -p $(MG_XLIB_ROOT) && cd $(MG_XLIB_ROOT) && \
	if [ ! -d "$(XLIB_SPACK)" ]; then \
	  git clone -c feature.manyFiles=true https://github.com/spack/spack.git ; \
	fi && \
	cp $(MG_XLIB_SRC)/config-lib/config.yaml $(XLIB_SPACK)/etc/spack/ && \
	cp $(MG_XLIB_SRC)/config-lib/memgaze.patch $(XLIB_SPACK)/var/spack/repos/builtin/packages/dyninst && \
	cp $(MG_XLIB_SRC)/config-lib/package.py $(XLIB_SPACK)/var/spack/repos/builtin/packages/dyninst && \
	cp $(MG_XLIB_SRC)/config-lib/hpctoolkit-packages.yaml $(XLIB_SPACK)/etc/spack/packages.yaml

# platform: "$(XLIB_SPACK)/bin/spack arch"


xlib_hpctoolkit:
	@$(PRINTF) $(_msg_infoGrp)
	$(XLIB_SPACK)/bin/spack install --reuse --source $(SPACK_HPCTK)


xlib_dyninst_patch:
	@$(PRINTF) $(_msg_infoGrp)
	$(RM) $(XLIB_SPACK)/etc/spack/packages.yaml && \
	$(XLIB_SPACK)/bin/spack install --reuse --source --overwrite -y dyninst@12.0.1


xlib_clean:
# delete build files

xlib_distclean: xlib_clean
	$(RM) -r $(MG_XLIB_ROOT)


#****************************************************************************

MG_PERF = $(MG_PERF_ROOT)/linux-5.5.9

perf:
	@$(PRINTF) $(_msg_infoGrp)
	mkdir -p $(MG_PERF_ROOT) && \
  cd $(MG_PERF_ROOT) && \
  if [[ ! -d "$(MG_PERF)" ]] ; then \
	  wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.5.9.tar.gz && \
	  tar -xf linux-5.5.9.tar.gz && rm linux-5.5.9.tar.gz && \
	  patch $(MG_PERF)/tools/perf/util/scripting-engines/trace-event-python.c $(MG_XLIB_SRC)/config-perf/perf-script.patch ; \
	fi && \
  cd $(MG_PERF)/tools/perf && make

perf_distclean:
	$(RM) -r $(MG_PERF_ROOT)


#****************************************************************************
# Template Rules
#****************************************************************************

include ../Makefile-template.mk

#****************************************************************************
# Local Rules
#****************************************************************************

install.local : xlib perf

info.local :
	@$(PRINTF) "MG_XLIB_ROOT=$(MG_XLIB_ROOT)\n"
	@$(PRINTF) "MG_PERF_ROOT=$(MG_PERF_ROOT)\n"

check.local :

clean : xlib_clean

distclean : xlib_distclean perf_distclean
