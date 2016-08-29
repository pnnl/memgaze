/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: memory_coverage.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Compute footprint coverage for a reuse group.
 */

#ifndef _MEMORY_COVERAGE_H
#define _MEMORY_COVERAGE_H

#include "miami_types.h"
#include "slice_references.h"
#include <list>

namespace MIAMI
{

// A reuse group represents a group of references with similar access
// patterns that are part of the same path and are predicted to have
// close by spatial access. A group is described by a stride value,
// average number of iterations, and a group of references that access
// memory within one "stride" of each other. For each member of a group
// we store a spatial offset relative to the group leader.
// In the end we compute a coverage factor for the references that are part 
// of one group, relative to their common access stride.

class MemAccessInfo
{
public:
   MemAccessInfo(addrtype _pc, int _op, int _bwidth, int _offset = 0) :
        pc(_pc), bwidth(_bwidth), offset(_offset)
   {}

   addrtype pc;       // reference address
   int memop;         // memory operand index
   int bwidth;        // number of bytes touched by this ref (its footprint in bytes)
   int offset;        // spatial offset within this memory block, relative to the leader
};

typedef std::list<MemAccessInfo*> MemAccessInfoList;

class MemoryCoverage
{
public:
   MemoryCoverage(int32_t _stride, uint32_t _setIndex, 
             addrtype _pc, int _op, uint32_t bwidth, GFSliceVal& _formula);
   ~MemoryCoverage();
   
   inline addrtype leaderAddress() const { return (leader_pc); }
   inline int leaderMemOp() const        { return (leader_memop); }
   inline uint32_t getSetIndex() const   { return (setIndex); }
   inline int32_t getStrideValue() const { return (stride); }
   inline GFSliceVal& LeaderFormula()    { return (l_formula); }
   
   // add a new member; if delay<0, this will become the new leader;
   // must update delay and offset for all members;
   void addMember(addrtype _pc, int _op, int _offset, int _bwidth);
   
   int getCoverageValue() const  { return (stride - unusedBytes); }
   int getLargestGap() const;
   
private:
   addrtype leader_pc;
   int leader_memop;
   uint32_t setIndex;
   int unusedBytes;
   int32_t stride;
   char *coverage;
   GFSliceVal l_formula;
   int largest_gap;
   bool gap_computed;
   
//   MemAccessInfoList reflist;
};

typedef std::list<MemoryCoverage*> MCList;

}  /* namespace MIAMI */

#endif
