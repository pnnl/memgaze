-*-Mode: markdown;-*-

$Id$


Prerequisites
=============================================================================

Environment
  - GCC ...


Building & Installing
=============================================================================


0. Options:
   ```sh
   DEVELOP=1

   MG_PERF_CC=<linux-perf-cc>
   
   export MG_XLIB_ROOT=<optional-xlib-build-path>
   export MG_PERF_ROOT=<optional-perf-build-path>
   ```

1. Build libraries
   ```sh
   cd <memgaze>/xlib
   make install
   ```
  

2. Build MemGaze
   ```sh
   cd <memgaze>
   make PREFIX=<install-prefix> install
   ```


Using
=============================================================================

