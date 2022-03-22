-*-Mode: markdown;-*-

$Id$

-----------------------------------------------------------------------------
MemGaze Pipeline
=============================================================================

MemGaze has four main steps.

1. Instrument application binary (or libary) `<app>` for memory
   tracing. Generate instrumented `<app>` and auxiliary data within
   directory `<inst-dir>`, which defaults to
   `<app-dir>/memgaze-<app>`. Instrumented `<app>` is
   `<inst-dir>/<app>`.
   
   Note: If instrumenting `<app>`'s libraries, must instrument each
   library and relink instrumented `<app>`.

   ```
   memgaze-inst <app-dir>/<app> [-o <inst-dir>]
   ```

  Important contents of `<inst-dir>`: [[FIXME]]
  - `<app>`           : Instrumented `<app>`
  - `<app>.binanlys`  : Static binary analysis
  - `<app>.hpcstruct` : HPCToolkit structure file

  Steps:
  - Read binary (with dyninst)
  - Create CFG and dependence graph for each routine to determine each
    load's classification (strided, indirect, constant)
  - Selectively insert ptwrite.
  - Generates 1) load classification data and 2) mapping from original
    IP to instrumented IP (`libexec/memgaze-inst-cat`)
  - Generates HPCToolkit structure file

  ```
  libexec/memgaze-inst-cat <tracedir>/<app>.binanlys.log <tracedir>/<app>.binanlys0   [[FIXME: was ip_converter.py]]
  ```


2. Trace memory behavior of application `<app>`, which is normally
   `<inst-dir>/<app>`. Generates output in `<trace-dir>`, which
   defaults to `./memgaze-<app>-trace-b<bufsz>-p<period>`. [[FIXME]]

   By default collect a sampled trace with period <period>; if period
   is 0 then collect a full (non-sampled) trace. Sample event can be
   either load-based or time-based.

   ```
   memgaze-run -p <period or 0> -b <bufsz> [-e pt-load | pt-time | ldlat] [-o <trace-dir>] -- <app> <app-args> [[FIXME: what is 0 and <LOAD>?]]
   ```
   
   Important contents of `<trace-dir>`: [[FIXME]]
   - `<memgaze.config>`: Configuration arguments
   - `<app>.perf`      : Tracing data (Linux perf)
   
   [[FIXME]] cf. `<palm>/palm-task/palm-task-memlat. 


3. Translate tracing and profiling data within `<trace-dir>` into open
   formats and place results therein.

   ```
   memgaze-xtrace <trace-dir>
   ```

  New contents of `<trace-dir>`: [[FIXME]]
  - `<app>.trace`    : Memory references
  - `<app>.callpath` : Call paths


  - Extract data file with perf-script using [[libexec/perf-script-intel-pt.py or libexec/perf-script-ldlat.py]]
  ```
  libexec/extract_data.sh <trace-dir> [[FIXME: part of memgaze-xtrace; delete]]
  ```

  - Convert IP offsets (from perf script) to full static IPs and combine two-address loads into single trace entry.
  - Remove data collection errors from trace
  - Separate memory references and call paths
  ```
  libexec/memgaze-xtrace-normalize <trace-dir>.trace [[FIXME && called from memgaze-xtrace]]
  ./add_base_IP.py <trace-dir>.trace.clean <app>_PTW <tracedir>/<app>.binanlys_Fixed <trace-dir>.trace.final [[FIXME: move into memgaze-xtrace-normalize]]
  ```


4. Analyze memory behavior using execution interval tree and generate
   footprint metrics. As inputs, takes `<trace-dir>` and `<inst-dir>`

   ```
   memgaze-analyze -t <trace-dir> -s <inst-dir> [-o <output>] [[FIXME]]
  
   [[FIXME: old]] memgaze-analyze -t <trace> -l <binanlys> -s <hpcstruct> -o <output> -m <1(load) | 0(time)> -p <period> -c <callpath>
   ```
  
  [[FIXME]] -f func: poor man's inclusive and masked-bit
  


FIXME
----------------------------------------

Execute entire pipeline [[FIXME: kill]]

  ```
  memgaze-all <FOLDER> <BIN> <ARGS> <0(time)/1(load)> <BufferSIZE(def=8192)> <PERIOD(def=37500000)> <MaskedBits(def=0)> <FUNCTION-NAME if need to focus> 
  ```


-----------------------------------------------------------------------------
Linux Perf
=============================================================================

- Using kernel 5.5.9: no changes needed.
    
- Modifications to Linux perf (user level):
  - perf script (static ip address instead of dynamic)
  - we played with perf driver, but are not using it

- Perf command we used:
  - Collecting trace by sampling based on number of loads:
  
  ```
  perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -g -e cpu/umask=0x81,event=0xd0,period=<period>,aux-sample-size=<bufsz>,call-graph=lbr/u -o ${bin}.data -- ./${bin} $args
  ```

  
  - Collecting trace by sampling based on time:

  ```
  perf record -m 2M,2M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u -g -e ref-cycles/period=<period>,aux-sample-size=<bufsz>,call-graph=lbr/u -- ${bin} ${args}
  ```

  - Using filter to focus a single function

  ```
  perf record -m 4M,4M -e intel_pt/ptw=1,branch=0,period=1,fup_on_ptw=1/u --filter 'filter @distBuildLocalMapCounters' -o ${bin}.data -- ./${bin} $args
  ```

  - To extract trace created usin ptwrite

  ```
  perf script --script=/home/kili337/Projects/Fallacy/scripts/intel-pt-events.py -i ${bin}.data > ${bin}.trace
  ```

  - To collect trace using ldlat for intel machines

  ```
  perf record -W -d -e cpu/mem-loads,ldlat=1,period=100/upp
  ```
  
  - To extract ldlat trace

  ```
  perf script --script=../ldlat-events.py -i <input>
  ```

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

MemCAMera: insn-pc mem-addr cpu-id timestamp [mem-addr2 cpu-id timestamp]
