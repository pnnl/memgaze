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

[[Ozgur: todo]]

- Using kernel 5.5.9: no changes needed.
    
- Modifications to Linux perf (user level):
  - perf script (static ip address instead of dynamic)
  - we played with perf driver, but are not using it


???
- system wide monitoring and ordering/timestamps

    ```
    - without --per-thread, perf opens a fd on each core because it has a buffer/event per cpu; size (129 pages  4 k)
    - perf-event-disable (ioctl) should have an effect when it returns (write to msr)
    - libunwind happens in userland: perf copies context/stack into its buffer

    > perf report -D
    > perf -g + pt
    > strace -etrace=perf_event_open
    ```


-----------------------------------------------------------------------------
Trace format
=============================================================================

MemGaze:   insn-pc mem-addr cpu-id timestamp sample-id

MemCAMera: insn-pc mem-addr cpu-id timestamp [mem-addr2 <cpu-id> <timestamp>]
