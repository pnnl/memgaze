-*-Mode: markdown;-*-

$Id$

-----------------------------------------------------------------------------
Linux Perf
=============================================================================

- Using kernel 5.5.9: no changes needed.
    
- Modifications to Linux perf (user level):
  - perf script (static ip address instead of dynamic)
  - we played with perf driver, but are not using it

- Perf examples

  - Intel PT, sampling based on loads:
  ```
  perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -g -e cpu/umask=0x81,event=0xd0,period=<period>,aux-sample-size=<bufsz>,call-graph=lbr/u -o ${bin}.data -- <app>
  ```

  - Intel PT, sampling based on time:
  ```
  perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -g -e ref-cycles/period=<period>,aux-sample-size=<bufsz>,call-graph=lbr/u -- <app>
  ```

  - Intel PT, enabling/disabling PT instructions via hardware code filter:
  ```
  perf record -m 4M,4M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u --filter 'filter @distBuildLocalMapCounters' -o ${bin}.data -- <app>
  ```

  - Intel Load Latency-based (sparse) data addresses:
  ```
  perf record -W -d -e cpu/mem-loads,ldlat=1,period=100/upp -- <app>
  ```

  - Use 'perf script' to extract trace from Intel PT
  ```
  perf script --script=perf-script-intel-pt.py -i <trace>
  ```
  
  - Use 'perf script' to extract trace from Intel Load Latency
  ```
  perf script --script=perf-script-intel-pt.py -i <trace>
  ```

- Other notes (???)

  - without --per-thread, perf opens a fd on each core because it has a buffer/event per cpu; size (129 pages  4 k)

  - perf-event-disable (ioctl) should have an effect when it returns (write to msr)

  - libunwind happens in userland: perf copies context/stack into its buffer


-----------------------------------------------------------------------------
Libraries
=============================================================================

- We build dependency libraries using Spack

- Leverage HPCToolkit's spack recipe.

- Customizations to Spack

  - config.yaml:   New Spack root.  Update of Spack's version.
  - package.py:    DynInst patching (memgaze.patch). Update of Spack's version.
  - hpctoolkit-packages.yaml: HPCToolkit's "packages.yaml"
  - memgaze.patch: DynInst patch

- DynInst patch

  Currently, we must apply one hack/patch to DynInst.
  
    New Dyninst master provides source line mapping for instrumented
    code (now in master), which hpcstruct can read.

    However, to gather instruction classes, we still need (more
    precisely, want) the mapping.  The MemGaze instrumentor determines
    instruction classes from the original binary, but we also need the
    classes for the corresponding new/instrumented ins. Currently,
    we use a one-line hack in Dyninst to print the mapping.
    
    Alternatives: 1) Re-run MemGaze's static binary analysis on the
    new code within the binary. 2) Propose a DynInst interface for
    exporting the details of the mapping.
