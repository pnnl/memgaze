# -*-Mode: makefile;-*-

include ../miami.config

#****************************************************************************

TEST_STEMS = \
	test1 \
	test2 \
	test2-1 \
	test2-2 \
	test2-3 \
	test2-4 \
	test2-5 \
	\
	petsc-matmult \
	petsc-matmult-inner-loop \
	petsc-matmult-inner-loop-2its

TEST_SCHED_DOT = $(addsuffix $(SCHED_STEM).dot, $(TEST_STEMS))

TEST_SCHED_PDF = $(addsuffix $(SCHED_STEM).pdf, $(TEST_STEMS))

TEST_SCHED_SVG = $(addsuffix $(SCHED_STEM).svg, $(TEST_STEMS))


#----------------------------------------------------------------------------

SCHED_STEM = .schedule

#----------------------------------------------------------------------------

PALM_MIAMI = \
	$(PALM_MIAMI_ROOT)/install/bin/miami

MACHINE_MODEL = \
	$(PALM_MIAMI_ROOT)/share/machines/x86_SandyBridge_EP.mdf

GRAPHVIZ_DOT = /home/frie869/pkg/graphviz-2.40.1/bin/dot


#****************************************************************************

all : $(TEST_SCHED_PDF) # $(TEST_SCHED_SVG)

clean :
	$(RM) $(TEST_SCHED_DOT)
	$(RM) $(TEST_SCHED_PDF)
#	$(RM) $(TEST_SCHED_SVG)
	$(RM) *[0-9]-units.xml
	$(RM) *[0-9].xml

#----------------------------------------------------------------------------

%$(SCHED_STEM).pdf : %$(SCHED_STEM).dot
	$(GRAPHVIZ_DOT) -Tpdf $< -o $@

%$(SCHED_STEM).svg : %$(SCHED_STEM).dot
	$(GRAPHVIZ_DOT) -Tsvg $< -o $@

#----------------------------------------------------------------------------

# TODO: variants with and without memory lat file

test1$(SCHED_STEM).dot : %$(SCHED_STEM).dot :
	$(PALM_MIAMI) \
	  --func _Z5test1ii \
	  --bin_path ./$*.x86_64-linux.x \
	  --lat_path ./$*.memprof \
	  -m $(MACHINE_MODEL) \
	  --blk_path "1" \
	  --dump_file $@

test2$(SCHED_STEM).dot : %$(SCHED_STEM).dot :
	$(PALM_MIAMI) \
	  --func _Z13randomAccess3P4nodeii \
	  --bin_path ./$*.x86_64-linux.x \
	  --lat_path ./$*.memprof \
	  -m $(MACHINE_MODEL) \
	  --blk_path "0" \
	  --dump_file $@

test2-1$(SCHED_STEM).dot : %$(SCHED_STEM).dot :
	$(PALM_MIAMI) \
	  --func _Z13randomAccess3P4nodeii \
	  --bin_path ./$*.x86_64-linux.x \
	  --lat_path ./$*.memprof \
	  -m $(MACHINE_MODEL) \
	  --blk_path "1" \
	  --dump_file $@

test2-2$(SCHED_STEM).dot : %$(SCHED_STEM).dot :
	$(PALM_MIAMI) \
	  --func _Z13randomAccess3P4nodeii \
	  --bin_path ./$*.x86_64-linux.x \
	  --lat_path ./$*.memprof \
	  -m $(MACHINE_MODEL) \
	  --blk_path "2" \
	  --dump_file $@

test2-3$(SCHED_STEM).dot : %$(SCHED_STEM).dot :
	$(PALM_MIAMI) \
	  --func _Z13randomAccess3P4nodeii \
	  --bin_path ./$*.x86_64-linux.x \
	  --lat_path ./$*.memprof \
	  -m $(MACHINE_MODEL) \
	  --blk_path "0->1" \
	  --dump_file $@

test2-4$(SCHED_STEM).dot : %$(SCHED_STEM).dot :
	$(PALM_MIAMI) \
	  --func _Z13randomAccess3P4nodeii \
	  --bin_path ./$*.x86_64-linux.x \
	  --lat_path ./$*.memprof \
	  -m $(MACHINE_MODEL) \
	  --blk_path "0->2" \
	  --dump_file $@

test2-5$(SCHED_STEM).dot : %$(SCHED_STEM).dot :
	$(PALM_MIAMI) \
	  --func _Z13randomAccess3P4nodeii \
	  --bin_path ./$*.x86_64-linux.x \
	  --lat_path ./$*.memprof \
	  -m $(MACHINE_MODEL) \
	  --blk_path "3" \
	  --dump_file $@


#----------------------------------------------------------------------------
# PFLOTRAN / PetSc / MatMult_SeqBAIJ_1
#----------------------------------------------------------------------------

# examine entry block
petsc-matmult$(SCHED_STEM).dot : %$(SCHED_STEM).dot :
	$(PALM_MIAMI) \
	  --func MatMult_SeqBAIJ_1 \
	  --bin_path ./petsc.x86_64-linux.x \
	  --lat_path ./petsc-matmult.memprof \
	  -m $(MACHINE_MODEL) \
	  --blk_path "0" \
	  --dump_file $@

# examine the block in the innermost loop
petsc-matmult-inner-loop$(SCHED_STEM).dot : %$(SCHED_STEM).dot :
	$(PALM_MIAMI) \
	  --func MatMult_SeqBAIJ_1 \
	  --bin_path ./petsc.x86_64-linux.x \
	  --lat_path ./petsc-matmult.memprof \
	  -m $(MACHINE_MODEL) \
	  --blk_path "21" \
	  --dump_file $@

# "unroll" innermost loop twice
petsc-matmult-inner-loop-2its$(SCHED_STEM).dot : %$(SCHED_STEM).dot :
	$(PALM_MIAMI) \
	  --func MatMult_SeqBAIJ_1 \
	  --bin_path ./petsc.x86_64-linux.x \
	  --lat_path ./petsc-matmult.memprof \
	  -m $(MACHINE_MODEL) \
	  --blk_path "21->21" \
	  --dump_file $@

