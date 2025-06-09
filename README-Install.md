<!-- -*-Mode: markdown;-*- -->
<!-- $Id$ -->


Prerequisites
=============================================================================

Environment
  - GCC
  - Python (required: v2 for perf)
  - Python Module(s)
    - `pefile` for perf

Benchmarks
  - OpemMP
  - MPI

Building & Installing
=============================================================================


0. Options:
   ```sh
   DEVELOP=0 # use 1 for developmental build

   MG_PERF_CC=<linux-perf-cc>
   
   export MG_XLIB_ROOT=<optional-xlib-build-path>
   export MG_PERF_ROOT=<optional-perf-build-path>
   ```

1. Build libraries
   ```sh
   cd <memgaze>/xlib
   make xlib
   make perf # optional
   ```
  

2. Build MemGaze
   ```sh
   cd <memgaze>
   make PREFIX=<install-prefix> install
   ```


Using
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
   memgaze-inst [-o <inst-dir>] <app-dir>/<app>
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

  Uses:
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

  - Extract data file with perf-script
  - Convert IP offsets (from perf script) to full static IPs and combine two-address loads into single trace entry.
  - Remove data collection errors from trace
  - Separate memory references and call paths
  
  Uses:
  ```
  perf script libexec/perf-script-*
  libexec/memgaze-xtrace-normalize <trace-dir>.trace
  ```


4. Analyze memory behavior. Default analyzes memory operations over
   time using execution interval tree and generates metrics
   characterizing accesses, spatio-temporal reuse, and access
   patterns. As inputs, takes `<trace-dir>`.

   ```
   memgaze-analyze [-o <output>] <trace-dir>
   ```

  Currently, memgaze-analyze's default output includes two parts:
  - execution window histograms
  - function analysis summary (exclusive), averaged over whole trace, for all functions

   Note: -f <func>: Focus the above analysis on 'func', i.e., the effective trace includes all accesses between first and last instance of <func>.
   


