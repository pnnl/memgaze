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
SPK_HPCTK = $(XLIB_SPACK)/var/spack/repos/builtin/packages/hpctoolkit

#MY_HPCTK_DEV   = hpctoolkit@develop -papi -mpi
MY_HPCTK_DEV   = hpctoolkit@develop -papi -mpi
MY_HPCTK   = hpctoolkit@2022.05.15 -papi -mpi
#MY_DYNINST = dyninst@12.0.1
MY_DYNINST = dyninst@12.1.0


xlib :
ifeq ($(DEVELOP),1)
  xlib : info xlib_spack xlib_hpctoolkit_dev  xlib_hpctk_libs_copy
  #xlib : info xlib_spack xlib_hpctoolkit_dev xlib_dyninst_patch xlib_hpctk_libs_copy
	@$(PRINTF) $(_msg_infoGrp)
else
  xlib : info xlib_spack xlib_hpctoolkit
  #xlib : info xlib_spack xlib_hpctoolkit xlib_dyninst_patch
	@$(PRINTF) $(_msg_infoGrp)
endif

# platform: "$(XLIB_SPACK)/bin/spack arch"


xlib_spack :
	@$(PRINTF) $(_msg_infoGrp)
	mkdir -p $(MG_XLIB_ROOT) && cd $(MG_XLIB_ROOT) && \
	if [ ! -d "$(XLIB_SPACK)" ]; then \
	  git clone --depth 1 --branch v0.20.0 -c feature.manyFiles=true https://github.com/spack/spack.git ; \
	fi && \
	cp $(XLIB_CFG)/config.yaml     $(SPK_ETC)/config.yaml && \
	cp $(XLIB_CFG)/dyninst.py      $(SPK_DYNINST)/package.py && \
	cp $(XLIB_CFG)/dyninst.patch   $(SPK_DYNINST)/memgaze.patch && \
	cp $(XLIB_CFG)/hpctk.py        $(SPK_HPCTK)/package.py && \
	$(XLIB_SPACK)/bin/spack clean -ab && \
	$(XLIB_SPACK)/bin/spack clean -fmps && \
	$(XLIB_SPACK)/bin/spack install ca-certificates-mozilla


xlib_hpctoolkit :
	@$(PRINTF) $(_msg_infoGrp)
	cp $(XLIB_CFG)/hpctk-pkgs.yaml $(SPK_ETC)/packages.yaml && \
	$(XLIB_SPACK)/bin/spack install --reuse --source $(MY_HPCTK) 

xlib_hpctoolkit_dev :
	@$(PRINTF) $(_msg_infoGrp)
	cp $(XLIB_CFG)/hpctk-dev-pkgs.yaml $(SPK_ETC)/packages.yaml && \
	$(XLIB_SPACK)/bin/spack install --reuse --source --keep-prefix --keep-stage --overwrite -y $(MY_HPCTK_DEV) && \
	$(XLIB_SPACK)/bin/spack install --reuse binutils@2.38 && \
	$(XLIB_SPACK)/bin/spack install --reuse libdwarf@20180129  

xlib_dyninst_patch :
	@$(PRINTF) $(_msg_infoGrp)
	$(RM) $(SPK_ETC)/packages.yaml && \
	cp $(XLIB_CFG)/dyninst.py      $(SPK_DYNINST)/package.py && \
	cp $(XLIB_CFG)/dyninst.patch   $(SPK_DYNINST)/memgaze.patch && \
	$(XLIB_SPACK)/bin/spack install --reuse --source --overwrite -y $(MY_DYNINST) 


xlib_hpctk_libs_copy:
	$(eval HPCTK_STAGE_DIR := $(shell $(XLIB_SPACK)/bin/spack location -b hpctoolkit@develop))
	cp $(HPCTK_STAGE_DIR)/src/include/hpctoolkit-config.h $(HPCTK_DEV_SRC)/include
	cp $(HPCTK_STAGE_DIR)/src/lib/prof/.libs/$(HPCTKLIB_Prof_a) $(HPCTKLIB_Prof) && \
	cp $(HPCTK_STAGE_DIR)/src/lib/profile/.libs/$(HPCTKLIB_Profile_a) $(HPCTKLIB_Profile) && \
	cp $(HPCTK_STAGE_DIR)/src/lib/profile/.libs/$(HPCTKLIB_ProfileStandalone_a) $(HPCTKLIB_ProfileStandalone) && \
	cp $(HPCTK_STAGE_DIR)/src/lib/prof-lean/.libs/$(HPCTKLIB_ProfLean_a) $(HPCTKLIB_ProfLean) && \
	cp $(HPCTK_STAGE_DIR)/src/lib/binutils/.libs/$(HPCTKLIB_Binutils_a) $(HPCTKLIB_Binutils) && \
	cp $(HPCTK_STAGE_DIR)/src/lib/support/.libs/$(HPCTKLIB_Support_a) $(HPCTKLIB_Support) && \
	cp $(HPCTK_STAGE_DIR)/src/lib/support-lean/.libs/$(HPCTKLIB_SupportLean_a) $(HPCTKLIB_SupportLean) 
	$(XLIB_SPACK)/bin/spack clean --stage && \
	$(XLIB_SPACK)/bin/spack clean -ab 



xlib_clean : info
# delete build files

xlib_distclean : info xlib_clean
	$(RM) -r $(MG_XLIB_ROOT)


#****************************************************************************

#MG_PERF = $(MG_PERF_ROOT)/linux-5.5.9
MG_PERF = $(MG_PERF_ROOT)/linux-5.18.14

perf_libs: 
	$(XLIB_SPACK)/bin/spack install --reuse zstd@1.5.2 libcap@2.64 babeltrace@1.2.4 && \
	export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(MG_XLIB_ROOT)/zstd-1.5.2/lib:$(MG_XLIB_ROOT)/libcap-2.64/lib:$(MG_XLIB_ROOT)/babeltrace-1.2.4/lib 


perf : info
	@$(PRINTF) $(_msg_infoGrp)
	mkdir -p $(MG_PERF_ROOT) && \
	cd $(MG_PERF_ROOT) && \
	if [[ ! -d "$(MG_PERF)" ]] ; then \
	wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.18.14.tar.xz && \
	  tar -xf linux-5.18.14.tar.xz && rm linux-5.18.14.tar.xz && \
	  patch $(MG_PERF)/tools/perf/util/scripting-engines/trace-event-python.c $(MG_XLIB_SRC)/config-perf/perf-script.patch ; \
	fi && \
  \
	[[ -n "$(MG_PERF_CC)" ]] && export HOSTCC="$(MG_PERF_CC)" ; \
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

#ifdef DEVELOP
#  install.local : xlib_dev perf
#else
#  install.local : xlib perf
#endif


info.local :
	@$(PRINTF) $(msg_info)

check.local :

clean : xlib_clean

distclean : xlib_distclean perf_distclean
