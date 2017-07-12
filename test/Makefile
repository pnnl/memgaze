# -*-Mode: makefile;-*-

include ../miami.config

#****************************************************************************

TEST_STEMS = \
	test1 \
	test2

TEST_SCHED = $(addsuffix -schedule.dot, $(TEST_STEMS))

TEST_SCHED_PDF = $(addsuffix -schedule.pdf, $(TEST_STEMS))

TEST_SCHED_SVG = $(addsuffix -schedule.svg, $(TEST_STEMS))


#****************************************************************************

PALM_MIAMI = \
	$(PALM_MIAMI_ROOT)/install/bin/miami

MACHINE_MODEL = \
	$(PALM_MIAMI_ROOT)/share/machines/x86_SandyBridge_EP.mdf

GRAPHVIZ_DOT = /home/frie869/pkg/graphviz-2.40.1/bin/dot

#----------------------------------------------------------------------------

all : $(TEST_SCHED_PDF) $(TEST_SCHED_SVG)

#----------------------------------------------------------------------------

%-schedule.svg : %-schedule.dot
	$(GRAPHVIZ_DOT) -Tsvg $< -o $@

%-schedule.pdf : %-schedule.dot
	$(GRAPHVIZ_DOT) -Tpdf $< -o $@

#----------------------------------------------------------------------------

# TODO: variants with and without memory lat file

test1-schedule.dot : %-schedule.dot :
	$(PALM_MIAMI) \
	  --func _Z5test1ii \
	  --bin_path ./$*.x86_64-linux.x \
	  --lat_path ./$*.memprof \
	  -m $(MACHINE_MODEL) \
	  --blk_path "1" \
	  --dump_file $@

test2-schedule.dot : %-schedule.dot :
	$(PALM_MIAMI) \
	  --func _Z13randomAccess3P4nodeii \
	  --bin_path ./$*.x86_64-linux.x \
	  --lat_path ./$*.memprof \
	  -m $(MACHINE_MODEL) \
	  --blk_path "0" \
	  --dump_file $@

clean :
	$(RM) *[0-9]-units.xml
	$(RM) *[0-9].xml
	$(RM) *.dot
	$(RM) *.pdf *.svg
