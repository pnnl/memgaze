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

  Important contents of `<inst-dir>`:
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
  libexec/memgaze-inst-cat <tracedir>/<app>.binanlys.log <tracedir>/<app>.binanlys0
  ```


2. Trace memory behavior of application `<app>`, which is normally
   `<inst-dir>/<app>`. Generates output in `<trace-dir>`, which
   defaults to `./memgaze-<app>-trace-b<bufsz>-p<period>`.

   By default collect a sampled trace with period <period>; if period
   is 0 then collect a full (non-sampled) trace. Sample event can be
   either load-based or time-based.

   ```
   memgaze-run -p <period or 0> -b <bufsz> [-e pt-load | pt-time | ldlat] [-o <trace-dir>] -- <app> <app-args>
   ```
   
   Important contents of `<trace-dir>`:
   - `<memgaze.config>`: Configuration arguments
   - `<app>.perf`      : Tracing data (Linux perf)

3. Translate tracing and profiling data within `<trace-dir>` into open
   formats and place results therein.

   ```
   memgaze-xtrace <trace-dir>
   ```

  New contents of `<trace-dir>`:
  - `<app>.trace`    : Memory references
  - `<app>.callpath` : Call paths


  - Extract data file with perf-script using [[FIXME: libexec/perf-script-intel-pt.py or libexec/perf-script-ldlat.py]]

  - Convert IP offsets (from perf script) to full static IPs and combine two-address loads into single trace entry.
  - Remove data collection errors from trace
  - Separate memory references and call paths
  ```
  libexec/memgaze-xtrace-normalize <trace-dir>.trace [[FIXME: called from memgaze-xtrace]]
  ./add_base_IP.py <trace-dir>.trace.clean <app>_PTW <tracedir>/<app>.binanlys_Fixed <trace-dir>.trace.final [[FIXME: move into memgaze-xtrace-normalize]]
  ```


4. Analyze memory behavior using execution interval tree and generate
   footprint metrics. As inputs, takes `<trace-dir>` and `<inst-dir>`

   ```
   memgaze-analyze -t <trace-dir> -s <inst-dir> [-o <output>]
   ```
  
  [[FIXME]] -f func: poor man's inclusive and masked-bit
  


-----------------------------------------------------------------------------
Trace format
=============================================================================

MemGaze:   <insn-pc> <mem-addr> <cpu-id> <timestamp> sample-id
  (in initial trace, a two-source load has two lines, which are converted to one line)


MemCAMera: <insn-pc> <mem-addr> <cpu-id> <timestamp> [mem-addr2 cpu-id timestamp]
