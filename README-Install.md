-*-Mode: markdown;-*-

$Id$


Prerequisites
=============================================================================

Environment
  - GCC ...


Building & Installing
=============================================================================

1. Libraries
   ```sh
   cd <memgaze>/xlib
   make install
   ```
  
   Options:
   ```sh
   export MG_XLIB_ROOT=<optional-build-path>
   export MG_PERF_ROOT=<optional-build-path>
   ```

2. Build
   ```sh
   cd <memgaze>
   make PREFIX=<install-prefix> install
   ```


Using
=============================================================================

