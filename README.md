-*-Mode: markdown;-*-

$Id$

URL: https://gitlab.pnnl.gov/perf-lab-hub/palm/palm-task

-----------------------------------------------------------------------------
Palm Task
=============================================================================

Palm/FastFootprints: The Performance and Architecture Lab Modeling
tool, or Palm, is a modeling tool designed to make application
modeling easier.  The new FastFootprints is a tool for low-overhead
analysis of memory footprint access diagnostics.  (MIAMI-NW computes
footprints, but has overheads of at least ~200x.)  FastFootprints has
two methods. The whole-program method reduces the overhead to 10% by
computing upper bounds, but still yields inter-procedural insight
through a call path profile. The precise method uses additional static
analysis and profiling to refine the upper bounds for intra-procedural
loop nests.

-----------------------------------------------------------------------------
Prerequisites
=============================================================================

Environment
  - GCC 4+
  - DynInstAPI 9.3.2 (Palm externals)
  - Xed (Palm externals)
  - binutils 2.26+ (Palm externals)


-----------------------------------------------------------------------------
Building & Installing
=============================================================================

1. Build Palm Externals package:
   https://gitlab.pnnl.gov/perf-lab-hub/palm/palm-externals

2. 

-----------------------------------------------------------------------------
Using
=============================================================================

