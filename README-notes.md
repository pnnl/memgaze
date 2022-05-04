-*-Mode: markdown;-*-

$Id$

-----------------------------------------------------------------------------
MemGaze Consolidation
=============================================================================

* [[todo]]

  - paper:
    - overhead results: do not collect call graph???
    - how good is exclusive function attribution on new minivite trace?
    - analyze parallelism: single vs. multi-thread runs for minivite

  - nuke: /DATA/Projects/MemGaze
    - keep data-locality results and delete "overhead" results
    
  - Build:
    - can we now use official DynInst? ask Xiaozhu Meng
    - autoconfigure hpctk source tree?
    - dependences for perf?
    
  - run scripts: memgaze-*:
    - memgaze-inst: name output binary
    
    - memgaze-*: libexec
      - example for system-wide collection?
      - on nuke, the 'lbr' spec is ignored? any warning?
    
    ??? without --per-thread, perf opens a fd on each core because it has a buffer/event per cpu; size (129 pages  4 k)

    ??? perf-event-disable (ioctl) should have an effect when it returns (write to msr)

* Minor:
  - Try Release-2022.04.15 (2022.04.15)

* Bugs:
  - mem-anlys only reads only one load-classification file even if multiple are needed
    - results in some instructions with unknown laod classes

  - Our window analysis algorithm uses pre-selected bins to create the histogram. Due to the variation of sample sizes window sizes also vary. This can create a binning anomaly at the largest one or two window sizes. Since \fpSym  should never get smaller in a larger window, we force each bin to take the maximum of the current and previous windows. This anomaly only happens when there are constant loads instrumented for our quantitative approach. To address this issue we are working on a more detailed fix.


-----------------------------------------------------------------------------

* Ruchi <nina222> branch to write execution interval tree as hpcviewer xml:

  ~/1perf-lab/palm/memgaze-memanlys-ruchi/ (/files0/kili337/Nathan/intelPT_FP)
  
  See: 'ruchi-*.cpp' and test example in 'hpcviewer-data'

```
<MetricTable>
    <Metric i="0" n="footprint" o="0" v="final" show="1" show-percent="1">
      <Info><NV n="units" v="footprint"/><NV n="period" v="1"/></Info>
    </Metric>
</MetricTable>

<PF i="2" ...>
  <M n="0" v="<value"/>
  <C i="3" s="212" l="0" v="0xd39f">
    <PF i="4" ...>
```



=============================================================================
MemGaze Structure
=============================================================================

* bin-anlys: Static binary analysis and instrumentation

  - Contains code derived from MIAMI-NW. Uses DynInst.

  - Fast-footprint 'fine-grained footprint analysis' (ISPASS 20). This
    is function-level, footprint analysis combining (intraprocedural)
    static analysis, control-flow profiles (for loop execution
    counts), and memory-hit profiles.
  
  - Palm's task-based path cost analysis (critical path) for paths of
    basic blocks. Cost includes instruction pipeline latency and
    memory latency. Replaces IACA and extends its analysis.


* mem-trace: Trace or profile memory behavior

* mem-anlys: Analyze memory behavior from memory traces

  - footprint, execution interval tree

  - Palm coarse-grained footprint analysis
    palm-task: hpctoolkit run + fp analysis on database xml file


=============================================================================
MemGaze/bin-anlys: Changes from MIAMI-NW (newest first)
=============================================================================

* Ozgur Kilic's annotations:

  - FIXME:dyninst     -> changes to use dyninst
  
  - FIXME:amd         -> commented out for AMD instructions
  - FIXME:instruction -> changes for new instruction types

  - FIXME:BETTER      -> things can be updated for better aproach
  - FIXME:NEWBUILD    -> changes made for spack build

  - FIXME:latency     -> possible error for latency

  - FIXME:unkown      -> I don't remember why I did that

  - FIXME:tallent     -> not from Ozgur
  - FIXME:old         -> not from Ozgur
  - FIXME:deprecated  -> not from Ozgur


* Initial support for new Xed instructions (see MIAMI-NW/Pin 3.x support)
  [[TODO]] properly model instruction, esp. LOCK instructions.


* Initial support for decoding with either Xed or DynInst
  `SystemSpecific/x86_xed_decoding.C` -> `InstructionDecoder-xed.cpp`
  `SystemSpecific/IB_x86_xed.C` -> `InstructionDecoder-xed-iclass.h`


* Simplify make system (now that PIN-based tools are gone)
  - `miami.config`
  - `src/make.rules`                               [removed]
  - `src/Scheduler/makefile.pin -> Makefile`    [renamed]

* Replace use of PIN with Xed (support newer GCCs and C++ RTTI)
  - Use argp option parser (instead of PIN's)
  - Remove tools that depend on PIN.
  - `miami.config.sample`
  - `miami.config`
  - `src/make.rules`
  - `src/Scheduler/makefile.pin`                     [removed]
  - `src/tools/pin_config`                            [removed]
  - `src/{CFGtool, CacheSim, MemReuse, StreamSim}` [removed]


* Use DynInst to load/process binary instead of
  PIN. LoadModule/Routine/instructions use data from DynInst.
  - `src/Scheduler/load-module.C`
  - `src/Scheduler/routine.C`

* changes made for  spack build
  load_module.C:1146:11: error: ‘class BPatch’ has no member named ‘setRelocateJumpTable’
  #    bpatch.setRelocateJumpTable(true);
  #TODO FIXME I commet out that line for now

  #src/Scheduler/Makefile
  36 DYNINST_CXXFLAGS = \
  37         -I$(DYNINST_INC) \
  38         -I$(BOOST_INC) \
  39         -I$(TBB_INC)

  /files0/kili337/TestBed/memgaze/memgaze/bin-anlys/src/common/source_file_mapping_binutils.C:168:9: note: suggested alternative: ‘bfd_set_section_flags’
      if ((bfd_get_section_flags (abfd, section) & SEC_ALLOC) == 0)
  /files0/kili337/TestBed/memgaze/memgaze/bin-anlys/src/common/source_file_mapping_binutils.C:175:10: note: suggested alternative: ‘bfd_set_section_vma’
      vma = bfd_get_section_vma (abfd, section);
  /files0/kili337/TestBed/memgaze/memgaze/bin-anlys/src/common/source_file_mapping_binutils.C:176:11: note: suggested alternative: ‘bfd_set_section_size’
      size = bfd_get_section_size (section);

  /files0/kili337/TestBed/memgaze/memgaze/bin-anlys/src/common/source_file_mapping_binutils.C:217:19: error: ‘bfd_get_section_vma’ was not declared in this scope
      addrtype vma = bfd_get_section_vma (info->abfd, section);  

  src/common/InstructionDecoder-xed-iclass.h
  25 //FIXME:NEWBUILD    case XED_ICLASS_PFCPIT1:            // 3DNOW
  38 //FIXME:NEWBUILD    case XED_ICLASS_PFSQRT:             // 3DNOW 


=============================================================================
MemGaze/bin-anlys: Translate DynInst IR to MIAMI IR
=============================================================================

* Changes I made to Xia's <huxi333> code to incorporate 'external4'
  into 'MemGaze/bin-anlys'

  - Reinstated MIAMI driver.
  
  - Disable Xia's code.

  - Although we will eventually discard it, we should use XED as a
    validation tool.
	
	- I updated the instrution translation to print the *input*
      instruction using XED's and DynInst's decoders. The decoding
      should align.

    - We should check the *output* translation in two ways. The first
      way is using MIAMI's original XED-based translator; and then
      using our DynInst-based translator. I've updated the driver
      accordingly.

  - Xia's code is slow for a couple reasons:
    - For one routine, seach through all functions O(|functions|)...
    - For one instruction, the translation is O(|functions| *
      |blocks| * |insn-in-block|). This holds even for the simplest
      instruction (e.g. a nop) without registers.
  
    I removed the last term (|insn-in-block|) by fixing a decoding bug
    isaXlate_getDyninstInsn(). The code had scanned the instrutions in
    a basic block but stopped just before the requested instruction
    address.
  
  - Xia's code had static data structures.  It makes the code hard to
    understand and is unsafe for threads.  Also, it turns out that
    some of the data structures were computed twice, once when
    building the CFG and again when initializing instruction
    translation for the routine. For example `bpatch.openBinary()` was
    done twice on the same routine, creating two versions of
    BPatch_image and vector<BPatch_function*>.
	[`create_loadModule()` and `isaXlate_init()`]
	
	I consolidated the CFG and Instruction translation code to avoid
	this and make the code easier to understand.

    For now I have partially cleaned the way the static data is used
    so that, e.g., lm_func2blockMap is not computed multiple times
    (once for each routine).


* BUGS:

  - The DynInst Function/BasicBlock context should be part of MIAMI
    classes, e.g. Routine and CFG.

  - Complete translation from DynInst::Instruction -> MIAMI::Instruction.

  - Some of the static data structure are never cleared.  For example,
    `func_blockVec` is never cleared. It seems this may have created
    an ever-expanding worklist for `get_instructions_address_from_block()`.

  - Memory leak in `Routine::decode_instructions_for_block()`...


Xia's <huxi333> efforts on SeaPearl
-----------------------------------------------------------------------------

* ~huxi333/palm/trunk/external: Corresponds to MIAMI-v1 (just debug output)
  - `src/CFGtool/cfgtool_dynamic.C`
  - `src/CFGtool/cfgtool_static.C`

  - `src/Scheduler/DGBuilder.C`
  - `src/Scheduler/MiamiDriver.C`
  - `src/Scheduler/load_module.C`
  - `src/Scheduler/routine.C`

  - `src/common/PrivateCFG.h`
  - `src/common/SystemSpecific/x86_xed_decoding.C`


* ~huxi333/palm/trunk/external4: Incorporated partially into MemGaze/bin-anlys
   - `MIAMI/src/Scheduler/Report`      : Summary of work
   - `MIAMI/src/Scheduler/dyninst_*`   : New files
	 
  * Control flow changes in driver
    - `src/Scheduler/MiamiDriver.C`
    - `src/Scheduler/load-module.C`
    - `src/Scheduler/routine.C`
    - `src/Scheduler/DGBuilder.C`
   
  * Test for duplicates
    - `src/OAUtils/BaseGraph.C`

  * Replace `dynamic_cast` with `static_cast`
    - `src/CFGtool/CFG.h`
    - `src/MemReuse/CFG.h`
    - `src/OAUtils/DGraph.h`
    - `src/Scheduler/PatternGraph.h`

  * Debug output
    - `src/CFGtool/cfg_data.C`
    - `src/CFGtool/routine.C`
    - `src/CFGtool/cfgtool_static.C`
    - `src/Scheduler/XML_output.C`
    - `src/Scheduler/schedtool.C`

  * No effects/buggy
    - `src/Scheduler/DGBuilder.h`
    - `src/Scheduler/SchedDG.C`


* Environment:
  ~huxi333/.bashrc
  ~huxi333/pkg


* Others
  ~huxi333/palm/trunk/external2: Not useful; not yet compiled.
  ~huxi333/palm/trunk/external3: Not useful. Attempted to work on 'CFGTool'



=============================================================================
MIAMI-NW structure
=============================================================================

* Open 'cfgprof' profile: 
  `MIAMI_Driver::Initialize()`

* Instrution path analysis using new basic-block profiling (Paths are
  reconstructed per routine. Callsites connect inter-procedural
  paths.)

  `Routine::myConstructPaths` [routine.C]
  -> `SchedDG::SchedDG()` -> `MIAMI_DG::DGBuilder(Routine)`
     -> `DGBuilder::build_graph()`
  
  `Routine::constructPaths()`
  -> `DGBuilder::computeMemoryInformationForPath()`
     -> `SchedDG::find_memory_parallelism()`
  

* Calls [some] to instruction decoding:
  `main()` [schedtool.C]
  -> `MIAMI_Driver::LoadImage()`
     -> `LoadModule::analyzeRoutines()`
        -> `Routine::main_analysis()`
		   -> `Routine::build_paths_for_interval`
              -> `Routine::decode_instructions_for_block()`
                 -> `isaXlate_insn() / decode_instruction_at_pc()`  <==
     	      -> `Routine::build_paths_for_interval()`
    	         -> `Routine::constructPaths()`
    	            -> `DGBuilder::DGBuilder()`
    	               -> `DGBuilder::build_graph()`
    	                  -> `DGBuilder::build_node_for_instruction()`
                          -> `isaXlate_insn() / decode_instruction_at_pc()`


* Some debug tracing:
  - src/Scheduler/debug_scheduler.h

* XED-based decoding implementation
  - `common/instruction_decoding.h`
  - `common/SystemSpecific/x86_xed_decoding.C`

* Scheduling...
  `Scheduler/SchedDG.{h,C}`:   scheduler analysis on the MIAMI IR
  `Scheduler/DGBuilder.{h,C}`: dependence graph builder; extends SchedDG

  MIAMI's DGBuilder takes a CFG::Block of raw data and decodes
  it. (This seems to be the wrong order.)

  Gabriel: "The DGBuilder takes as input a vector of (MIAMI) CFG::Node
  elements (the basic blocks of the path), decodes them to the MIAMI
  IR, and builds register/memory/control dependencies on them. All the
  analysis is then done on the IR itself, so I feel that it is
  possible to interface with another tool if you write another DG
  builder that takes as input your representation of blocks and
  converts them to the MIAMI IR."

  Two basic approaches for interfacing with DynInst. Favor (b)
   a) Use DynInst CFG/instructions. Build interface around them to
      enable MIAMI dependence-graph builder.
   b) Create MIAMI CFG/instructions from DynInst CFG/instructions.

  MIAMI's DGBuilder takes a CFG::Block of raw data and decodes
  it. (This seems to be the wrong order.)

  - common/instr_info.H: information for an instruction
  - common/instr_bins.H: IB = instruction-bin
  - common/instruction_decoding.C: instruction decoding interface
  - Scheduler/GenericInstruction.h
  
  Any definition files for other microarchitectures?


