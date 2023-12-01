<!-- -*-Mode: markdown;-*- -->
<!-- $Id$ -->


Notes for amd-ibs-select
=============================================================================

1a. Collecting IBS data on AMD machines:

   ```sh
   # Tested: 4d4884b9bf1f20fc096270a7d071bac03564a636, Thu Apr 29 15:17:03 2021
   git clone https://github.com/jlgreathouse/AMD_IBS_Toolkit

   # Patch: see below

   # Collect
   OMP_NUM_THREADS=1 <path>/AMD_IBS_Toolkit.git/tools/ibs_run_and_annotate/ibs_run_and_annotate -o 20400  -- ./miniVite-v1 -n 300000 # -o 10400
   ```

   Patch: `<AMD_IBS_Toolkit>/tools/ibs_run_and_annotate/ibs_run_and_annotate`

```
[bluesky]:(x-AMD_IBS_Toolkit.git)> git diff
diff --git a/tools/ibs_run_and_annotate/ibs_run_and_annotate b/tools/ibs_run_and_annotate/ibs_run_and_an
index acc105c..26f069e 100755
--- a/tools/ibs_run_and_annotate/ibs_run_and_annotate
+++ b/tools/ibs_run_and_annotate/ibs_run_and_annotate
@@ -1,4 +1,4 @@
-#!/usr/bin/env python
+#!/usr/bin/env python3
 # coding=utf-8
 #
 # Copyright (C) 2017-2018 Advanced Micro Devices, Inc.
@@ -507,8 +507,8 @@ def main():
     global lib_size
     libs, pid_to_use = read_ld_debug(ld_debug_fn)
     vprint('Dumping info for process {}'.format(pid_to_use))
-    lib_base = map(itemgetter(0), libs)
-    lib_size = map(itemgetter(1), libs)
+    lib_base = list(map(itemgetter(0), libs)) # tallent
+    lib_size = list(map(itemgetter(1), libs)) # tallent
 
     # Next, split off parallel jobs to look up the IBS fetch or op samples
     # within the application or its shared libraries.
```


2. Processing:
   ```sh
  ./amd-ibs-select ibs_annotated_op.csv -o ibs_annotated-winnow.csv
   ```
 

