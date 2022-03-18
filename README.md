-*-Mode: markdown;-*-

$Id$

-----------------------------------------------------------------------------
MemGaze Pipeline
=============================================================================

MemGaze has 3 mains steps.

1. Instrument application binary <app> and write to `<app>-memgaze`
   [[FIXME]]. Also generates downstream static analysis data to
   `<app>-memgaze.binanlys`. Generates output within <app-dir>.

   Note: If instrumenting <app>'s libraries, must instrument each
   library and relink <app>-memgaze.

   Note: For a system library, copy to a writable directory first.

   ```
   memgaze-inst <app-dir>/<app>  [[FIXME: was instument_binary.sh]]
   ```

   Auxiliary output:
   - `<app>-memgaze.binanlys.log`: Mapping from original IP to instrumented IP [[FIXME]]
   - `<app>-memgaze.binanlys0`   : Load classifications [[FIXME]]
   - `<app>-memgaze.hpcstruct`   ; HPCToolkit structure file

  Steps:
  - Read binary (with dyninst)
  - Create CFG and dependence graph for each routine to determine each load's classification (strided, indirect, constant)
  - Selectively insert ptwrite.
  - Generates 1) load classification data and 2) mapping from original IP to instrumented IP (`libexec/memgaze-inst-cat`)
  - Generates HPCToolkit structure file

  
  [[FIXME: libexec/memgaze-inst-cat && called by memgaze-inst]]
  ```
  ./ip_converter.py <app>-memgaze.binanlys.log <app>-memgaze.binanlys0
  ```

  Create hpcstruct file  [[FIXME: called by memgaze-inst]]


2. Trace or (profile memory) behavior of application `<app>-memgaze`.
  
   ```
   memgaze-run <size> <period> 0 ${LOAD} -- <app-dir> "<app-args>"   [[FIXME]]
   ```

3. Convert tracing data into open format from Linux perf's format.

   ```
   memgaze-xlate <app>_s<size>_p<period>
   ```

  - Extract data file with perf-script using [[libexec/perf-script-intel-pt.py or libexec/perf-script-ldlat.py]]
  ```
  libexec/extract_data.sh <app>_s<size>_p<period> [[FIXME: move into memgaze-xlate]]
  ```

  - Remove data collection errors from trace
  - Convert IP offsets (from perf script) to full static IPs and combine two-address loads into single trace entry.
  - Separate trace and Call Path into two different file from the perf script output
  ```
  libexec/memgaze-xlate-normalize <app>_s<size>_p<period>.trace [[FIXME && called from memgaze-xlate]]
  ./add_base_IP.py <app>_s<size>_p<period>.trace.clean <app>_PTW <app>-memgaze.binanlys_Fixed <app>_s<size>_p<period>.trace.final [[FIXME: move into memgaze-xlate-normalize]]
  ```


4. Analyze memory behavior using execution interval tree and generate footprint metrics.

  ```
  memgaze-analysis <app>_PTW.hpcstruct <app>-memgaze.lc_Fixed <app>_s<size>_p<period>.trace.final $Period $LOAD $FUNC $MAKEDBIT  [[FIXME: no script!]]
  ```
  - Read multiple file:
    - CallPath  (*.callpath)
    - Trace     (*.trace)
    - hpcstruct (*.hpcstruct)
    - binary analysis (*.binanlys)
  
  - Build calling context tree (work in progress)
  - Do memeory analysis on the tree  or per routine



Extra:
----------------------------------------

To run all three main steps use compile.sh 

  ```
  ./compile.sh <FOLDER> <BIN> <ARGS> <0(time)/1(load)> <BufferSIZE(def=8192)> <PERIOD(def=37500000)> <MaskedBits(def=0)> <FUNCTION-NAME if need to focus>
  ```

[[memgaze-all]]




-----------------------------------------------------------------------------
Linux Perf
=============================================================================

- Using kernel 5.5.9: no changes needed.
    
- Modifications to Linux perf (user level):
  - perf script (static ip address instead of dynamic)
  - we played with perf driver, but are not using it

- Perf command we used:
  - Collecting trace by sampling based on number of loads:
  
  ```perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -g -e cpu/umask=0x81,event=0xd0,period=<period>,aux-sample-size=<size>,call-graph=lbr/u -o ${bin}.data -- ./${bin} $args```

  
  - Collecting trace by sampling based on time:

  ```perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -g -e ref-cycles/period=<period>,aux-sample-size=<size>,call-graph=lbr/u -- ${bin} ${args}```

  - Using filter to focus a single function

  ```perf record -m 4M,4M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u --filter 'filter @distBuildLocalMapCounters' -o ${bin}.data -- ./${bin} $args```

  - To extract trace created usin ptwrite

  ```perf script --script=/home/kili337/Projects/Fallacy/scripts/intel-pt-events.py -i ${bin}.data > ${bin}.trace```

  - To collect trace using ldlat for intel machines

  ```perf record -W -d -e cpu/mem-loads,ldlat=1,period=100/upp```
  
  - To extract ldlat trace

  ```perf script --script=../ldlat-events.py -i <input>```

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
