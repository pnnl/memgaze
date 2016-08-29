SUBDIRS = CFGtool MemReuse CacheSim StreamSim Scheduler 

all clean cleanall distclean info:
	@for dir in $(SUBDIRS); \
	do \
	   echo "Running \"make $@\" in $$dir"; \
	   (cd $$dir && $(MAKE) $@ MIAMI_KIT=1); \
	done
