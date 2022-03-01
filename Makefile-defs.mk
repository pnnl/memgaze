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

PALM_ROOT ?= $(mkfile_defs_root)

PALM_EXT_ROOT = $(PALM_ROOT)/../../palm-externals/trunk

PALM_EXT_HPCTKEXT_ROOT ?= $(PALM_EXT_ROOT)/hpctoolkit-externals/x86_64-linux-$(MYCFG_SYSNAMESHORT)

DYNINST_ROOT = $(PALM_EXT_HPCTKEXT_ROOT)/symtabAPI
DYNINST_INC = $(DYNINST_ROOT)/include
DYNINST_LIB = $(DYNINST_ROOT)/lib

LIBDWARF_LIB = $(PALM_EXT_HPCTKEXT_ROOT)/libdwarf/lib

LIBELF_LIB = $(PALM_EXT_HPCTKEXT_ROOT)/libelf/lib

BOOST_ROOT = $(PALM_EXT_HPCTKEXT_ROOT)/boost
BOOST_INC = $(BOOST_ROOT)/include
BOOST_LIB = $(BOOST_ROOT)/lib


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
