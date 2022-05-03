-*-Mode: markdown;-*-

$Id$

-----------------------------------------------------------------------------
Linux Perf
=============================================================================

- Intel PT works with kernel 5.5.9, no changes needed.
  - Although we played with perf's kernel driver, we are not using it
    
- Minor patch/modification to Linux perf's user level tool for "perf script"
  - convert the dynamic ip to a static ip, to attribute to binary

- Perf examples

  - Intel PT, sampling based on loads:
  ```
  perf record -m 2M,2M -o ${bin}.data
    -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u 
    -e cpu/umask=0x81,event=0xd0,period=<period>,aux-sample-size=<bufsz>,call-graph=lbr/u -g
    -- <app>
  ```

  - Intel PT, sampling based on time:
  ```
  perf record -m 2M,2M -o ${bin}.data
    -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u
    -e ref-cycles/period=<period>,aux-sample-size=<bufsz>,call-graph=lbr/u -g 
    -- <app>
  ```

  - Intel PT, enabling/disabling PT instructions via hardware code filter:
  ```
  perf record -m 4M,4M -o ${bin}.data \
    -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u
    --filter 'filter @distBuildLocalMapCounters'  -- <app>
  ```

  - Intel PT, system-wide [[FIXME]]:


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


- Other notes:

  - Call paths: On our Atom-based (J5005) test machine, it appears
    that collecting call paths using LBR is not supported. Therefore,
    '-g' defaults to libunwind. libunwind occurs in user-space and
    perf copies context/stack into its buffer.


-----------------------------------------------------------------------------
Libraries
=============================================================================

- We build dependency libraries using Spack

- Leverage HPCToolkit's spack recipe.


  - Ideally, we would hope that on the initial HPCToolkit build, the
    dyninst patch would trigger and everything would build with one
    "spack install". However, it seems that 'hpctk-pkgs.yaml' needs to
    be removed to trigger the patch.
    
  - We can successfully build HPCToolkit and then DynInst. Currently,
    the reverse doesn't seem to work because DynInst will select some
    package dependencies in a way that HPCToolkit does not like.


- Customizations to Spack

  - config.yaml:     Update of Spack's version to change 'root' dir names
  - hpctk-pkgs.yaml: HPCToolkit's "packages.yaml"
  - dyninst.py:      Control DynInst patching. Update of Spack's version.
  - dyninst.patch:   DynInst patch

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
