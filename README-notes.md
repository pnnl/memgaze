-*-Mode: markdown;-*-

$Id$

-----------------------------------------------------------------------------
Changes from MIAMI-NW (Oldest to newest)
=============================================================================

* Use DynInst to load/process binary instead of
  PIN. LoadModule/Routine/instructions use data from DynInst.
  - `src/Scheduler/load-module.C`
  - `src/Scheduler/routine.C`

* Replace use of PIN with Xed (support newer GCCs and C++ RTTI)
  - Use argp option parser (instead of PIN's)
  - Remove tools that depend on PIN.
  - `miami.config.sample`
  - `miami.config`
  - `src/make.rules`
  - `src/Scheduler/makefile.pin`                     [removed]
  - `src/tools/pin_config`                            [removed]
  - `src/{CFGtool, CacheSim, MemReuse, StreamSim}` [removed]

* Simplify make system (now that PIN-based tools are gone)
  - `miami.config`
  - `src/make.rules`                               [removed]
  - `src/Scheduler/makefile.pin -> Makefile`    [renamed]

* Initial support for decoding with either Xed or DynInst
  `SystemSpecific/x86_xed_decoding.C` -> `InstructionDecoder-xed.cpp`
  `SystemSpecific/IB_x86_xed.C` -> `InstructionDecoder-xed-iclass.h`

* Initial support for new Xed instructions (see MIAMI-NW/Pin 3.x support)
  [[TODO]] properly model instruction, esp. LOCK instructions.



-----------------------------------------------------------------------------
MIAMI Notes
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


-----------------------------------------------------------------------------
Notes on Xia's SeaPearl Working Directory
=============================================================================

* Changes I have made to Xia's code (w.r.t. incorporating 'external4'
  work into 'MIAMI-palm')

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


----------------------------------------
Index
----------------------------------------

* ~huxi333/palm/trunk/external:  Corresponds to MIAMI-v1

  * Debug Output:
    - `src/CFGtool/cfgtool_dynamic.C`
    - `src/CFGtool/cfgtool_static.C`
  
    - `src/Scheduler/DGBuilder.C`
    - `src/Scheduler/MiamiDriver.C`
    - `src/Scheduler/load_module.C`
    - `src/Scheduler/routine.C`

    - `src/common/PrivateCFG.h`
    - `src/common/SystemSpecific/x86_xed_decoding.C`
  
* ~huxi333/palm/trunk/external4: MIAMI-palm
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

