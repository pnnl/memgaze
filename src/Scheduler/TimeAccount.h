/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: TimeAccount.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements a simple performance database using an 
 * associative map.
 */


#ifndef _TIME_ACCOUNT_H
#define _TIME_ACCOUNT_H

/* TimeAccount class records where every cpu cycle is spent:
 * - if dependency lower bound is larger, assign all those cycles to 
 *   APPLICATION_DEPENDENCY_TIME
 * - if resource lower bound is larger, assign all those cycles to
 *   LACK_OF_RESOURCES_TIME family, with the subtype = index of restrictive
 *   unit (this includes unit restriction rules)
 * - additional cycle when scheduling unsuccessfull, add cycle to
 *   SCHEDULING_RESOURCES_TIME family, with the subtype = index of unit 
 *   needed that was already allocated (this is less precise because it may
 *   be that none of a set of interchangeable resources are available)
 * - record a minimum application time in MINIMUM_APPLICATION_TIME that
 *   uses the dependency lower bound for all paths (e.g. only 1 cycle/iter 
 *   for loops with no dependency cycle, or should it be the longest path 
 *   from a root to a leaf node for all iterations?)
 * - record a minimum machine constraints time in MINIMUM_RESOURCES_TIME
 *   that uses the resource lower bound for every path (assumes no dependency
 *   between instructions of a path, but paths are executed sequentially).
 *
 *   EVENTUALLY, after I understand how to compute regiter pressure, 
 *   I can add entries due to lack of registers.
 *
 * The database also store information about memory hierarchy metrics (misses,
 * penalty, bandwidth) for each of the levels. I can just invoke with the type
 * MISS_COUNT_LEVEL and subtype = index of memory level, and compute
 * penalty and bandwidth automatically from machine description data.
 * I can add also a MEMORY_PENALTY_TIME for each and all levels, and a
 * TOTAL_EXECUTION_TIME which includes memory penalty and scheduling time.
 *
 */


# include <map>
# include <string>
# include "Machine.h"

namespace MIAMI
{

typedef enum {
    UOP_INVALID = 0,
    UOP_LOOP_COND,
    UOP_ADDRESS_CALC, 
    UOP_SPILL_UNSPILL,
    UOP_DATA_MOVE,
    UOP_FLOATING_UTIL,
    UOP_INTEGER_UTIL,
    UOP_MEMORY_UTIL,
    UOP_BRANCH,
    UOP_PREFETCH,
    UOP_NOP,
    UOP_NUM_TYPES
} RetiredUopType;

const char* RetiredUopToString(RetiredUopType rut);

# define DEFAULT_UNIT   0x007ffff0

typedef std::map<unsigned int, double> UiToDoubleMap;

class TimeAccount // : public UiToDoubleMap
{
public:
   TimeAccount () {} // : UiToDoubleMap () {}
   ~TimeAccount () { data.clear (); }
   TimeAccount (const TimeAccount &ta);

   TimeAccount& operator = (const TimeAccount &ta);
   
   TimeAccount operator + (const TimeAccount& ta) const;
   TimeAccount& operator += (const TimeAccount& ta);
   TimeAccount operator - (const TimeAccount& ta) const;
   TimeAccount& operator -= (const TimeAccount& ta);
   
   TimeAccount operator * (double factor) const;
   TimeAccount& operator *= (double factor);
   TimeAccount operator / (double factor) const;
   TimeAccount& operator /= (double factor);

   void addExecutionTime (double cycles);
   void addResourceUsage (int unit, double cycles);

   void addDependencyTime (double cycles);
   void addResourcesTime (int unit, double cycles);
   void addSchedulingTime (int unit, double cycles, int verbose);
   void addApplicationMinimumTime (double cycles);
   void addResourcesMinimumTime (double cycles);
   void addIdealVectorizationTime (int unit, double cycles);

   void addMemoryOverlapTime (double cycles);
   void addMemoryStallTime (double cycles);
   void addRetiredUops (int type, double count);
   
   void addMemReferenceCount(double count);
   void addCacheMissesCount(double count);
   void addNoStreamCount(double count);
   void addStreamBinCount(int bin, double count);

   void addFpgaComputeTime (double cycles);
   void addFpgaTransferTime (double cycles);
   void addNonFpgaComputeTime (double cycles);

   void addMemFootprintLevel (int level, double footprint);

   void computeBandwidthTime (const Machine *tmach);
   void addMissCountLevel (int level, double misses, const Machine *mach);
   void addMissCountLevel (int level, double misses, const MemoryHierarchyLevel *mhl);
   void addTotalMissCountLevel (int level, double misses);
   void addFragMissCountLevel (int level, double misses);
   void addIrregMissCountLevel (int level, double misses);

   void addReuseMissCountLevel (int level, double misses, const Machine *mach);
   void addReuseMissCountLevel (int level, double misses, const MemoryHierarchyLevel *mhl);
   void addReuseSchedulingTime (double cycles);
   double getTotalCpuTime ();
   double getNoDependencyTime ();
   
   double getDisplayValue (unsigned int key, const Machine *mach);
   
   const UiToDoubleMap& getDataConst() const { return (data); }
   UiToDoubleMap& getData() { return (data); }
   
private:
   UiToDoubleMap data;

};

extern std::string getTANameForKey(unsigned int key, const Machine *mach);
extern std::string getTAShortNameForKey(unsigned int key, const Machine *mach);
extern const char* TAcomputePercentForKey (unsigned int key);
extern bool TAdisplayValueForKey (unsigned int key, int verbose = 0);
extern bool TAkeyActsAsThreshold (unsigned int key);

}  /* namespace MIAMI */

#endif
