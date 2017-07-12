# -*-Mode: makefile;-*-

include ../../miami.config

#****************************************************************************

# GNU compiler
DBG ?= -g
OPT ?= -O3

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

DYNINST_CXXFLAGS = -I$(DYNINST_INC) -I$(BOOST_INC)
DYNINST_LDFLAGS = \
	-L$(DYNINST_LIB) -Wl,-rpath $(DYNINST_LIB) \
	-ldyninstAPI \
	-lsymtabAPI \
	-lparseAPI \
	-lpatchAPI \
	-linstructionAPI \
	-lstackwalk \
	-lpcontrol \
	-ldynElf \
	-ldynDwarf \
	-lcommon

XED_CXXFLAGS = -I$(XED_INC)
XED_LDFLAGS = \
	-L$(XED_LIB) -Wl,-rpath $(XED_LIB) \
	-lxed

BINUTILS_CFLAGS = -I$(BINUTILS_ROOT)/include
BINUTILS_LDFLAGS = \
	-L$(BINUTILS_ROOT)/lib -L$(BINUTILS_ROOT)/lib64 \
	-lbfd -liberty -lz

TOOL_CXXFLAGS = \
	$(DYNINST_CXXFLAGS) \
	$(XED_CXXFLAGS)

TOOL_CFLAGS =

#****************************************************************************

TOOL_ROOTS = miami
TOOL_VAR = vars_sched
TOOL_OBJ_DIR = scheduler
BINSCRIPT = $(PALM_MIAMI_ROOT)/src/tools/run_static.in


MIAMI_TARGET ?= $(PALM_MIAMI_ROOT)/install
BINDIR = $(MIAMI_TARGET)/bin/
EXEDIR = $(BINDIR)libexec/
OBJDIR = $(PALM_MIAMI_ROOT)/objs/
ETCDIR = $(MIAMI_TARGET)/etc
VARSDIR = $(ETCDIR)/vars
ifeq ($(MIAMI_KIT),1)
   VARSFILE = $(ETCDIR)/miami_vars
else
   VARSFILE = $(ETCDIR)/vars/$(TOOL_VAR)
endif

BINSCRIPT ?= $(PALM_MIAMI_ROOT)/src/tools/run_tool.in

MIAMI_LIBRARY_PATHS = $(BINUTILS_ROOT)/lib:$(BINUTILS_ROOT)/lib64:$(DYNINST_LIB):$(XED_LIB)

#----------------------------------------------------------------------------

SA_TOOLS   = $(TOOL_ROOTS:%=$(EXEDIR)%)
SA_WRAPPER = $(TOOL_ROOTS:%=$(BINDIR)%)

TOOLS = $(SA_TOOLS) $(SA_WRAPPER)

#****************************************************************************

vpath %.C   $(PALM_MIAMI_ROOT)/src/Scheduler
vpath %.cpp $(PALM_MIAMI_ROOT)/src/Scheduler
vpath %.C   $(PALM_MIAMI_ROOT)/src/common
vpath %.cpp $(PALM_MIAMI_ROOT)/src/common
vpath %.C   $(PALM_MIAMI_ROOT)/src/OAUtils
vpath %.C   $(PALM_MIAMI_ROOT)/src/tarjans

SCHEDULER_OBJS = \
	schedtool.o \
	MiamiDriver.o \
	debug_scheduler.o \
	\
	load_module.o routine.o \
	CFG.o \
	\
	block_path.o PathInfo.o TimeAccount.o Clique.o FindCliques.o \
	reuse_group.o Instruction.o Machine.o MemoryHierarchyLevel.o \
	PatternGraph.o \
	InstTemplate.o dependency_type.o schedule_time.o StringAssocTable.o \
	\
	imix_histograms.o imix_clustering.o imix_width_clustering.o \
	\
	TemplateExecutionUnit.o UnitRestriction.o BitSet.o BypassRule.o \
	prog_scope.o \
	\
	machdesc.tab.o machdesc.lex.o \
	memory_latency_histograms.o \
	memory_reuse_histograms.o \
	stream_reuse_histograms.o \
	\
	path_id.o CycleSetDAG.o DGBuilder.o SchedDG.o \
	\
	XML_output.o TemplateInstantiate.o

COMMON_OBJS = \
	PrivateCFG.o printable_class.o private_routine.o Assertion.o \
	BaseDominator.o \
	default_values.o xml.o code_scope.o report_time.o \
	source_file_mapping_binutils.o \
	file_utilities.o debug_miami.o loadable_class.o miami_globals.o \
	miami_utils.o \
	private_load_module.o slice_references.o Dominator.o \
	mark_back_edges.o uop_code_cache.o \
	base_slice.o static_memory_analysis.o \
	\
	InstructionDecoder.o instr_info.o instr_bins.o register_class.o \
	\
	static_branch_analysis.o instruction_class.o canonical_ops.o \
	math_routines.o

PIN_COMMON_OBJS = instruction_decoding_pin.o
#instruction_decoding_dyninst.o

OA_OBJS = BaseGraph.o DGraph.o

TARJ_OBJS = TarjanIntervals.o MiamiRIFG.o UnionFindUniverse.o

OBJS = \
	$(SCHEDULER_OBJS:%=$(OBJDIR)$(TOOL_OBJ_DIR)/%) \
	$(COMMON_OBJS:%=$(OBJDIR)common/%) \
	$(PIN_COMMON_OBJS:%=$(OBJDIR)pinobj/%) \
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
	@echo "PALM_MIAMI_ROOT=$(PALM_MIAMI_ROOT)"
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
	mkdir -p $(OBJDIR)pinobj
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

#----------------------------------------------------------------------------

$(OBJDIR)pinobj/%.o : %.cpp
	$(CXX) $(TOOL_CXXFLAGS) $(CXXFLAGS) $(COMP_OBJ) $@ $<
$(OBJDIR)pinobj/%.o : %.C
	$(CXX) $(TOOL_CXXFLAGS) $(CXXFLAGS) $(COMP_OBJ) $@ $<

$(OBJDIR)pinobj/%.o : %.c
	$(CC) $(TOOL_CFLAGS) $(CFLAGS) $(COMP_OBJ) $@ $<

#----------------------------------------------------------------------------

$(OBJDIR)oautils/%.o : %.C
	$(CXX) $(CXXFLAGS) $(COMP_OBJ) $@ $<

#----------------------------------------------------------------------------

$(OBJDIR)tarjans/%.o : %.C
	$(CXX) $(CXXFLAGS) $(COMP_OBJ) $@ $<

#****************************************************************************

