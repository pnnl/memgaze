/*
 * This file is part of the MIAMI framework. For copyright information, see    
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: load_module.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data bookkeeping at the load module level for a CFG profiler tool.
 */

#ifndef MIAMI_CFGTOOL_LOAD_MODULE_H
#define MIAMI_CFGTOOL_LOAD_MODULE_H

#include "miami_types.h"
#include "private_load_module.h"
#include "cfg_data.h"
#include "CFG.h"

#include "bucket_hashmap.h"
#include <map>

namespace MIAMI
{

class Routine;
extern MIAMI::Routine* defRoutineP;
extern int32_t defInt32Val;
extern addrtype defAddrVal;
typedef std::map<addrtype, Routine*> RoutineMap;
typedef MIAMIU::BucketHashMap <addrtype, MIAMI::Routine*, &defRoutineP> RoutineHashMap;
typedef MIAMIU::BucketHashMap<addrtype, int32_t, &defInt32Val> AddrIndexMap;
typedef MIAMIU::BucketHashMap<int32_t, addrtype, &defAddrVal> IndexAddrMap;
// We use the next map to cache the edge index for a particular instruction 
// index / target address combination.
// Use a guaranteed 64-bit key, regardless of the addrtype size
typedef MIAMIU::BucketHashMap<uint64_t, int32_t, &defInt32Val> Int64IndexMap;

class LoadModule : public PrivateLoadModule
{
public:
   LoadModule (uint32_t _id, addrtype _load_offset, addrtype _low_addr , std::string _name)
       : PrivateLoadModule(_id, _load_offset, _low_addr, _name)
   {
      nextPcIndex = 1;
   }
   virtual ~LoadModule();
   
   void SaveCFGsToFile(FILE *fd, const MIAMI_CFG::count_type *thr_counters, 
                 MIAMI_CFG::MiamiCfgStats &stats);
   void dumpRoutineCFG(const std::string& rname, const MIAMI_CFG::count_type *thr_counters,
                const char* suffix);
   
   int32_t GetRoutineEntryIndex(addrtype pc);
   int32_t GetTraceStartIndex(addrtype pc);
   int32_t GetCFGEdgeIndex(CFG::Edge *e);

   int32_t HasRoutineEntryIndex(addrtype pc);
   int32_t HasTraceStartIndex(addrtype pc);
   int32_t HasCFGEdgeIndex(CFG::Edge *e);
   
   // the next two routines use a separate indexing scheme
   // I must be able to do the mapping in both directions, 
   // so store two separate maps for performance.
   // Only the first method adds new elements. The second method
   // could be declared const, but our bucket hashmap does not have
   // a const method to query by a key.
   int32_t GetIndexForPC(addrtype pc);
   addrtype GetPCForIndex(int32_t index);
   
   inline static int32_t GetHighWaterMarkIndex()   { return (nextCounterId); }
   
   inline Routine*& GetRoutineAt(addrtype pc) { return (funcMap[pc]); }

   inline Routine*& RoutineAtIndirectPc(addrtype pc) { return (indPcRoutines[pc]); }
   
   Routine* HasRoutineAtPC(addrtype pc) const;
   void  AddRoutine(Routine *r);
   
   inline int32_t& getEdgeIndexForInstAndTarget(int32_t iidx, addrtype target)
   {
      addrtype toffset = target - base_addr - low_addr_offset;
      uint64_t key = (((uint64_t)iidx)<<32) + toffset;
      return (indIdxCache[key]);
   }

private:
   static int32_t nextCounterId;  // use a unified counter across all LoadModules
   int32_t nextPcIndex;  // use a separate counter per LoadModule
   
   // store routines per image; find the routine the includes a particular address
   RoutineMap     miamiRoutines; 
   // I need to get the routine that includes a particular indirect branch.
   // For this, I use the previous map. However, some libraries do not include all
   // the symbols. In such cases, PIN uses the name of the last symbol with addr < pc.
   // When the indirect branch end pc-1 is outside the routine range, store the 
   // routine pointer in the next map.
   RoutineHashMap indPcRoutines; 
   // store routines per image; map routine pointer to routine starting address
   RoutineHashMap funcMap;  
   AddrIndexMap edgesMapper, entriesMapper, traceMapper;
   
   AddrIndexMap pc2indexMap;
   IndexAddrMap index2pcMap;
   Int64IndexMap indIdxCache;
};

} /* namespace MIAMI */

#endif
