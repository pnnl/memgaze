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
##include miami.config

#****************************************************************************

MIAMI_TARGET ?= $(MEMGAZE_MIAMI_ROOT)

VAR_FILES = $(wildcard $(MIAMI_TARGET)/etc/vars/vars_*)


#****************************************************************************
# Template Rules
#****************************************************************************

include ../Makefile-template.mk

#****************************************************************************
# Local Rules
#****************************************************************************

all:
ifneq ($(VAR_FILES),)
	rm $(VAR_FILES)
endif
	cd src && $(MAKE) $@ MIAMI_KIT=1
#	@echo "Installing out of tree"
	mkdir -p $(MIAMI_TARGET)

clean cleanall info:
	cd src && $(MAKE) $@ MIAMI_KIT=1

distclean: cleanall

install.local :

