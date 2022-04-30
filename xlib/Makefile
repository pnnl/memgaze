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

XLIB_CFG   = $(MG_XLIB_SRC)/config-lib

SPK_ETC     = $(XLIB_SPACK)/etc/spack
SPK_DYNINST = $(XLIB_SPACK)/var/spack/repos/builtin/packages/dyninst

MY_HPCTK   = hpctoolkit@2022.01.15 -papi -mpi
MY_DYNINST = dyninst@12.0.1


xlib : info xlib_spack xlib_hpctoolkit xlib_dyninst_patch
	@$(PRINTF) $(_msg_infoGrp)
# platform: "$(XLIB_SPACK)/bin/spack arch"


xlib_spack :
	@$(PRINTF) $(_msg_infoGrp)
	mkdir -p $(MG_XLIB_ROOT) && cd $(MG_XLIB_ROOT) && \
	if [ ! -d "$(XLIB_SPACK)" ]; then \
	  git clone -c feature.manyFiles=true https://github.com/spack/spack.git ; \
	fi && \
	cp $(XLIB_CFG)/config.yaml     $(SPK_ETC)/ && \
	cp $(XLIB_CFG)/hpctk-pkgs.yaml $(SPK_ETC)/packages.yaml && \
	cp $(XLIB_CFG)/dyninst.py      $(SPK_DYNINST)/package.py && \
	cp $(XLIB_CFG)/dyninst.patch   $(SPK_DYNINST)/memgaze.patch


xlib_hpctoolkit :
	@$(PRINTF) $(_msg_infoGrp)
	$(XLIB_SPACK)/bin/spack install --reuse --source $(MY_HPCTK)


xlib_dyninst_patch :
	@$(PRINTF) $(_msg_infoGrp)
	$(RM) $(SPK_ETC)/packages.yaml && \
	$(XLIB_SPACK)/bin/spack install --reuse --source --overwrite -y $(MY_DYNINST)


xlib_clean : info
# delete build files

xlib_distclean : info xlib_clean
	$(RM) -r $(MG_XLIB_ROOT)


#****************************************************************************

MG_PERF = $(MG_PERF_ROOT)/linux-5.5.9

perf : info
	@$(PRINTF) $(_msg_infoGrp)
	mkdir -p $(MG_PERF_ROOT) && \
  cd $(MG_PERF_ROOT) && \
  if [[ ! -d "$(MG_PERF)" ]] ; then \
	  wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.5.9.tar.gz && \
	  tar -xf linux-5.5.9.tar.gz && rm linux-5.5.9.tar.gz && \
	  patch $(MG_PERF)/tools/perf/util/scripting-engines/trace-event-python.c $(MG_XLIB_SRC)/config-perf/perf-script.patch ; \
	fi && \
  cd $(MG_PERF)/tools/perf && make

perf_distclean : info
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
	@$(PRINTF) $(msg_info)

check.local :

clean : xlib_clean

distclean : xlib_distclean perf_distclean
