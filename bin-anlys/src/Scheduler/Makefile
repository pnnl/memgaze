# -*-Mode: makefile;-*-

include ../../../Makefile-defs.mk
include ../../miami.config

#****************************************************************************

# GNU compiler
DBG ?= -g
OPT ?= -O0

WFLAGS = \
	-Wall \
	-Wno-error \
	-Wno-unknown-pragmas \
	-Wno-unused-function \
	-fno-strict-aliasing

WXXFLAGS = -Woverloaded-virtual

CXXFLAGS += \
	-fPIC $(DBG) $(OPT) $(WFLAGS) $(WXXFLAGS) \
	-I. -I../common -I..

CFLAGS += \
	-fPIC $(DBG) $(OPT) $(WFLAGS) \
	-I. -I../common -I..

COMP_OBJ = -c -o
LINK_EXE = -o

YACC = bison
LEX  = flex

#****************************************************************************

DYNINST_CXXFLAGS = \
	-I$(DYNINST_INC) \
	-I$(TBB_INC) \
	-I$(BOOST_INC) \
	-I$(LIBDWARF_INC) \
	-I$(LIBELF_INC) \
	-I$(BINUTILS_INC) \
	-I$(LIBIBERTY_INC)

DYNINST_LDFLAGS = \
	-L$(DYNINST_LIB)  -Wl,-rpath $(DYNINST_LIB) \
	-L$(TBB_LIB)      -Wl,-rpath $(TBB_LIB) \
	-L$(BOOST_LIB)    -Wl,-rpath $(BOOST_LIB) \
	-L$(LIBDWARF_LIB) -Wl,-rpath $(LIBDWARF_LIB) \
	-L$(LIBELF_LIB)   -Wl,-rpath $(LIBELF_LIB) \
	-ldyninstAPI \
	-lsymtabAPI \
	-lparseAPI \
	-lpatchAPI \
	-linstructionAPI \
	-lstackwalk \
	-ldynElf \
	-lpcontrol \
	-ldynDwarf \
	-lboost_system \
	-lcommon

XED_CFLAGS = -I$(XED_INC)

XED_LDFLAGS = \
	-L$(XED_LIB) -Wl,-rpath $(XED_LIB) \
	-lxed

BINUTILS_CFLAGS = \
	-I$(BINUTILS_INC) \
	-I$(LIBIBERTY_INC)

BINUTILS_LDFLAGS = \
	-L$(BINUTILS_ROOT)/lib -L$(BINUTILS_ROOT)/lib64 \
	-lbfd -liberty -lz

TOOL_CXXFLAGS = \
	$(DYNINST_CXXFLAGS)

TOOL_CFLAGS =

#****************************************************************************

TOOL_ROOTS = memgaze-instrumentor
TOOL_VAR = vars_sched
TOOL_OBJ_DIR = scheduler
BINSCRIPT = $(MEMGAZE_MIAMI_ROOT)/src/tools/run_static.in


MIAMI_TARGET ?= $(MEMGAZE_MIAMI_ROOT)/install
BINDIR = $(MIAMI_TARGET)/bin/
EXEDIR = $(BINDIR)libexec/
#OBJDIR = $(MEMGAZE_MIAMI_ROOT)/objs/
OBJDIR = $(MIAMI_TARGET)/objs/
ETCDIR = $(MIAMI_TARGET)/etc
VARSDIR = $(ETCDIR)/vars
ifeq ($(MIAMI_KIT),1)
   VARSFILE = $(ETCDIR)/miami_vars
else
   VARSFILE = $(ETCDIR)/vars/$(TOOL_VAR)
endif

BINSCRIPT ?= $(MEMGAZE_MIAMI_ROOT)/src/tools/run_tool.in

MIAMI_LIBRARY_PATHS = $(BINUTILS_ROOT)/lib:$(BINUTILS_ROOT)/lib64:$(DYNINST_LIB):$(XED_LIB)

#----------------------------------------------------------------------------

SA_TOOLS   = $(TOOL_ROOTS:%=$(EXEDIR)%)
SA_WRAPPER = $(TOOL_ROOTS:%=$(BINDIR)%)

TOOLS = $(SA_TOOLS) $(SA_WRAPPER)

#****************************************************************************

vpath %.C   $(MEMGAZE_MIAMI_ROOT)/src/Scheduler
vpath %.cpp $(MEMGAZE_MIAMI_ROOT)/src/Scheduler
vpath %.C   $(MEMGAZE_MIAMI_ROOT)/src/common
vpath %.cpp $(MEMGAZE_MIAMI_ROOT)/src/common
vpath %.C   $(MEMGAZE_MIAMI_ROOT)/src/OAUtils
vpath %.C   $(MEMGAZE_MIAMI_ROOT)/src/tarjans

#----------------------------------------------------------------------------

SCHEDULER_OBJS = \
	schedtool.o \
	MiamiDriver.o \
	\
	load_module.o \
	routine.o \
	Instruction.o \
	\
	block_path.o \
	path_id.o \
	\
	PathInfo.o \
	TimeAccount.o \
	schedule_time.o \
	reuse_group.o \
	\
	DGBuilder.o \
	SchedDG.o \
	CycleSetDAG.o \
	dependency_type.o \
	CFG.o \
	Clique.o \
	FindCliques.o \
	\
	memory_latency_histograms.o \
	memory_reuse_histograms.o \
	stream_reuse_histograms.o \
	\
	imix_clustering.o \
	imix_width_clustering.o \
	imix_histograms.o \
	\
	Machine.o \
	MemoryHierarchyLevel.o \
	TemplateExecutionUnit.o \
	UnitRestriction.o \
	InstTemplate.o \
	PatternGraph.o \
	BypassRule.o \
	BitSet.o \
	StringAssocTable.o \
	machdesc.tab.o \
	machdesc.lex.o \
	\
	XML_output.o \
	debug_scheduler.o \
	\
	TemplateInstantiate.o

#   Used headers:
#     GenericInstruction.h
#     MachineExecutionUnit.h
#     CFGDominator.h, DGDominator.h
#     ReplacementRule.h
#     miami_options.h
#     mrd_file_info.h
#     position.h
#     reg_sched_info.h
#     scope_implementation.h
#     scope_reuse_data.h

#   Unused:
#     memory_coverage.[Ch]

#----------------------------------------------------------------------------

COMMON_OBJS = \
	private_load_module.o \
	private_routine.o \
	\
	instruction_class.o \
	instr_bins.o \
	instr_info.o \
	register_class.o \
	canonical_ops.o \
	math_routines.o \
	\
	Dominator.o \
	BaseDominator.o \
	PrivateCFG.o \
	static_branch_analysis.o \
	\
	slice_references.o \
	base_slice.o \
	static_memory_analysis.o \
	\
	InstructionDecoder.o \
	uop_code_cache.o \
	\
	prog_scope.o \
	code_scope.o \
	source_file_mapping_binutils.o \
	file_utilities.o \
	xml.o \
	\
	printable_class.o \
	loadable_class.o \
	\
	miami_utils.o \
	report_time.o \
	miami_globals.o \
	default_values.o \
	Assertion.o \
	debug_miami.o


ifeq (1,1) # if XED is available...
  COMMON_OBJS += InstructionDecoder-xed.o
endif

ifeq (1,1) # if DynInst is available...
  #CXXFLAGS += -DDYNINST_DECODE
  #COMMON_OBJS += InstructionDecoder-dyninst.o
endif


#   Used headers:
#     gnu_demangle.h, config.h
#     charless.h
#     hashmaps.h, bucket_hashmap.h, fast_hashmap.h
#     generic_pair.h, generic_trio.h
#     dense_container.h
#     general_formula.h, generic_math_expression.h
#     instruction_decoding.h
#     InstructionDecoder-xed-iclass.h
#     miami_allocator.h
#     miami_containers.h
#     miami_data_sections.h
#     miami_types.h
#     self_sorted_list.h
#     uipair.h
#     unionfind.h
#     CRCHash.h

#   Unused files:
#     cache_sim.[Ch]
#     block_mapper.h (cache block mapping)
#     mrd_splay_tree.h
#     miami_stack.h
#     miami_growvector.h
#     ranged_set.H

#----------------------------------------------------------------------------

OA_OBJS = \
	BaseGraph.o \
	DGraph.o

#----------------------------------------------------------------------------

TARJ_OBJS = \
	TarjanIntervals.o \
	MiamiRIFG.o \
	UnionFindUniverse.o

#----------------------------------------------------------------------------

OBJS = \
	$(SCHEDULER_OBJS:%=$(OBJDIR)$(TOOL_OBJ_DIR)/%) \
	$(COMMON_OBJS:%=$(OBJDIR)common/%) \
	$(OA_OBJS:%=$(OBJDIR)oautils/%) \
	$(TARJ_OBJS:%=$(OBJDIR)tarjans/%)


#****************************************************************************

all: tools
	mkdir -p $(VARSDIR)
	@echo "MIAMI_LIBRARY_PATHS=$(MIAMI_LIBRARY_PATHS)" >> $(VARSFILE)

tools: $(OBJDIR) $(EXEDIR) $(TOOLS)

$(TOOLS): $(OBJS) $(SAMAIN_OBJS)

$(SA_TOOLS): $(OBJS) $(SAMAIN_OBJS)
	$(LINKER) $(LINK_EXE) $@ $(OBJS) $(SAMAIN_OBJS) $(DBG) \
	  $(DYNINST_LDFLAGS) \
	  $(XED_LDFLAGS) \
	  $(BINUTILS_LDFLAGS) \
	  -Wl,--as-needed -ldl

$(SA_WRAPPER) : $(SA_TOOLS)
	@sed -e 's/XTOOLVARX/$(TOOL_VAR)/g' -e 's/XTOOLLIBX/$(TOOL_ROOTS)/g' $(BINSCRIPT) > $(BINDIR)$(TOOL_ROOTS)
	@chmod +x $(BINDIR)$(TOOL_ROOTS)

#----------------------------------------------------------------------------

clean:
	-rm -rf $(OBJS) $(MAIN_PIN_OBJS) *.out *.tested *.failed makefile.copy *~

cleanall distclean: clean
	-rm -rf $(MIAMI_TARGET) $(OBJDIR)

#----------------------------------------------------------------------------

info:
	@echo "MEMGAZE_MIAMI_ROOT=$(MEMGAZE_MIAMI_ROOT)"
	@echo "MIAMI_TARGET=$(MIAMI_TARGET)"
	@echo "TOOL_OBJ_DIR=$(TOOL_OBJ_DIR)"
	@echo "TOOL_CXXFLAGS=$(TOOL_CXXFLAGS)"
	@echo "COMP_OBJ=$(COMP_OBJ)"
	@echo "DYNINST_ROOT=$(DYNINST_ROOT)"

#****************************************************************************

.force:

$(EXEDIR): .force
	mkdir -p $(BINDIR)
	mkdir -p $(EXEDIR)

$(OBJDIR): .force
	mkdir -p $(OBJDIR)
	mkdir -p $(OBJDIR)$(TOOL_OBJ_DIR)
	mkdir -p $(OBJDIR)common
	mkdir -p $(OBJDIR)oautils
	mkdir -p $(OBJDIR)tarjans

#****************************************************************************

machdesc.tab.c: machine_description.y
	$(YACC) -d -b machdesc machine_description.y

machdesc.tab.h: machine_description.y
	$(YACC) -d -b machdesc machine_description.y

machdesc.lex.c: machine_description.l
	$(LEX) -t machine_description.l > machdesc.lex.c

#----------------------------------------------------------------------------

$(OBJDIR)$(TOOL_OBJ_DIR)/%.o : %.cpp
	$(CXX) $(TOOL_CXXFLAGS) $(CXXFLAGS) $(COMP_OBJ) $@ $<
$(OBJDIR)$(TOOL_OBJ_DIR)/%.o : %.C
	$(CXX) $(TOOL_CXXFLAGS) $(CXXFLAGS) $(COMP_OBJ) $@ $<

$(OBJDIR)$(TOOL_OBJ_DIR)/%.o : %.c
	$(CC) $(TOOL_CFLAGS) $(CFLAGS) $(COMP_OBJ) $@ $<

$(OBJDIR)$(TOOL_OBJ_DIR)/machdesc.tab.o : machdesc.tab.c machdesc.tab.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJDIR)$(TOOL_OBJ_DIR)/machdesc.lex.o : machdesc.lex.c machdesc.tab.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

#----------------------------------------------------------------------------

$(OBJDIR)common/%.o : %.cpp
	$(CXX) $(TOOL_CXXFLAGS) $(CXXFLAGS) $(COMP_OBJ) $@ $<
$(OBJDIR)common/%.o : %.C
	$(CXX) $(TOOL_CXXFLAGS) $(CXXFLAGS) $(COMP_OBJ) $@ $<

$(OBJDIR)common/%.o : %.c
	$(CC) $(TOOL_CFLAGS) $(CFLAGS) $(COMP_OBJ) $@ $<

$(OBJDIR)common/source_file_mapping_binutils.o : source_file_mapping_binutils.C
	$(CXX) $(CXXFLAGS) $(BINUTILS_CFLAGS) $(COMP_OBJ) $@ $<

$(OBJDIR)common/InstructionDecoder-xed.o : InstructionDecoder-xed.cpp
	$(CXX) $(CXXFLAGS) $(XED_CFLAGS) $(COMP_OBJ) $@ $<

$(OBJDIR)common/InstructionDecoder-dyninst.o : InstructionDecoder-dyninst.cpp
	$(CXX) $(CXXFLAGS) $(DYNINST_CXXFLAGS) $(COMP_OBJ) $@ $<

#----------------------------------------------------------------------------

$(OBJDIR)oautils/%.o : %.C
	$(CXX) $(CXXFLAGS) $(COMP_OBJ) $@ $<

#----------------------------------------------------------------------------

$(OBJDIR)tarjans/%.o : %.C
	$(CXX) $(CXXFLAGS) $(COMP_OBJ) $@ $<

#****************************************************************************

