/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: memory_coverage.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Compute footprint coverage for a reuse group.
 */

#include "memory_coverage.h"
#include <assert.h>
#include <string.h>

namespace MIAMI
{

MemoryCoverage::MemoryCoverage(int32_t _stride, uint32_t _setIndex, 
             addrtype _pc, int _op, uint32_t bwidth, GFSliceVal& _formula)
{
   int i;
   leader_pc = _pc;
   leader_memop = _op;
   l_formula = _formula;
   unusedBytes = stride = _stride;
   coverage = new char [stride];
   memset (coverage, 0, stride*sizeof(char));
   setIndex = _setIndex;
   gap_computed = false;
//   reflist.push_back (new MemAccessInfo (_pc, _op, _bwidth));
   for (i=0 ; i<bwidth && i<stride ; ++i)
      if (!coverage[i])
      {
         coverage[i] = 1;
         -- unusedBytes;
      }
}

MemoryCoverage::~MemoryCoverage ()
{
#if 0
   MemAccessInfoList::iterator riit = reflist.begin ();
   for ( ; riit!=reflist.end() ; ++riit)
      delete (*riit);
   reflist.clear ();
#endif
   delete[] coverage;
}
   
// add a new member; 
// must update delay and offset for all members ??;
void 
MemoryCoverage::addMember(addrtype _pc, int _op, int _offset, int bwidth);
{
   // currently, I receive offsets that are >=0 and <stride
   assert (_offset>=0 && _offset<stride);
   int i, j;
   for (i=_offset, j=0 ; j<bwidth && i<stride ; ++i, ++j)
      if (!coverage[i])
      {
         coverage[i] = 1;
         -- unusedBytes;
         gap_computed = false;
      }
}

int 
MemoryCoverage::getLargestGap()
{
   // I must traverse the footprint and compute the current largest gap
   if (! gap_computed)
   {
      int i;
      largest_gap = 0;
      for (i=0 ; i<stride ; ++i)
      {
         int local_gap = 0;
         while (i<stride && !coverage[i])
         {
            local_gap += 1;
            ++ i;
         }
         if (local_gap > largest_gap)
            largest_gap = local_gap;
      }
      gap_computed = true;
   }
   return (largest_gap);
}

}  /* namespace MIAMI */
