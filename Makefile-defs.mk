# -*-Mode: makefile;-*-

#*BeginPNNLCopyright*********************************************************
#
# $HeadURL$
# $Id$
#
#***********************************************************EndPNNLCopyright*

#****************************************************************************
# $HeadURL$
#
# Nathan Tallent
#****************************************************************************

# auto-discover root directory (may need make 3.81+)
mkfile_defs := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))

mkfile_defs_root := $(abspath $(dir $(mkfile_defs)))


#****************************************************************************

MEMGAZE_ROOT = $(mkfile_defs_root)

PREFIX        ?= $(MEMGAZE_ROOT)/install
PREFIX_BIN     = $(PREFIX)/bin
PREFIX_LIBEXEC = $(PREFIX)/libexec


#-----------------------------------------------------------
# MemGaze source/internal structure
#-----------------------------------------------------------

MG_XLIB_SRC  = $(MEMGAZE_ROOT)/xlib

# TODO: clean
MEMGAZE_MIAMI_ROOT := $(MEMGAZE_ROOT)/bin-anlys
#MIAMI_TARGET       = $(MEMGAZE_MIAMI_ROOT)/install
MIAMI_TARGET       = $(PREFIX_LIBEXEC)


#-----------------------------------------------------------
# MemGaze dependences: libraries
#-----------------------------------------------------------

MG_XLIB_ROOT ?= $(MG_XLIB_SRC)/lib

#-----------------------------------------------------------

#HPCTK_ROOT = $(MG_XLIB_ROOT)/hpctoolkit-2022.01.15
HPCTK_ROOT = $(MG_XLIB_ROOT)/hpctoolkit-2022.05.15
HPCTK_LIB  = $(HPCTK_ROOT)/lib
HPCTK_SRC  = $(HPCTK_ROOT)/share/hpctoolkit/src/src

#HPCTK_DEV_ROOT = $(MG_XLIB_ROOT)/hpctoolkit-develop
HPCTK_DEV_ROOT = $(MG_XLIB_ROOT)/hpctoolkit-develop.commit
HPCTK_DEV_LIB = $(HPCTK_DEV_ROOT)/lib
HPCTK_DEV_SRC = $(HPCTK_DEV_ROOT)/share/hpctoolkit/src/src
#-----------------------------------------------------------

# ---------- HPCToolkit LIBS ----------
HPCTKLIB_Profile 		= $(HPCTK_DEV_SRC)/lib/profile
HPCTKLIB_Profile_a 		= libHPCprofile.a 
HPCTKLIB_ProfileStandalone 	= $(HPCTK_DEV_SRC)/lib/profile
HPCTKLIB_ProfileStandalone_a 	= libHPCprofile_standalone.a
HPCTKLIB_ProfLean    		= $(HPCTK_DEV_SRC)/lib/prof-lean
HPCTKLIB_ProfLean_a 		= libHPCprof-lean.a
HPCTKLIB_SupportLean 		= $(HPCTK_DEV_SRC)/lib/support-lean
HPCTKLIB_SupportLean_a 		= libHPCsupport-lean.a
HPCTKLIB_Support     		= $(HPCTK_DEV_SRC)/lib/support
HPCTKLIB_Support_a 		= libHPCsupport.a
HPCTKLIB_Binutils    		= $(HPCTK_DEV_SRC)/lib/binutils
HPCTKLIB_Binutils_a 		= libHPCbinutils.a
HPCTKLIB_Prof        		= $(HPCTK_DEV_SRC)/lib/prof
HPCTKLIB_Prof_a 		= libHPCprof.a

BOOST_ROOT = $(MG_XLIB_ROOT)/boost-1.77.0
BOOST_INC  = $(BOOST_ROOT)/include
BOOST_LIB  = $(BOOST_ROOT)/lib

TBB_ROOT = $(MG_XLIB_ROOT)/intel-tbb-2020.3
TBB_INC  = $(TBB_ROOT)/include
TBB_LIB  = $(TBB_ROOT)/lib

#DYNINST_ROOT = $(MG_XLIB_ROOT)/dyninst-12.0.1
DYNINST_ROOT = $(MG_XLIB_ROOT)/dyninst-12.1.0
DYNINST_INC  = $(DYNINST_ROOT)/include
DYNINST_LIB  = $(DYNINST_ROOT)/lib

LIBDWARF_ROOT = $(MG_XLIB_ROOT)/libdwarf-20180129
LIBDWARF_INC = $(LIBDWARF_ROOT)/include
LIBDWARF_LIB = $(LIBDWARF_ROOT)/lib

LIBELF_ROOT = $(MG_XLIB_ROOT)/elfutils-0.186
LIBELF_INC = $(LIBELF_ROOT)/include
LIBELF_LIB = $(LIBELF_ROOT)/lib

LIBZ_ROOT = $(MG_XLIB_ROOT)/zlib-1.2.12
LIBZ_INC = $(LIBZ_ROOT)/include
LIBZ_LIB = $(LIBZ_ROOT)/lib

#XED_ROOT = $(MG_XLIB_ROOT)/intel-xed-12.0.1
XED_ROOT = $(MG_XLIB_ROOT)/intel-xed-2022.04.17
XED_INC  = $(XED_ROOT)/include
XED_LIB  = $(XED_ROOT)/lib

#BINUTILS_ROOT = $(MG_XLIB_ROOT)/binutils-2.36.1
BINUTILS_ROOT = $(MG_XLIB_ROOT)/binutils-2.38
BINUTILS_INC  = $(BINUTILS_ROOT)/include

#LIBIBERTY
LIBIBERTY_INC = $(BINUTILS_ROOT)/include/libiberty/
LIBIBERTY_LIB = $(BINUTILS_ROOT)/lib64

#LZMA ROOT
LZMA_ROOT = $(MG_XLIB_ROOT)/xz-5.2.5
LZMA_LIB = $(LZMA_ROOT)/lib/liblzma.a

XERCES_ROOT = $(MG_XLIB_ROOT)/xerces-c-3.2.3
XERCES_LIB = $(XERCES_ROOT)/lib

#-----------------------------------------------------------
# MemGaze dependences: Linux Perf
#-----------------------------------------------------------

MG_PERF_ROOT ?= $(MG_XLIB_SRC)/perf


#****************************************************************************
# 
#****************************************************************************

msg_info = "$(_msg_sep2)\nPREFIX=$(PREFIX)\nMG_XLIB_ROOT=$(MG_XLIB_ROOT)\nMG_PERF_ROOT=$(MG_PERF_ROOT)\nMG_PERF_CC=$(MG_PERF_CC)\nDEVELOP=$(DEVELOP)\n$(_msg_sep2)\n"

#****************************************************************************
# 
#****************************************************************************

# mk_foreach_file_pair: Given (a) a list of file pairs "src tgt" where
#   'src' is a source (must exist) and 'tgt' is a target; and (b) a
#   command; execute "<command> <src> <tgt>"
define mk_foreach_file_pair
  @list=( $1 ) ; \
  size=$${#list[@]} ; \
  cmd="$2" ; \
  for ((i = 0; i < $${size}; i = i + 2)) ; do \
    ((i_src = i)) ; \
    ((i_tgt = i + 1)) ; \
    x_src=$${list[$$i_src]} ; \
    x_tgt=$${list[$$i_tgt]} ; \
    printf "$${cmd} $${x_src} $${x_tgt}" ; \
    if test -f $${x_src} -o -d $${x_src} ; then \
      printf "\n" ; \
      $${cmd} $${x_src} $${x_tgt} ; \
    else \
      printf " (skipping)\n" ; \
    fi \
  done
endef
