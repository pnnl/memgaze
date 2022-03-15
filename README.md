-*-Mode: markdown;-*-

$Id$

-----------------------------------------------------------------------------
MemGaze Pipeline
=============================================================================

MemGaze has 3 mains steps:
1-) Analysis and Instrumentation of the Binary (bin-anyls):
  -> Read binary with dyninst
  -> Create internal CFG for each Routine
  -> Analyze each routine to find each load and their types ( strided, indirect, constant)
  -> Instrument with ptwrite for selective loads
  -> Print original and instrumented IP address
  -> Print load classification file
  -> Adjust IP addresses

2-) Trace Collection (mem-trace)
  -> Run Instrumented Binary with perf
  -> Extract data file with perf-script by using a python script
  -> Fix IP address by adding base address
  -> Separate trace and Call Path graph into two different file from the perf script output

3-) Memory Analysis (mem-anlys)
  -> Read multiple file:
    -> CallPath  (*.callpath)
    -> Trace     (*.trace)
    -> hpcstruct (*.hpcstruct)
    -> load classification (*.lc)
  -> Build calling context tree (work in progress)
  -> Do memeory analysis on the tree  or per routine



-----------------------------------------------------------------------------
Linux Perf
=============================================================================




-----------------------------------------------------------------------------
Trace format
=============================================================================

MemGaze:   insn-pc mem-addr cpu-id timestamp sample-id

MemCAMera: insn-pc mem-addr cpu-id timestamp [mem-addr2 <cpu-id> <timestamp>]
