-*-Mode: markdown;-*- [outline]
=============================================================================

-----------------------------------------------------------------------------
Execution Pipeline
-----------------------------------------------------------------------------

CpuUnits
----------------------------------------

/* Declare all the execution units here 
 * Include also the issue ports. Will create templates that take
 * issue ports into account.
 * List also writeback bus ports. These are used to write back the 
 * results and must match the issue port and the data type of the result.
 */



WindowSize
----------------------------------------

/* I should specify the window size for this machine. This information
 * is used to compute memory overlap & stalls using the mechanistic model.
 */


RetireRate
----------------------------------------

/* Write other rules that limit the number of instructions that can be
 * executed in parallel 
 */
/* This example is from an Itanium machine model. Keep it as reference.
Maximum 6 from I_M, I_I, I_F, I_B {"at most 6 instructions issued per cycle"};
*/


-----------------------------------------------------------------------------
Memory hierarchy
-----------------------------------------------------------------------------


<level> [ <num-blocks>, <block-size>, <entry-size>, <assoc>,
         <fill-bw>, <fill-level>, <fill-penalty> ]

- <num-blocks>: cache-size = <num-blocks> * <block-size>

  An asterisk (*) seems to mean unknown/infinite/irrelevant (e.g., DRAM).

- <block-size>: The logical size in bytes of an entry into this memory
  level. For example, for TLB is the size of a memory page.

  The block size is used to understand spatial reuses to the same
  entry, and it is the block size used for measuring the memory reuse
  distance for that level.
			  
- <entry-size>: The physical size in bytes of an entry into this
  level.  For example, a TLB entry is only 8-16 bytes (I do not know
  the actual value, to store the physical page index and the page
  flags.  The entry-size is what is used to compute bandwidth demands
  for a memory level.
  
  ?? An asterisk (*) in this position ("entry size") or in the bandwidth from next level, indicates not to compute bandwidth demands for this level.

- <assoc>:

  An asterisk (*) seems to mean unknown/infinite/irrelevant (e.g., DRAM).


- <fill-bw>: bdwth from a lower cache level on a miss at this level (bytes/cyc)

  A bandwidth value of 0, means the requests are serialized and the
  latency penalty applies in all cases (no overlap of requests).

- <fill-level>: level to go for miss at this level

- <fill-penalty>: penalty in cycles for going to that level


-----------------------------------------------------------------------------
Instructions
-----------------------------------------------------------------------------

/* Write the issue rules for each instruction class 
 */
/* Inner Loop is a special instruction type. It is not an instruction that
 * is found in any instruction set. A node of this type is created when an
 * entrance into an inner loop is detected and it is used purely for
 * constraining reordering of instructions.
 */


-----------------------------------------------------------------------------
Questions
-----------------------------------------------------------------------------

General
----------------------------------------

!!! validate associativity, bandwidth, and penalties (incl. memory speed) !!!

!!! support wider vector instructions

? CPU frequency (?)

? Number of cores / hardware threads

? branch predictors

? shared caches, e.g., shared L3

? number of memory channels (?)
  [not that relevant because there is (at most) one channel per DIMM]

? TLB description assumes small pages

? separate HBM
? different caching modes (e.g., KNL)


SNB -> IVB
----------------------------------------

SNB: 2 x 8.0 GT/s QPI
IVB: 2 x 8.0 GT/s QPI

update memory... otherwise basically a shrink

- dual memory controllers on one version (?)
