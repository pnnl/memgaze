/*
 * This file is part of the MIAMI framework. For copyright information, see    
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: load_module.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data bookkeeping at the load module level for a CFG profiler tool.
 */

#include <stdio.h>
#include <map>
#include "load_module.h"
#include "routine.h"
#include "CFG.h"

using namespace MIAMI;
using namespace std;

namespace MIAMI
{
   Routine* defRoutineP = 0;
   addrtype defAddrVal = 0;
   int32_t defInt32Val = -1;
   int32_t LoadModule::nextCounterId = 1;
}

static unsigned int objsInVec = 0;
static int
placeRoutinesInArray (void* arg0, addrtype key, void* value)
{
   MIAMI::Routine **routArray = (MIAMI::Routine**)arg0;
   MIAMI::Routine* r = *((MIAMI::Routine**)value);
   routArray[objsInVec++] = r;
   return (0);
}

LoadModule::~LoadModule()
{
   unsigned int numRoutines = funcMap.size();
   MIAMI::Routine **routArray = new MIAMI::Routine*[numRoutines];
   objsInVec = 0;
   funcMap.map (placeRoutinesInArray, routArray);
   
   // deallocate the info stored for each image
   for (unsigned int r=0 ; r<numRoutines ; ++r)
      delete routArray[r];
   delete[] routArray;
   funcMap.clear();
}

void 
LoadModule::SaveCFGsToFile(FILE *fd, const MIAMI_CFG::count_type *thr_counters, 
           MIAMI_CFG::MiamiCfgStats &stats)
{
   // First call the base class method to save basic info
   PrivateLoadModule::saveToFile(fd);
   
   // then save custom information; save the routines
   uint32_t numRoutines = (uint32_t)funcMap.size();
   fwrite(&numRoutines, 4, 1, fd);
   
   MIAMI::Routine **routArray = new MIAMI::Routine*[numRoutines];
   objsInVec = 0;
   funcMap.map (placeRoutinesInArray, routArray);
   for (unsigned int r=0 ; r<numRoutines ; ++r)
   {
      if (fd)
         routArray[r]->saveToFile(fd, base_addr, this, thr_counters, stats);
   }
   delete[] routArray;
}

void
LoadModule::dumpRoutineCFG(const string &rname, const MIAMI_CFG::count_type *thr_counters, 
              const char* suffix)
{
   unsigned int numRoutines = funcMap.size();
   MIAMI::Routine **routArray = new MIAMI::Routine*[numRoutines];
   objsInVec = 0;
   funcMap.map (placeRoutinesInArray, routArray);
   for (unsigned int r=0 ; r<numRoutines ; ++r)
   {
      int genCFG = routArray[r]->Name().compare(rname);
      if (!genCFG)
      {
         routArray[r]->SetEntryAndTraceCounts(this, thr_counters);
         routArray[r]->CfgToDot(suffix);
      }
   }
   delete[] routArray;
}

int32_t 
LoadModule::GetRoutineEntryIndex(addrtype pc)
{
   addrtype key = pc - base_addr;
   int32_t &index = entriesMapper[key];
   if (index < 0)
   {
      index = nextCounterId;
      nextCounterId += 1;
   }  // new entry index needed
   return (index);
}

int32_t 
LoadModule::GetTraceStartIndex(addrtype pc)
{
   addrtype key = pc - base_addr;
   int32_t &index = traceMapper[key];
   if (index < 0)
   {
      index = nextCounterId;
      nextCounterId += 1;
   }  // new trace start index needed
   return (index);
}

int32_t 
LoadModule::GetCFGEdgeIndex(CFG::Edge *e)
{
   addrtype key = (addrtype)e;
   int32_t &index = edgesMapper[key];
   if (index < 0)
   {
      index = nextCounterId;
      nextCounterId += 1;
   }  // new edge index needed
   return (index);
}

int32_t 
LoadModule::GetIndexForPC(addrtype pc)
{
   addrtype key = pc - base_addr;
   int32_t &index = pc2indexMap[key];
   if (index < 0)
   {
      index = nextPcIndex;
      nextPcIndex += 1;
      // add an entry to the reverse map as well
      index2pcMap[index] = key;
   }  // new pc index needed
   return (index);
}

addrtype 
LoadModule::GetPCForIndex(int32_t index)
{
   // check the index2pc map for the given index
   unsigned int pre_hash = index2pcMap.pre_hash(index);
   if (index2pcMap.is_member(index, pre_hash))
      return (index2pcMap(index, pre_hash) + base_addr);
   else
      return (0);
}

int32_t 
LoadModule::HasRoutineEntryIndex(addrtype pc)
{
   addrtype key = pc - base_addr;
   unsigned int pre_hash = entriesMapper.pre_hash(key);
   if (entriesMapper.is_member(key, pre_hash))
      return (entriesMapper(key, pre_hash));
   else
      return (0);
}

int32_t 
LoadModule::HasTraceStartIndex(addrtype pc)
{
   addrtype key = pc - base_addr;
   unsigned int pre_hash = traceMapper.pre_hash(key);
   if (traceMapper.is_member(key, pre_hash))
      return (traceMapper(key, pre_hash));
   else
      return (0);
}

int32_t 
LoadModule::HasCFGEdgeIndex(CFG::Edge *e)
{
   addrtype key = (addrtype)e;
   unsigned int pre_hash = edgesMapper.pre_hash(key);
   if (edgesMapper.is_member(key, pre_hash))
      return (edgesMapper(key, pre_hash));
   else
      return (0);
}

Routine* 
LoadModule::HasRoutineAtPC(addrtype pc) const 
{
   RoutineMap::const_iterator rit = miamiRoutines.upper_bound(pc);
   if (rit!=miamiRoutines.end() && rit->second->Start()<=pc)
      return (rit->second);
   else
      return (0);
}

void 
LoadModule::AddRoutine(Routine *r) 
{
   miamiRoutines.insert(RoutineMap::value_type(r->End(), r));
}
