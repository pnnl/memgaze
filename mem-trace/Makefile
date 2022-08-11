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

MK_SUBDIRS =

#****************************************************************************
# Template Rules
#****************************************************************************

include ../Makefile-template.mk

#****************************************************************************
# 
#****************************************************************************

# TODO:
# MK_INSTALL = \
#   memgaze-run
#   memgaze-xtrace

#****************************************************************************
# Local Rules
#****************************************************************************

info.local :

install.local :
	$(INSTALL) -d $(PREFIX_BIN)
	$(INSTALL) -d $(PREFIX_LIBEXEC)
	$(INSTALL) memgaze-inst memgaze-analyze memgaze-analyze-loc memgaze-run memgaze-xtrace $(PREFIX_BIN)
	$(INSTALL) memgaze-inst-cat memgaze-xtrace-normalize perf-script-intel-pt.py  perf-script-intel-ldlat.py $(PREFIX_LIBEXEC)

check.local :
