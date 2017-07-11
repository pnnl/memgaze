# -*-Mode: makefile;-*-

include ../../miami.config

#****************************************************************************

TOOL_ROOTS = miami
TOOL_VAR = vars_sched
TOOL_OBJ_DIR = scheduler
BINSCRIPT = $(PALM_MIAMI_ROOT)/src/tools/run_static.in

#****************************************************************************

SCHEDULER_OBJS = \
	schedtool.o \
	\
	CFG.o routine.o load_module.o debug_scheduler.o \
	block_path.o PathInfo.o TimeAccount.o Clique.o FindCliques.o \
	reuse_group.o Instruction.o Machine.o MemoryHierarchyLevel.o \
	PatternGraph.o \
	InstTemplate.o dependency_type.o schedule_time.o StringAssocTable.o \
	imix_histograms.o imix_clustering.o imix_width_clustering.o \
	TemplateExecutionUnit.o UnitRestriction.o BitSet.o BypassRule.o \
	prog_scope.o \
	machdesc.tab.o machdesc.lex.o MiamiDriver.o \
	memory_latency_histograms.o \
	path_id.o CycleSetDAG.o SchedDG.o DGBuilder.o \
	stream_reuse_histograms.o \
	XML_output.o memory_reuse_histograms.o TemplateInstantiate.o

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
	instruction-xlate.o register_class.o instr_info.o instr_bins.o \
	static_branch_analysis.o instruction_class.o canonical_ops.o \
	math_routines.o

PIN_COMMON_OBJS = instruction_decoding_pin.o
#instruction_decoding_dyninst.o

OA_OBJS = BaseGraph.o DGraph.o

TARJ_OBJS = TarjanIntervals.o MiamiRIFG.o UnionFindUniverse.o

SA_TOOLS   = $(TOOL_ROOTS:%=$(EXEDIR)%)
SA_WRAPPER = $(TOOL_ROOTS:%=$(BINDIR)%)

OBJS = \
	$(SCHEDULER_OBJS:%=$(OBJDIR)$(TOOL_OBJ_DIR)/%) \
	$(COMMON_OBJS:%=$(OBJDIR)common/%) \
	$(PIN_COMMON_OBJS:%=$(OBJDIR)pinobj/%) \
	$(OA_OBJS:%=$(OBJDIR)oautils/%) \
	$(TARJ_OBJS:%=$(OBJDIR)tarjans/%)

YACC = bison
FLEX = flex

TOOLS = $(SA_TOOLS) $(SA_WRAPPER)

#****************************************************************************

# finds files for the compiling rules
vpath %.C $(PALM_MIAMI_ROOT)/src/Scheduler
vpath %.cpp $(PALM_MIAMI_ROOT)/src/Scheduler
vpath %.C $(PALM_MIAMI_ROOT)/src/common
vpath %.cpp $(PALM_MIAMI_ROOT)/src/common
vpath %.C $(PALM_MIAMI_ROOT)/src/OAUtils
vpath %.C $(PALM_MIAMI_ROOT)/src/tarjans

##############################################################
#
# build rules
#
##############################################################

include ../make.rules

.force:

$(OBJDIR): .force
	mkdir -p $(OBJDIR)
	mkdir -p $(OBJDIR)$(TOOL_OBJ_DIR)
	mkdir -p $(OBJDIR)common
	mkdir -p $(OBJDIR)pinobj
	mkdir -p $(OBJDIR)oautils
	mkdir -p $(OBJDIR)tarjans

$(OBJDIR)$(TOOL_OBJ_DIR)/machdesc.tab.o : machdesc.tab.c machdesc.tab.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJDIR)$(TOOL_OBJ_DIR)/machdesc.lex.o : machdesc.lex.c machdesc.tab.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

machdesc.tab.c: machine_description.y
	$(YACC) -d -b machdesc machine_description.y

machdesc.tab.h: machine_description.y
	$(YACC) -d -b machdesc machine_description.y

machdesc.lex.c: machine_description.l
	$(FLEX) -t machine_description.l > machdesc.lex.c

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
