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

# Auto-discover root directory (may need make 3.81+)
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

MEMGAZE_MIAMI_ROOT := $(MEMGAZE_ROOT)/bin-anlys
MIAMI_TARGET       = $(MEMGAZE_MIAMI_ROOT)/install


#-----------------------------------------------------------
# MemGaze dependences
#-----------------------------------------------------------

MG_XLIB_ROOT ?= $(MG_XLIB_SRC)/lib

DYNINST_ROOT = $(MG_XLIB_ROOT)/dyninst-12.0.1
DYNINST_INC  = $(DYNINST_ROOT)/include
DYNINST_LIB  = $(DYNINST_ROOT)/lib

LIBDWARF_LIB = $(MG_XLIB_ROOT)/libdwarf-20180129/lib

LIBELF_LIB = $(MG_XLIB_ROOT)/elfutils-0.186/lib
LIBELF_INC = $(MG_XLIB_ROOT)/elfutils-0.186/include

BOOST_ROOT = $(MG_XLIB_ROOT)/boost-1.77.0
BOOST_INC  = $(BOOST_ROOT)/include
BOOST_LIB  = $(BOOST_ROOT)/lib

XED_ROOT = $(MG_XLIB_ROOT)/intel-xed-12.0.1
XED_INC  = $(XED_ROOT)/include
XED_LIB  = $(XED_ROOT)/lib

BINUTILS_ROOT = $(MG_XLIB_ROOT)/binutils-2.36.1
BINUTILS_INC  = $(BINUTILS_ROOT)/include

LIBIBERTY_INC = $(BINUTILS_ROOT)/include/libiberty/

TBB_ROOT = $(MG_XLIB_ROOT)/intel-tbb-2020.3
TBB_INC  = $(TBB_ROOT)/include
TBB_LIB  = $(TBB_ROOT)/lib

#-----------------------------------------------------------

MG_PERF_ROOT ?= $(MG_XLIB_SRC)/perf


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
