# -*-Mode: makefile;-*-
include ../Makefile-defs.mk
include miami.config

MIAMI_TARGET ?= $(MEMGAZE_MIAMI_ROOT)
VAR_FILES = $(wildcard $(MIAMI_TARGET)/etc/vars/vars_*)


all:
	@# remove any existing individual vars files
ifneq ($(VAR_FILES),)
	rm $(VAR_FILES)
endif
	cd src && $(MAKE) $@ MIAMI_KIT=1
ifneq ($(MIAMI_TARGET),$(MEMGAZE_MIAMI_ROOT))
	@echo "Installing out of tree"
	mkdir -p $(MIAMI_TARGET)
	@echo "Copying documentation"
#	@cp -r doc README LICENSE ACKNOWLEDGEMENTS $(MIAMI_TARGET)
	@echo "Copying utilities"
#	@cp -r share ExtractSourceFiles $(MIAMI_TARGET)
	@echo "Copying Viewer"
#	@cp -r Viewer $(MIAMI_TARGET)
else
	@echo "Installing in place"
endif

clean cleanall info:
	cd src && $(MAKE) $@ MIAMI_KIT=1

distclean: cleanall
