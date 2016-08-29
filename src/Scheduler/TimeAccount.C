/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: TimeAccount.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements a simple performance database using an 
 * associative map.
 */

#include "TimeAccount.h"
#include "stream_reuse_histograms.h"
#include <iostream>
#include <sstream>

namespace MIAMI
{

# define TIME_FAMILY_MASK                0xff000000

# define MEM_LAT_EXECUTION_TIME          0x01000000
# define MEM_BDWTH_EXECUTION_TIME        0x02000000

# define SCHEDULING_TOTAL_TIME           0x03000000
# define LACK_OF_RESOURCES_LOST_TIME     0x04000000
# define INSTR_PARALLELISM_LOST_TIME     0x05000000
# define VECTORIZATION_LOST_TIME         0x06000000

# define MINIMUM_APPLICATION_TIME        0x08000000
# define MINIMUM_RESOURCES_TIME          0x09000000

# define RETIRED_UOPS                    0x0a000000
# define RETIRED_UOPS_TYPE               0x0b000000

# define MEMORY_OVERLAP_TIME             0x0c000000
# define MEMORY_STALL_TIME               0x0d000000

# define MEMORY_BANDWIDTH_TOTAL_TIME     0x0e000000
# define IMPROVEMENT_FROM_FPGA           0x0f000000
# define FPGA_COMPUTATION_COST           0x10000000
# define FPGA_TRANSFER_COST              0x11000000

# define MEMORY_REFERENCES               0x12000000
# define CACHE_MISSES                    0x13000000
# define NON_STREAMABLE_ACCESS           0x14000000
# define STREAM_BIN_COUNT                0x15000000

# define MEMORY_PENALTY_TOTAL_TIME       0x20000000
# define APPLICATION_DEPENDENCY_TIME     0x21000000
# define LACK_OF_RESOURCES_TOTAL_TIME    0x22000000
# define SCHEDULING_EXTRA_TOTAL_TIME     0x23000000

# define MEMORY_PENALTY_TIME_LEVEL       0x24000000
# define MEMORY_BANDWIDTH_TIME_LEVEL     0x25000000
# define BANDWIDTH_REQUIRED_LEVEL        0x26000000
# define LACK_OF_RESOURCES_TIME          0x30000000
# define SCHEDULING_EXTRA_TIME           0x31000000
# define VECTORIZATION_TIME              0x32000000

# define MEMORY_FOOTPRINT_LEVEL          0x34000000
# define MISS_COUNT_LEVEL                0x35000000

# define RESOURCE_USAGE                  0x40000000

/* define sub-families (the last 4 bits of a key) */
# define TOTAL_VALUE           0
# define IRREGULAR_ACCESS      1
# define UNUSED_MEMORY         2

/* unit restriction rules are passed as a negative integer. I will keep only
 * the least significant 24 bits. But I have to be able to recover the index
 * when I need it.
 * I also have asynchronous resources. The index for these resources starts at 
 * MACHINE_ASYNC_UNIT_BASE.
 */
# define UNIT_INDEX_MASK       0x00fffff0
# define UNIT_INDEX_SHIFT      4
# define SUB_FAMILY_MASK       0x0000000f
# define UNIT_INDEX_SIGN_BIT   0x00800000
# define NEGATIVE_VALUE_MASK   0xfff00000

# define SUB_FAMILY_VALUE(x)   ((x)&SUB_FAMILY_MASK)
# define UNIT_INDEX_VALUE(x)   (((x)&UNIT_INDEX_SIGN_BIT) ? \
          (NEGATIVE_VALUE_MASK|((x)>>UNIT_INDEX_SHIFT)) : \
          ((UNIT_INDEX_MASK&(x))>>UNIT_INDEX_SHIFT))
# define MAKE_TIME_ACCOUNT_KEY(x, y)  ((x)|(((y)<<UNIT_INDEX_SHIFT)&UNIT_INDEX_MASK))
# define MAKE_SUB_FAMILY_KEY(x, y, z) \
          ((x)|(((y)<<UNIT_INDEX_SHIFT)&UNIT_INDEX_MASK)|((z)&SUB_FAMILY_MASK))


bool aggregateValuesForKey (unsigned int key);

TimeAccount::TimeAccount (const TimeAccount &ta)
{
   data = ta.data;
}

TimeAccount& 
TimeAccount::operator = (const TimeAccount &ta)
{
   data = ta.data;
   return (*this);
}

TimeAccount 
TimeAccount::operator + (const TimeAccount& ta) const
{
   TimeAccount temp(*this);
   UiToDoubleMap::const_iterator tait = ta.data.begin();
   for ( ; tait!=ta.data.end() ; ++tait)
   {
      UiToDoubleMap::iterator it2 = temp.data.find (tait->first);
      if (aggregateValuesForKey (tait->first))
         if (it2!=temp.data.end())
            it2->second += tait->second;
         else
            temp.data.insert (UiToDoubleMap::value_type (tait->first, tait->second));
      else
         if (it2 == temp.data.end())
            temp.data.insert (UiToDoubleMap::value_type (tait->first, 0.0));
   }
   return (temp);
}

TimeAccount& 
TimeAccount::operator += (const TimeAccount& ta)
{
   UiToDoubleMap::const_iterator tait = ta.data.begin();
   for ( ; tait!=ta.data.end() ; ++tait)
   {
      UiToDoubleMap::iterator it2 = data.find (tait->first);
      if (aggregateValuesForKey (tait->first))
         if (it2!=data.end())
            it2->second += tait->second;
         else
            data.insert (UiToDoubleMap::value_type (tait->first, tait->second));
      else
         if (it2 == data.end())
            data.insert (UiToDoubleMap::value_type (tait->first, 0.0));
   }
   return (*this);
}

TimeAccount 
TimeAccount::operator - (const TimeAccount& ta) const
{
   TimeAccount temp(*this);
   UiToDoubleMap::const_iterator tait = ta.data.begin();
   for ( ; tait!=ta.data.end() ; ++tait)
   {
      UiToDoubleMap::iterator it2 = temp.data.find(tait->first);
      if (aggregateValuesForKey (tait->first))
         if (it2!=temp.data.end())
            it2->second -= tait->second;
         else
            temp.data.insert (UiToDoubleMap::value_type (tait->first, -tait->second));
      else
         if (it2 == temp.data.end())
            temp.data.insert (UiToDoubleMap::value_type (tait->first, 0.0));
   }
   return (temp);
}

TimeAccount& 
TimeAccount::operator -= (const TimeAccount& ta)
{
   UiToDoubleMap::const_iterator tait = ta.data.begin();
   for ( ; tait!=ta.data.end() ; ++tait)
   {
      UiToDoubleMap::iterator it2 = data.find(tait->first);
      if (aggregateValuesForKey (tait->first))
         if (it2!=data.end())
            it2->second -= tait->second;
         else
            data.insert (UiToDoubleMap::value_type (tait->first, -tait->second));
      else
         if (it2 == data.end())
            data.insert (UiToDoubleMap::value_type (tait->first, 0.0));
   }
   return (*this);
}

TimeAccount 
TimeAccount::operator * (double factor) const
{
   TimeAccount temp(*this);
   UiToDoubleMap::iterator tait = temp.data.begin();
   for ( ; tait!=temp.data.end() ; ++tait)
   {
      tait->second *= factor;
   }
   return (temp);
}

TimeAccount& 
TimeAccount::operator *= (double factor)
{
   UiToDoubleMap::iterator tait = data.begin();
   for ( ; tait!=data.end() ; ++tait)
   {
      tait->second *= factor;
   }
   return (*this);
}

TimeAccount 
TimeAccount::operator / (double factor) const
{
   TimeAccount temp(*this);
   UiToDoubleMap::iterator tait = temp.data.begin();
   for ( ; tait!=temp.data.end() ; ++tait)
   {
      tait->second /= factor;
   }
   return (temp);
}

TimeAccount& 
TimeAccount::operator /= (double factor)
{
   UiToDoubleMap::iterator tait = data.begin();
   for ( ; tait!=data.end() ; ++tait)
   {
      tait->second /= factor;
   }
   return (*this);
}

void 
TimeAccount::addFpgaComputeTime (double cycles)
{
   unsigned int keyall = IMPROVEMENT_FROM_FPGA;
   unsigned int key = FPGA_COMPUTATION_COST;

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(key, cycles));

   tait = data.find(keyall);
   if (tait!=data.end())
      tait->second -= cycles;
   else
      data.insert (UiToDoubleMap::value_type (keyall, -cycles));
}

void 
TimeAccount::addFpgaTransferTime (double cycles)
{
   unsigned int keyall = IMPROVEMENT_FROM_FPGA;
   unsigned int key = FPGA_TRANSFER_COST;

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(key, cycles));

   tait = data.find(keyall);
   if (tait!=data.end())
      tait->second -= cycles;
   
   // if we do not have already an entry (created when computing the 
   // computation cost) then do not add one. This is a scope that contains
   // not inlined function calls.
   // else
   //    data.insert (UiToDoubleMap::value_type (keyall, -cycles));
}

void 
TimeAccount::addNonFpgaComputeTime (double cycles)
{
   unsigned int key = IMPROVEMENT_FROM_FPGA;

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end() && tait->second<0.0)
      tait->second += cycles;
}

void 
TimeAccount::addMemFootprintLevel (int level, double footprint)
{
   unsigned int key = MAKE_TIME_ACCOUNT_KEY (MEMORY_FOOTPRINT_LEVEL, level);

   UiToDoubleMap::iterator tait = data.find (key);
   if (tait!=data.end()) {
      tait->second += footprint;
   }
   else {
      data.insert (UiToDoubleMap::value_type (key, footprint));
   }
}

// Is this routine ever called? Hmm, probably for the resource usage database.
void 
TimeAccount::addExecutionTime (double cycles)
{
   unsigned int key = SCHEDULING_TOTAL_TIME;
   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(key, cycles));
}

void 
TimeAccount::addResourceUsage (int unit, double cycles)
{
   unsigned int key = MAKE_TIME_ACCOUNT_KEY (RESOURCE_USAGE, unit);

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(key, cycles));
}

void 
TimeAccount::addDependencyTime (double cycles)
{
   unsigned int keyall = MEM_LAT_EXECUTION_TIME;
   unsigned int keytot = SCHEDULING_TOTAL_TIME;
   unsigned int key = APPLICATION_DEPENDENCY_TIME;

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(key, cycles));

   tait = data.find(keytot);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(keytot, cycles));

   tait = data.find(keyall);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(keyall, cycles));
}

void 
TimeAccount::addResourcesTime (int unit, double cycles)
{
   unsigned int keyall = MEM_LAT_EXECUTION_TIME;
   unsigned int keytot = SCHEDULING_TOTAL_TIME;
   unsigned int key2 = LACK_OF_RESOURCES_TOTAL_TIME;
   unsigned int key = MAKE_TIME_ACCOUNT_KEY (LACK_OF_RESOURCES_TIME, unit);

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(key, cycles));

   tait = data.find(key2);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(key2, cycles));

   tait = data.find(keytot);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(keytot, cycles));

   tait = data.find(keyall);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(keyall, cycles));
}

void 
TimeAccount::addSchedulingTime (int unit, double cycles, int verbose)
{
   unsigned int keyall = MEM_LAT_EXECUTION_TIME;
   unsigned int keytot = SCHEDULING_TOTAL_TIME;
   unsigned int key2 = SCHEDULING_EXTRA_TOTAL_TIME;
   unsigned int keyfam = (verbose ? SCHEDULING_EXTRA_TIME : LACK_OF_RESOURCES_TIME);
   unsigned int key = MAKE_TIME_ACCOUNT_KEY (keyfam, unit);

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(key, cycles));

   tait = data.find(key2);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(key2, cycles));

   tait = data.find(keytot);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(keytot, cycles));

   tait = data.find(keyall);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(keyall, cycles));
}

void
TimeAccount::computeBandwidthTime (const Machine *tmach)
{
   int i;
   unsigned int schedkey = SCHEDULING_TOTAL_TIME;
   unsigned int bdwthkey;

   MemLevelAssocTable *memLevels = tmach->getMemoryLevels ();
   int numLevels = memLevels->NumberOfElements ();
   
   double max_penalty = 0.;
   double cpu_time = 0.;
   double memlat_penalty = 0.;
   UiToDoubleMap::iterator tait = data.find (schedkey);
   if (tait != data.end())
      cpu_time = tait->second;
   for (i=0 ; i<numLevels ; ++i)
   {
      unsigned int key = MAKE_TIME_ACCOUNT_KEY (BANDWIDTH_REQUIRED_LEVEL, i);
      tait = data.find (key);
      if (tait!=data.end() && tait->second>0)
      {
         MemoryHierarchyLevel *mhl = memLevels->getElementAtIndex (i);
         double peak_bdwth = mhl->getBandwidth ();
         if (peak_bdwth < 0)  // means unlimited, not a constraint
            continue;
         else if (peak_bdwth == 0)
         {
            // bandwidth does not make sense for this level
            // I should use the latency penalty for both the lower and upper bounds, FIXME
            unsigned int latkey = MAKE_TIME_ACCOUNT_KEY (MEMORY_PENALTY_TIME_LEVEL, i);
            tait = data.find (latkey);
            if (tait != data.end())
               memlat_penalty += tait->second;
         } else
         {
            double bd_penalty = tait->second / peak_bdwth;
            if (bd_penalty > cpu_time)
            {
               bd_penalty -= cpu_time;   // compute delay due to bandwidth
               bdwthkey = MAKE_TIME_ACCOUNT_KEY (MEMORY_BANDWIDTH_TIME_LEVEL, i);
               // I shoould not have such an entry already
               assert (data.find (bdwthkey) == data.end());
               data.insert (UiToDoubleMap::value_type (bdwthkey, bd_penalty));
               if (bd_penalty > max_penalty)
                  max_penalty = bd_penalty;
            }
         }
      }
   }
   
   // total penalty due to bandwidth is the maximum of penalties from each
   // level. Check if any level is bandwidth starved.
   if (max_penalty > 0)
   {
      bdwthkey = MEMORY_BANDWIDTH_TOTAL_TIME;
      assert (data.find (bdwthkey) == data.end());
      data.insert (UiToDoubleMap::value_type (bdwthkey, max_penalty));
   }
   // compute also the total execution time if all data could be perfectly prefetched
   cpu_time = max_penalty+cpu_time+memlat_penalty;
   if (cpu_time > 0.01)
   {
      bdwthkey = MEM_BDWTH_EXECUTION_TIME;
      assert (data.find (bdwthkey) == data.end());
      data.insert (UiToDoubleMap::value_type (bdwthkey, cpu_time));
   }
}

void 
TimeAccount::addMemoryOverlapTime (double cycles)
{
   unsigned int key = MEMORY_OVERLAP_TIME;

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(key, cycles));
}

void 
TimeAccount::addMemoryStallTime (double cycles)
{
   unsigned int keyall = MEM_LAT_EXECUTION_TIME;
   unsigned int key = MEMORY_STALL_TIME;

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(key, cycles));
   
   tait = data.find(keyall);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(keyall, cycles));
}

void 
TimeAccount::addRetiredUops (int type, double count)
{
   unsigned int keyall = RETIRED_UOPS;
   unsigned int key = MAKE_TIME_ACCOUNT_KEY (RETIRED_UOPS_TYPE, type);

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += count;
   else
      data.insert (UiToDoubleMap::value_type(key, count));

   tait = data.find(keyall);
   if (tait!=data.end())
      tait->second += count;
   else
      data.insert (UiToDoubleMap::value_type(keyall, count));
}

void 
TimeAccount::addMemReferenceCount(double count)
{
   unsigned int key = MEMORY_REFERENCES;

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += count;
   else
      data.insert (UiToDoubleMap::value_type(key, count));
}

void 
TimeAccount::addCacheMissesCount(double count)
{
   unsigned int key = CACHE_MISSES;

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += count;
   else
      data.insert (UiToDoubleMap::value_type(key, count));
}

void 
TimeAccount::addNoStreamCount(double count)
{
   unsigned int key = NON_STREAMABLE_ACCESS;

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += count;
   else
      data.insert (UiToDoubleMap::value_type(key, count));
}

void 
TimeAccount::addStreamBinCount(int bin, double count)
{
   unsigned int key = MAKE_TIME_ACCOUNT_KEY (STREAM_BIN_COUNT, bin);

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += count;
   else
      data.insert (UiToDoubleMap::value_type(key, count));
}

void 
TimeAccount::addMissCountLevel (int level, double misses, const Machine *mach)
{
   MemoryHierarchyLevel *mhl = mach->getMemoryLevels()->getElementAtIndex(level);
   addMissCountLevel(level, misses, mhl);
}

void 
TimeAccount::addMissCountLevel (int level, double misses, const MemoryHierarchyLevel *mhl)
{
   unsigned int keyall = MEM_LAT_EXECUTION_TIME;
   unsigned int keytot = MEMORY_PENALTY_TOTAL_TIME;
   unsigned int key3 = MAKE_TIME_ACCOUNT_KEY (BANDWIDTH_REQUIRED_LEVEL, level);
   unsigned int key2 = MAKE_TIME_ACCOUNT_KEY (MEMORY_PENALTY_TIME_LEVEL, level);
   unsigned int key = MAKE_TIME_ACCOUNT_KEY (MISS_COUNT_LEVEL, level);

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += misses;
   else
      data.insert (UiToDoubleMap::value_type(key, misses));
   
   double cycles = misses * mhl->getMissPenalty ();
   int entry_size = mhl->getEntrySize ();
   double bdwth = (entry_size>0? misses*entry_size : 0.);
   
   tait = data.find(key2);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(key2, cycles));

   tait = data.find(keytot);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(keytot, cycles));

   tait = data.find(keyall);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(keyall, cycles));

   if (bdwth > 0.0)
   {
      tait = data.find(key3);
      if (tait!=data.end())
         tait->second += bdwth;
      else
         data.insert (UiToDoubleMap::value_type(key3, bdwth));
   }
}

void 
TimeAccount::addTotalMissCountLevel (int level, double misses)
{
   unsigned int key = MAKE_TIME_ACCOUNT_KEY (MISS_COUNT_LEVEL, level);

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += misses;
   else
      data.insert (UiToDoubleMap::value_type(key, misses));
}
   
void 
TimeAccount::addFragMissCountLevel (int level, double misses)
{
   unsigned int key = MAKE_SUB_FAMILY_KEY (MISS_COUNT_LEVEL, level, UNUSED_MEMORY);

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += misses;
   else
      data.insert (UiToDoubleMap::value_type(key, misses));
}
   
void 
TimeAccount::addIrregMissCountLevel (int level, double misses)
{
   unsigned int key = MAKE_SUB_FAMILY_KEY (MISS_COUNT_LEVEL, level, IRREGULAR_ACCESS);

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += misses;
   else
      data.insert (UiToDoubleMap::value_type(key, misses));
}
   
void 
TimeAccount::addReuseMissCountLevel (int level, double misses, const Machine *mach)
{
   MemoryHierarchyLevel *mhl = mach->getMemoryLevels()->getElementAtIndex(level);
   addReuseMissCountLevel(level, misses, mhl);
}

void 
TimeAccount::addReuseMissCountLevel (int level, double misses, const MemoryHierarchyLevel *mhl)
{
   unsigned int keytot = MEMORY_PENALTY_TOTAL_TIME;
   unsigned int key3 = MAKE_TIME_ACCOUNT_KEY (BANDWIDTH_REQUIRED_LEVEL, level);
   unsigned int key2 = MAKE_TIME_ACCOUNT_KEY (MEMORY_PENALTY_TIME_LEVEL, level);
   unsigned int key = MAKE_TIME_ACCOUNT_KEY (MISS_COUNT_LEVEL, level);

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += misses;
   else
      data.insert (UiToDoubleMap::value_type(key, misses));
   
   double cycles = misses * mhl->getMissPenalty ();
   int entry_size = mhl->getEntrySize ();
   double bdwth = (entry_size>0? misses*entry_size : 0.);
   tait = data.find(key2);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(key2, cycles));

   tait = data.find(keytot);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(keytot, cycles));

   if (bdwth > 0.0)
   {
      tait = data.find(key3);
      if (tait!=data.end())
         tait->second += bdwth;
      else
         data.insert (UiToDoubleMap::value_type(key3, bdwth));
   }
}

void
TimeAccount::addReuseSchedulingTime (double cycles)
{
   unsigned int key = SCHEDULING_TOTAL_TIME;

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(key, cycles));
}

double
TimeAccount::getTotalCpuTime ()
{
   unsigned int key = SCHEDULING_TOTAL_TIME;

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      return (tait->second);
   else
      return (0);
}

double
TimeAccount::getNoDependencyTime ()
{
   unsigned int key = MINIMUM_RESOURCES_TIME;

   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      return (tait->second);
   else
      return (0);
}

double 
TimeAccount::getDisplayValue (unsigned int key, const Machine *mach)
{
//   int unit = UNIT_INDEX_VALUE (key);
   double value = 0.0;
   UiToDoubleMap::iterator tait = data.find(key);
   if (tait != data.end())
   {
      switch (key & TIME_FAMILY_MASK)
      {
         case MEM_LAT_EXECUTION_TIME:
         case MEM_BDWTH_EXECUTION_TIME:
         case SCHEDULING_TOTAL_TIME:
         case MINIMUM_APPLICATION_TIME:
         case MINIMUM_RESOURCES_TIME:
         case MEMORY_OVERLAP_TIME:
         case MEMORY_STALL_TIME:
         case RETIRED_UOPS:
         case RETIRED_UOPS_TYPE:
         case MEMORY_REFERENCES:
         case CACHE_MISSES:
         case NON_STREAMABLE_ACCESS:
         case STREAM_BIN_COUNT:
         case MEMORY_PENALTY_TOTAL_TIME:
         case MEMORY_BANDWIDTH_TOTAL_TIME:
         case APPLICATION_DEPENDENCY_TIME:
         case LACK_OF_RESOURCES_TOTAL_TIME:
         case SCHEDULING_EXTRA_TOTAL_TIME:
         case MEMORY_PENALTY_TIME_LEVEL:
         case MEMORY_BANDWIDTH_TIME_LEVEL:
         case MISS_COUNT_LEVEL:
         case RESOURCE_USAGE:
         case LACK_OF_RESOURCES_TIME:
         case VECTORIZATION_TIME:
         case SCHEDULING_EXTRA_TIME:
         case FPGA_COMPUTATION_COST:
         case FPGA_TRANSFER_COST:
         case MEMORY_FOOTPRINT_LEVEL:
         case IMPROVEMENT_FROM_FPGA:
            value = tait->second;
            break;

         // display bdwth in bytes/cycle (optionally in MB/s);
         case BANDWIDTH_REQUIRED_LEVEL:
            value = tait->second;
            tait = data.find (SCHEDULING_TOTAL_TIME);
            // for MB/s I should read the CPU freq from machine description
   //         value = value * 900 / tait->second;
            if (tait!=data.end() && tait->second>0.5)
               value = value / tait->second;
            else
               value = 0.0;
            break;
   #if 0
         case IMPROVEMENT_FROM_FPGA:
            value = tait->second;
            if (value < 0.0)
            {
               tait = data.find (MINIMUM_RESOURCES_TIME);
               value += tait->second;
            }
            break;
   #endif
         case LACK_OF_RESOURCES_LOST_TIME:
         case INSTR_PARALLELISM_LOST_TIME:
         case VECTORIZATION_LOST_TIME:
         // add SCHEDULING_TOTAL_TIME to current value
            value = tait->second;
            tait = data.find (SCHEDULING_TOTAL_TIME);
            value += tait->second;
            break;
         
         default:
            assert (!"Invalid scheduling time category.");
            value = 0;
            break;
      }
   }
   return (value);
}

void 
TimeAccount::addApplicationMinimumTime (double cycles)
{
   unsigned int key = MINIMUM_APPLICATION_TIME;
   unsigned int key2 = LACK_OF_RESOURCES_LOST_TIME;
   UiToDoubleMap::iterator tait = data.find(key);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type(key, cycles));

   tait = data.find (key2);
   if (tait!=data.end())
      tait->second -= cycles;
   else
      data.insert (UiToDoubleMap::value_type(key2, -cycles));
}

void 
TimeAccount::addResourcesMinimumTime (double cycles)
{
   unsigned int key = MINIMUM_RESOURCES_TIME;
   unsigned int key2 = INSTR_PARALLELISM_LOST_TIME;
   UiToDoubleMap::iterator tait = data.find (key);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type (key, cycles));

   tait = data.find (key2);
   if (tait!=data.end())
      tait->second -= cycles;
   else
      data.insert (UiToDoubleMap::value_type (key2, -cycles));
}

void 
TimeAccount::addIdealVectorizationTime (int unit, double cycles)
{
   unsigned int key = MAKE_TIME_ACCOUNT_KEY (VECTORIZATION_TIME, unit);
   unsigned int key2 = VECTORIZATION_LOST_TIME;
   UiToDoubleMap::iterator tait = data.find (key);
   if (tait!=data.end())
      tait->second += cycles;
   else
      data.insert (UiToDoubleMap::value_type (key, cycles));

   tait = data.find (key2);
   if (tait!=data.end())
      tait->second -= cycles;
   else
      data.insert (UiToDoubleMap::value_type (key2, -cycles));
}

std::string 
getTANameForKey(unsigned int key, const Machine *mach)
{
   int unit = UNIT_INDEX_VALUE (key);
   int subkey = SUB_FAMILY_VALUE (key);
   std::ostringstream oss;
   MemoryHierarchyLevel *mhl;
   assert (subkey==TOTAL_VALUE || (key&TIME_FAMILY_MASK)==MISS_COUNT_LEVEL);

   switch (key & TIME_FAMILY_MASK)
   {
      case MEM_LAT_EXECUTION_TIME:
         assert (unit == 0);
         oss << "Memory latency execution time";
         break;

      case MEM_BDWTH_EXECUTION_TIME:
         assert (unit == 0);
         oss << "Memory bandwidth execution time";
         break;
      
      case SCHEDULING_TOTAL_TIME:
         assert (unit == 0);
         oss << "Total scheduling time";
         break;
      
      case MINIMUM_APPLICATION_TIME:
         assert (unit == 0);
         oss << "Infinite resources time";
         break;
      
      case MINIMUM_RESOURCES_TIME:
         assert (unit == 0);
         oss << "No dependencies time";
         break;

      case MEMORY_OVERLAP_TIME:
         assert (unit == 0);
         oss << "Memory overlap time";
         break;

      case MEMORY_STALL_TIME:
         assert (unit == 0);
         oss << "Memory stall time";
         break;

      case RETIRED_UOPS:
         assert (unit == 0);
         oss << "Retired micro-ops";
         break;

      case RETIRED_UOPS_TYPE:
         assert (unit>=0);
         oss << "Retired " << RetiredUopToString((RetiredUopType)unit) << " micro-ops";
         break;
      
      case MEMORY_REFERENCES:
         assert (unit == 0);
         oss << "Num memory accesses";
         break;
         
      case CACHE_MISSES:
         assert (unit == 0);
         oss << "Num cache misses";
         break;
         
      case NON_STREAMABLE_ACCESS:
         assert (unit == 0);
         oss << "Non-streamable accesses";
         break;
         
      case STREAM_BIN_COUNT:
         assert (unit>=0);
         oss << StreamRD::getNameForStreamBin((StreamRD::StreamCountBin)unit) << " streams";
         break;

      case IMPROVEMENT_FROM_FPGA:
         assert (unit == 0);
         oss << "Performance gain from FPGA";
         break;

      case FPGA_COMPUTATION_COST:
         assert (unit == 0);
         oss << "FPGA computation cost";
         break;

      case FPGA_TRANSFER_COST:
         assert (unit == 0);
         oss << "FPGA data transfer cost";
         break;

      case LACK_OF_RESOURCES_LOST_TIME:
         assert (unit == 0);
         oss << "Maximum improvement from additional CPU resources";
         break;
         
      case INSTR_PARALLELISM_LOST_TIME:
         assert (unit == 0);
         oss << "Maximum improvement from additional instruction parallelism";
         break;
         
      case VECTORIZATION_LOST_TIME:
         assert (unit == 0);
         oss << "Maximum improvement from ideal vectorization";
         break;
         
      case MEMORY_PENALTY_TOTAL_TIME:
         assert (unit == 0);
         oss << "Memory penalty total time";
         break;

      case MEMORY_BANDWIDTH_TOTAL_TIME:
         assert (unit == 0);
         oss << "Memory bandwidth total delays";
         break;

      case APPLICATION_DEPENDENCY_TIME:
         assert (unit == 0);
         oss << "Application dependency time";
         break;
      
      case LACK_OF_RESOURCES_TOTAL_TIME:
         assert (unit == 0);
         oss << "Lack of resources total time";
         break;
      
      case SCHEDULING_EXTRA_TOTAL_TIME:
         assert (unit == 0);
         oss << "Scheduling total extra time";
         break;
      
      case MEMORY_PENALTY_TIME_LEVEL:
         assert (unit>=0);
         mhl = mach->getMemoryLevels ()->getElementAtIndex (unit);
         oss << "Memory penalty " << mhl->getLongName ();
         break;

      case MEMORY_BANDWIDTH_TIME_LEVEL:
         assert (unit>=0);
         mhl = mach->getMemoryLevels ()->getElementAtIndex (unit);
         oss << "Bandwidth delays " << mhl->getNextLevel ()
             << "-" << mhl->getLongName ();
         break;

      case BANDWIDTH_REQUIRED_LEVEL:
         assert (unit>=0);
         mhl = mach->getMemoryLevels ()->getElementAtIndex (unit);
         oss << "Bandwidth requirements " << mhl->getNextLevel ()
             << "-" << mhl->getLongName ();
         break;

      case MISS_COUNT_LEVEL:
         assert (unit>=0);
         mhl = mach->getMemoryLevels ()->getElementAtIndex (unit);
         if (subkey == TOTAL_VALUE)
            oss << "Miss count " << mhl->getLongName ();
         else if (subkey == IRREGULAR_ACCESS)
            oss << "Irregular access misses " << mhl->getLongName ();
         else if (subkey == UNUSED_MEMORY)
            oss << "Non stride one misses " << mhl->getLongName ();
         else   // it should not happen
            assert (!"Invalid sub family.");
         break;

      case MEMORY_FOOTPRINT_LEVEL:
         assert (unit>=0);
         mhl = mach->getMemoryLevels ()->getElementAtIndex (unit);
         oss << "Footprint " << mhl->getLongName ();
         break;

      case LACK_OF_RESOURCES_TIME:
         if (unit>=MACHINE_ASYNC_UNIT_BASE)
         {
            const char* uName = mach->getNameForAsyncResource(unit-MACHINE_ASYNC_UNIT_BASE);
            oss << "Bottleneck on async resource " << uName;
         } else
         if (unit>=0)
         {
            int unitIdx = 0;
            const char* uName = mach->getNameForUnit(unit, unitIdx);
            oss << "Bottleneck on unit " << uName << "[" 
                << unitIdx << "]";
         }
         else
            oss << "Restriction from rule " 
                << mach->getNameForRestrictionWithId(-unit);
         break;

      case VECTORIZATION_TIME:
         if (unit>=MACHINE_ASYNC_UNIT_BASE)
         {
            const char* uName = mach->getNameForAsyncResource(unit-MACHINE_ASYNC_UNIT_BASE);
            oss << "Vectorization bottleneck on async resource " << uName;
         } else
         if (unit>=0)
         {
            int unitIdx = 0;
            const char* uName = mach->getNameForUnit(unit, unitIdx);
            oss << "Vectorization bottleneck on unit " << uName << "[" 
                << unitIdx << "]";
         }
         else
            oss << "Vectorization restriction from rule " 
                << mach->getNameForRestrictionWithId(-unit);
         break;

      case RESOURCE_USAGE:
         if (unit>=MACHINE_ASYNC_UNIT_BASE)
         {
            const char* uName = mach->getNameForAsyncResource(unit-MACHINE_ASYNC_UNIT_BASE);
            oss << "Usage of async resource " << uName;
         } else
         if (unit>=0)
         {
            int unitIdx = 0;
            const char* uName = mach->getNameForUnit(unit, unitIdx);
            oss << "Cycles unit " << uName << "[" 
                << unitIdx << "] was in use";
         }
         else
            assert(!"Unit should not be negative for this metric");
         break;
      
      case SCHEDULING_EXTRA_TIME:
         if (unit==DEFAULT_UNIT)
            oss << "Extra time due to undetermined unit.";
         else
         {
            if (unit>=0)
            {
               // Asynchronous resources affect only the schedule bound
               assert (unit < MACHINE_ASYNC_UNIT_BASE);
               int unitIdx = 0;
               const char* uName = mach->getNameForUnit(unit, unitIdx);
               oss << "Extra time due to unit " << uName << "["
                   << unitIdx << "]";
            }
            else
               oss << "Extra time due to rule " 
                   << mach->getNameForRestrictionWithId(-unit);
         }
         break;
      
      default:
         assert (!"Invalid scheduling time category.");
         break;
   }
   return (oss.str());
}

std::string  
getTAShortNameForKey (unsigned int key, const Machine *mach)
{
   int unit = UNIT_INDEX_VALUE (key);
   int subkey = SUB_FAMILY_VALUE (key);
   std::ostringstream oss;
   MemoryHierarchyLevel *mhl;
   assert (subkey==TOTAL_VALUE || (key&TIME_FAMILY_MASK)==MISS_COUNT_LEVEL);

   switch (key & TIME_FAMILY_MASK)
   {
      case MEM_LAT_EXECUTION_TIME:
         assert (unit == 0);
         oss << "MemLatExecTime";
         break;

      case MEM_BDWTH_EXECUTION_TIME:
         assert (unit == 0);
         oss << "MemBdwthExecTime";
         break;
      
      case SCHEDULING_TOTAL_TIME:
         assert (unit == 0);
         oss << "CPU_Time";
         break;
      
      case MINIMUM_APPLICATION_TIME:
         assert (unit == 0);
         oss << "InfCpuRes";
         break;
      
      case MINIMUM_RESOURCES_TIME:
         assert (unit == 0);
         oss << "NoDepend";
         break;

      case IMPROVEMENT_FROM_FPGA:
         assert (unit == 0);
         oss << "FPGA_Gain";
         break;

      case FPGA_COMPUTATION_COST:
         assert (unit == 0);
         oss << "FpgaCpuCost";
         break;

      case FPGA_TRANSFER_COST:
         assert (unit == 0);
         oss << "FpgaBdwthCost";
         break;

      case LACK_OF_RESOURCES_LOST_TIME:
         assert (unit == 0);
         oss << "GainExtraRes";
         break;

      case MEMORY_OVERLAP_TIME:
         assert (unit == 0);
         oss << "MemoryOverlap";
         break;

      case MEMORY_STALL_TIME:
         assert (unit == 0);
         oss << "MemoryStalls";
         break;

      case RETIRED_UOPS:
         assert (unit == 0);
         oss << "RetiredUops";
         break;

      case RETIRED_UOPS_TYPE:
         assert (unit>=0);
         oss << RetiredUopToString((RetiredUopType)unit); // << "Uops";
         break;

      case MEMORY_REFERENCES:
         assert (unit == 0);
         oss << "MemoryAccesses";
         break;
         
      case CACHE_MISSES:
         assert (unit == 0);
         oss << "CacheMisses";
         break;
         
      case NON_STREAMABLE_ACCESS:
         assert (unit == 0);
         oss << "NotAStream";
         break;
         
      case STREAM_BIN_COUNT:
         assert (unit>=0);
         oss << StreamRD::getNameForStreamBin((StreamRD::StreamCountBin)unit) << "Streams";
         break;
         
      case INSTR_PARALLELISM_LOST_TIME:
         assert (unit == 0);
         oss << "GainExtraILP";
         break;
      
      case VECTORIZATION_LOST_TIME:
         assert (unit == 0);
         oss << "GainVectorize";
         break;
      
      case MEMORY_PENALTY_TOTAL_TIME:
         assert (unit == 0);
         oss << "MemoryTime";
         break;

      case MEMORY_BANDWIDTH_TOTAL_TIME:
         assert (unit == 0);
         oss << "BdwthDelays";
         break;

      case APPLICATION_DEPENDENCY_TIME:
         assert (unit == 0);
         oss << "AppDepTime";
         break;
      
      case LACK_OF_RESOURCES_TOTAL_TIME:
         assert (unit == 0);
         oss << "CPUBottleneck";
         break;
      
      case SCHEDULING_EXTRA_TOTAL_TIME:
         assert (unit == 0);
         oss << "SchedExtraTime";
         break;
      
      case MEMORY_PENALTY_TIME_LEVEL:
         assert (unit>=0);
         mhl = mach->getMemoryLevels ()->getElementAtIndex (unit);
         oss << "Penalty_" << mhl->getLongName ();
         break;

      case MEMORY_BANDWIDTH_TIME_LEVEL:
         assert (unit>=0);
         mhl = mach->getMemoryLevels ()->getElementAtIndex (unit);
         oss << mhl->getNextLevel () << "-" << mhl->getLongName () 
             << "_BdwthDelay";
         break;

      case BANDWIDTH_REQUIRED_LEVEL:
         assert (unit>=0);
         mhl = mach->getMemoryLevels ()->getElementAtIndex (unit);
         oss << mhl->getNextLevel () << "-" << mhl->getLongName () 
             << "_ReqBdwth";
         break;

      case MISS_COUNT_LEVEL:
         assert (unit>=0);
         mhl = mach->getMemoryLevels ()->getElementAtIndex (unit);
         if (subkey == TOTAL_VALUE)
            oss << "Misses_" << mhl->getLongName ();
         else if (subkey == IRREGULAR_ACCESS)
            oss << "IrregMiss_" << mhl->getLongName ();
         else if (subkey == UNUSED_MEMORY)
            oss << "FragMiss_" << mhl->getLongName ();
         else   // it should not happen
            assert (!"Invalid sub family.");
         break;

      case MEMORY_FOOTPRINT_LEVEL:
         assert (unit>=0);
         mhl = mach->getMemoryLevels ()->getElementAtIndex (unit);
         oss << mhl->getLongName () << "_FootPrint ";
         break;
      
      case LACK_OF_RESOURCES_TIME:
         if (unit>=MACHINE_ASYNC_UNIT_BASE)
         {
            const char* uName = mach->getNameForAsyncResource(unit-MACHINE_ASYNC_UNIT_BASE);
            oss << "CPU_" << uName;
         } else
         if (unit>=0)
         {
            int unitIdx = 0;
            const char* uName = mach->getNameForUnit(unit, unitIdx);
            oss << "CPU_" << uName << "[" 
                << unitIdx << "]";
         }
         else
            oss << "RESTRICT_" << mach->getNameForRestrictionWithId(-unit);
         break;
      
      case VECTORIZATION_TIME:
         if (unit>=MACHINE_ASYNC_UNIT_BASE)
         {
            const char* uName = mach->getNameForAsyncResource(unit-MACHINE_ASYNC_UNIT_BASE);
            oss << "V_CPU_" << uName;
         } else
         if (unit>=0)
         {
            int unitIdx = 0;
            const char* uName = mach->getNameForUnit(unit, unitIdx);
            oss << "V_CPU_" << uName << "[" 
                << unitIdx << "]";
         }
         else
            oss << "V_RESTRICT_" << mach->getNameForRestrictionWithId(-unit);
         break;
      
      case RESOURCE_USAGE:
         if (unit>=MACHINE_ASYNC_UNIT_BASE)
         {
            const char* uName = mach->getNameForAsyncResource(unit-MACHINE_ASYNC_UNIT_BASE);
            oss << "InUse_" << uName;
         } else
         if (unit>=0)
         {
            int unitIdx = 0;
            const char* uName = mach->getNameForUnit(unit, unitIdx);
            oss << "InUse_" << uName << "[" 
                << unitIdx << "]";
         }
         else
            assert(!"Unit should not be negative in this case");
         break;
      
      case SCHEDULING_EXTRA_TIME:
         if (unit==DEFAULT_UNIT)
            oss << "SET_Unknown";
         else
            if (unit>=0)
            {
               assert (unit < MACHINE_ASYNC_UNIT_BASE);
               int unitIdx = 0;
               const char* uName = mach->getNameForUnit(unit, unitIdx);
               oss << "SET_" << uName << "[" << unitIdx << "]";
            }
            else
               oss << "SET_RESTRICT_" 
                   << mach->getNameForRestrictionWithId(-unit);
         break;
      
      default:
         assert (!"Invalid scheduling time category.");
         break;
   }
   return (oss.str());
}

const char* 
TAcomputePercentForKey (unsigned int key)
{
   switch (key & TIME_FAMILY_MASK)
   {
      case MEM_LAT_EXECUTION_TIME:
      case MEM_BDWTH_EXECUTION_TIME:
      case SCHEDULING_TOTAL_TIME:
      case MINIMUM_APPLICATION_TIME:
      case MINIMUM_RESOURCES_TIME:
      case MEMORY_OVERLAP_TIME:
      case MEMORY_STALL_TIME:
      case RETIRED_UOPS:
      case RETIRED_UOPS_TYPE:
      case MEMORY_REFERENCES:
      case CACHE_MISSES:
      case NON_STREAMABLE_ACCESS:
      case STREAM_BIN_COUNT:
      case MEMORY_PENALTY_TOTAL_TIME:
      case MEMORY_BANDWIDTH_TOTAL_TIME:
      case APPLICATION_DEPENDENCY_TIME:
      case LACK_OF_RESOURCES_TOTAL_TIME:
      case SCHEDULING_EXTRA_TOTAL_TIME:
      case MEMORY_PENALTY_TIME_LEVEL:
      case MEMORY_BANDWIDTH_TIME_LEVEL:
      case MISS_COUNT_LEVEL:
      case LACK_OF_RESOURCES_TIME:
      case VECTORIZATION_TIME:
      case RESOURCE_USAGE:
      case SCHEDULING_EXTRA_TIME:
      case LACK_OF_RESOURCES_LOST_TIME:
      case INSTR_PARALLELISM_LOST_TIME:
      case VECTORIZATION_LOST_TIME:
      case MEMORY_FOOTPRINT_LEVEL:
         return "true";
         break;

      case BANDWIDTH_REQUIRED_LEVEL:
      case IMPROVEMENT_FROM_FPGA:
      case FPGA_COMPUTATION_COST:
      case FPGA_TRANSFER_COST:
         return "false";
         break;
      
      default:
         assert (!"Invalid scheduling time category.");
         break;
   }
   return NULL;
}

bool 
TAdisplayValueForKey (unsigned int key, int verbose)
{
   switch (key & TIME_FAMILY_MASK)
   {
      case MEM_LAT_EXECUTION_TIME:
      case MEM_BDWTH_EXECUTION_TIME:
      case SCHEDULING_TOTAL_TIME:
      case MINIMUM_APPLICATION_TIME:
      case MINIMUM_RESOURCES_TIME:
      case MEMORY_OVERLAP_TIME:
      case MEMORY_STALL_TIME:
      case RETIRED_UOPS:
      case RETIRED_UOPS_TYPE:
      case MEMORY_REFERENCES:
      case CACHE_MISSES:
      case NON_STREAMABLE_ACCESS:
      case STREAM_BIN_COUNT:
      case MEMORY_PENALTY_TOTAL_TIME:
      case MEMORY_BANDWIDTH_TOTAL_TIME:
      case APPLICATION_DEPENDENCY_TIME:
      case LACK_OF_RESOURCES_TOTAL_TIME:
      case SCHEDULING_EXTRA_TOTAL_TIME:
      case MEMORY_PENALTY_TIME_LEVEL:
      case MISS_COUNT_LEVEL:
      case LACK_OF_RESOURCES_LOST_TIME:
      case INSTR_PARALLELISM_LOST_TIME:
      case VECTORIZATION_LOST_TIME:
      case MEMORY_BANDWIDTH_TIME_LEVEL:
      case IMPROVEMENT_FROM_FPGA:
      case FPGA_COMPUTATION_COST:
      case FPGA_TRANSFER_COST:
      case MEMORY_FOOTPRINT_LEVEL:
      case RESOURCE_USAGE:
      case LACK_OF_RESOURCES_TIME:
      case VECTORIZATION_TIME:
         return (true);
         break;

      case BANDWIDTH_REQUIRED_LEVEL:
      case SCHEDULING_EXTRA_TIME:
         return (verbose);
         break;
      
      default:
         assert (!"Invalid scheduling time category.");
         break;
   }
   return (false);
}

bool
aggregateValuesForKey (unsigned int key)
{
   switch (key & TIME_FAMILY_MASK)
   {
      case MEM_LAT_EXECUTION_TIME:
      case MEM_BDWTH_EXECUTION_TIME:
      case SCHEDULING_TOTAL_TIME:
      case MINIMUM_APPLICATION_TIME:
      case MINIMUM_RESOURCES_TIME:
      case MEMORY_OVERLAP_TIME:
      case MEMORY_STALL_TIME:
      case RETIRED_UOPS:
      case RETIRED_UOPS_TYPE:
      case MEMORY_REFERENCES:
      case CACHE_MISSES:
      case NON_STREAMABLE_ACCESS:
      case STREAM_BIN_COUNT:
      case MEMORY_PENALTY_TOTAL_TIME:
      case MEMORY_BANDWIDTH_TOTAL_TIME:
      case APPLICATION_DEPENDENCY_TIME:
      case LACK_OF_RESOURCES_TOTAL_TIME:
      case SCHEDULING_EXTRA_TOTAL_TIME:
      case MEMORY_PENALTY_TIME_LEVEL:
      case MISS_COUNT_LEVEL:
      case LACK_OF_RESOURCES_LOST_TIME:
      case INSTR_PARALLELISM_LOST_TIME:
      case VECTORIZATION_LOST_TIME:
      case BANDWIDTH_REQUIRED_LEVEL:
      case MEMORY_BANDWIDTH_TIME_LEVEL:
      case LACK_OF_RESOURCES_TIME:
      case VECTORIZATION_TIME:
      case SCHEDULING_EXTRA_TIME:
      case RESOURCE_USAGE:
         return (true);
         break;

      case IMPROVEMENT_FROM_FPGA:
      case FPGA_COMPUTATION_COST:
      case FPGA_TRANSFER_COST:
      case MEMORY_FOOTPRINT_LEVEL:
         return (false);
         break;
      
      default:
         assert (!"Invalid scheduling time category.");
         break;
   }
   return (false);
}

bool 
TAkeyActsAsThreshold (unsigned int key)
{
   switch (key & TIME_FAMILY_MASK)
   {
      case MEM_LAT_EXECUTION_TIME:
      case MEM_BDWTH_EXECUTION_TIME:
      case SCHEDULING_TOTAL_TIME:
      case MINIMUM_APPLICATION_TIME:
      case MINIMUM_RESOURCES_TIME:
      case MEMORY_OVERLAP_TIME:
      case MEMORY_STALL_TIME:
      case RETIRED_UOPS:
      case MEMORY_REFERENCES:
      case CACHE_MISSES:
      case NON_STREAMABLE_ACCESS:
      case STREAM_BIN_COUNT:
      case MEMORY_PENALTY_TOTAL_TIME:
      case MEMORY_BANDWIDTH_TOTAL_TIME:
      case APPLICATION_DEPENDENCY_TIME:
      case LACK_OF_RESOURCES_TOTAL_TIME:
      case SCHEDULING_EXTRA_TOTAL_TIME:
      case MISS_COUNT_LEVEL:
      case LACK_OF_RESOURCES_LOST_TIME:
      case INSTR_PARALLELISM_LOST_TIME:
      case VECTORIZATION_LOST_TIME:
      case IMPROVEMENT_FROM_FPGA:
      case FPGA_COMPUTATION_COST:
      case FPGA_TRANSFER_COST:
      case RESOURCE_USAGE:
         return (true);
         break;

      case RETIRED_UOPS_TYPE:
      case MEMORY_PENALTY_TIME_LEVEL:
      case MEMORY_BANDWIDTH_TIME_LEVEL:
      case BANDWIDTH_REQUIRED_LEVEL:
      case SCHEDULING_EXTRA_TIME:
      case MEMORY_FOOTPRINT_LEVEL:
      case LACK_OF_RESOURCES_TIME:
      case VECTORIZATION_TIME:
         return (false);
         break;
      
      default:
         assert (!"Invalid scheduling time category.");
         break;
   }
   return (false);
}

const char* 
RetiredUopToString(RetiredUopType rut)
{
   switch(rut)
   {
      case UOP_INVALID:       return ("Invalid");
      case UOP_LOOP_COND:     return ("LoopCond");
      case UOP_ADDRESS_CALC:  return ("AddrGen");
      case UOP_SPILL_UNSPILL: return ("StackTmp");
      case UOP_DATA_MOVE:     return ("RegMove");
      case UOP_FLOATING_UTIL: return ("FpWork");
      case UOP_INTEGER_UTIL:  return ("IntWork");
      case UOP_MEMORY_UTIL:   return ("MemWork");
      case UOP_BRANCH:        return ("Branches");
      case UOP_PREFETCH:      return ("Prefetch");
      case UOP_NOP:           return ("Nop");
      default:     assert(! "Unknown uop type");
   }
   return ("Unknown");
}

}  /* namespace MIAMI */

