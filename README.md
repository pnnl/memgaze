-*-Mode: markdown;-*-

$Id$

Palm Memory
=============================================================================

**Home**:
- https://gitlab.pnnl.gov/perf-lab-hub/palm/palm-memory


**About**: Palm/Memory: The Performance and Architecture Lab Modeling
tool, or Palm, is a modeling tool designed to make application
modeling easier.


**Palm/FastFootprints**: The new FastFootprints is a tool for
low-overhead analysis of memory footprint access diagnostics.
(MIAMI-NW computes footprints, but has overheads of at least ~200x.)
FastFootprints has two methods. The whole-program method reduces the
overhead to 10% by computing upper bounds, but still yields
inter-procedural insight through a call path profile. The precise
method uses additional static analysis and profiling to refine the
upper bounds for intra-procedural loop nests.


**Contributors**:
  - Ozgur Kilic (PNNL)
  - Nathan R. Tallent (PNNL)


Acknowledgements
-----------------------------------------------------------------------------

This work was supported by the U.S. Department of Energy's Office of
Advanced Scientific Computing Research:
- Integrated End-to-end Performance Prediction and Diagnosis
- PNNL DMC Fallacy...

Portions of Palm Memory utilize functionality heavily adapted from MIAMI-NW (https://gitlab.pnnl.gov/perf-lab-hub/palm/miami-nw), an updated version of MIAMI.
The original MIAMI READMEs and license are located in <README> directory.

