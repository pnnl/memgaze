# -*-Mode: makefile;-*-

#*BeginPNNLCopyright*********************************************************
#
# $HeadURL$
# $Id$
#
#***********************************************************EndPNNLCopyright*

#****************************************************************************
# Nathan Tallent
#****************************************************************************


#****************************************************************************
# Package defs
#****************************************************************************

include ../../Makefile-defs.mk

#****************************************************************************
# Recursion
#****************************************************************************

MK_SUBDIRS =


#****************************************************************************
# Memory Analysis
#****************************************************************************

#----------------------------------------------------------------------------
# Check
#----------------------------------------------------------------------------

mg_analyze := ../memgaze-analyze

sfx_out   := .out
sfx_outoe := .out-oe

sfx_gld   := .gold
sfx_gldoe := .gold-oe

#****************************************************************************

MK_CHECK = code_lbr # actor_lbr

#code_lbr_CHECK := \
	ubench-O3-n500k-buf8k-p100k$(sfx_out)
# 10000000

code_lbr_CHECK := \
	ubench-O3-n500k-buf8k-p100000$(sfx_out) \
	minivite-v1-O3-n300k-buf8k-p10000000-part1$(sfx_out) \
	minivite-v1-O3-n300k-buf8k-p10000000-part2$(sfx_out) \
	minivite-v3-O3-n300k-buf8k-p10000000-part1$(sfx_out) \
	minivite-v3-O3-n300k-buf8k-p10000000-part2$(sfx_out)

code_lbr_CHECK_BASE = $(patsubst %$(sfx_out),%,$(1))

code_lbr_RUN = \
  [[ $${chk_base} =~ -p([[:digit:]]+) ]] && \
  $(mg_analyze) \
    -t ./$${chk_base}/$${chk_base}.trace \
    -c ./$${chk_base}/$${chk_base}.callpath \
    -l ./$${chk_base}/$${chk_base}.binanlys \
    -s ./$${chk_base}/$${chk_base}.hpcstruct \
    -o $@ \
    -m 1 -p $${BASH_REMATCH[1]} \
    >& $${chk_base}$(sfx_outoe)

code_lbr_RUN_DIFF = \
  diff -C0 -N $*$(sfx_out)   ./$*/$*$(sfx_gld)   >  $@ && \
  diff -C0 -N $*$(sfx_outoe) ./$*/$*$(sfx_gldoe) >> $@

code_lbr_RUN_UPDATE = \
  mv $*$(sfx_out)   ./$*/$*$(sfx_gld) && \
  mv $*$(sfx_outoe) ./$*/$*$(sfx_gldoe)

code_lbr_CLEAN := $(patsubst %$(sfx_out),%$(sfx_outoe),$(code_lbr_CHECK))

# FIXME: clean will not capture the $(sfx_outoe) file

#****************************************************************************
# Template Rules
#****************************************************************************

include ../../Makefile-template.mk


#****************************************************************************
# Local Rules
#****************************************************************************

info.local :

check.local :
