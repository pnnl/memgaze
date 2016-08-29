/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: load_module.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data bookkeeping at the load module level for a data reuse analysis tool.
 */

#include <stdio.h>
#include "load_module.h"
#include "routine.h"

using namespace MIAMI;
using namespace std;

extern int MIAMI_MEM_REUSE::verbose_level;

namespace MIAMI
{
   Routine* defRoutineP = 0;
   int32_t defInt32 = 0;
   struct IntPair defIntPair(0,0);
   ListOfEvents* defEvListP = 0;
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

static int
placeKeysInArray (void* arg0, uint64_t key, void* value)
{
   uint64_t *keyArray = static_cast<uint64_t*>(arg0);
   int32_t *v = static_cast<int32_t*>(value);
   keyArray[*v] = key;
   return (0);
}

static int
placePairKeysInArray (void* arg0, uint64_t key, void* value)
{
   uint64_t *keyArray = static_cast<uint64_t*>(arg0);
   IntPair *v = static_cast<IntPair*>(value);
   if (v->index > 0)
      keyArray[v->index] = key;
   return (0);
}

LoadModule::~LoadModule()
{
   if (MIAMI_MEM_REUSE::verbose_level>0)
   {
      fprintf(stderr, "Unloading image %s\n", img_name.c_str());
      instMapper.print_stats(stderr);
   }
   
   unsigned int numRoutines = funcMap.size();
   MIAMI::Routine **routArray = new MIAMI::Routine*[numRoutines];
   objsInVec = 0;
   funcMap.map (placeRoutinesInArray, routArray);
   
   // deallocate the info stored for each image
   for (unsigned int r=0 ; r<numRoutines ; ++r)
      delete routArray[r];
   delete[] routArray;
   funcMap.clear();
   
   if (reverseInstIndex)
      delete[] reverseInstIndex;
   if (reverseScopeIndex)
      delete[] reverseScopeIndex;
}

addrtype 
LoadModule::getAddressForInstIndex(int32_t iindex, int32_t& memop)
{
   if (reverseInstIndex && savedInstIndex!=nextInstIndex)
   {
      delete[] reverseInstIndex;
      reverseInstIndex = 0;
   }
   if (! reverseInstIndex)
   {
      reverseInstIndex = new uint64_t[nextInstIndex];
      savedInstIndex = nextInstIndex;
      instMapper.map(placePairKeysInArray, reverseInstIndex);
   }
   memop = reverseInstIndex[iindex] & 0x3;
   // return the offset into the image for now
   return ((addrtype)(reverseInstIndex[iindex]>>2)); // + base_addr + low_addr_offset);
}

addrtype 
LoadModule::getAddressForScopeIndex(int32_t sindex, int32_t& level)
{
   if (reverseScopeIndex && savedScopeIndex!=nextScopeIndex)
   {
      delete[] reverseScopeIndex;
      reverseScopeIndex = 0;
   }
   if (! reverseScopeIndex)
   {
      reverseScopeIndex = new uint64_t[nextScopeIndex];
      savedScopeIndex = nextScopeIndex;
      scopeMapper.map(placeKeysInArray, reverseScopeIndex);
   }
   level = reverseScopeIndex[sindex] & 0xffff;
   // return the offset into the image for now
   return ((addrtype)(reverseScopeIndex[sindex]>>16)); // + base_addr + low_addr_offset);
}

int32_t 
LoadModule::getInstIndex(addrtype iaddr, int memop)
{
   uint64_t key = ((iaddr-base_addr-low_addr_offset)<<2) + memop;
//   int32_t &index = instMapper [key];
   IntPair &ipair = instMapper [key];
   if (ipair.index == 0)
   {
      ipair.index = nextInstIndex;
      nextInstIndex += 1;
   }  // new instruction index needed
   return (ipair.index);
}

IntPair& 
LoadModule::getInstIndexAndScope(addrtype iaddr, int memop)
{
   uint64_t key = ((iaddr-base_addr-low_addr_offset)<<2) + memop;
   IntPair &ipair = instMapper [key];
   if (ipair.index == 0)
   {
      ipair.index = nextInstIndex;
      nextInstIndex += 1;
   }  // new instruction index needed
   return (ipair);
}

void 
LoadModule::setScopeIdForReference(addrtype iaddr, int memop, int32_t scopeId)
{
   uint64_t key = ((iaddr-base_addr-low_addr_offset)<<2) + memop;
   IntPair &ipair = instMapper [key];
   if (ipair.scope && ipair.scope!=scopeId)
   {
#if DEBUG_SCOPE_DETECTION
      if (MIAMI_MEM_REUSE::verbose_level > 1)
         fprintf(stderr, "Img %d, reference at 0x%" PRIxaddr ", %d with key 0x%" PRIx64 " already has scope %d, new scopeId=%d. Update\n",
              img_id, iaddr, memop, key, ipair.scope, scopeId);
#endif
   }
   ipair.scope = scopeId;
}

int32_t 
LoadModule::getScopeIndex(addrtype saddr, int level)
{
   assert(level < (1<<16));
   if (saddr < (base_addr+low_addr_offset))
   {
      fprintf(stderr, "Error: Img %d, received scope addr 0x%" PRIxaddr " less than base_addr 0x%"
                  PRIxaddr " + low_addr_offset 0x%" PRIxaddr " = 0x%" PRIxaddr "\n",
            img_id, saddr, base_addr, low_addr_offset, base_addr+low_addr_offset);
   }
   uint64_t key = ((saddr-base_addr-low_addr_offset)<<16) + level;
   int32_t &index = scopeMapper [key];
   if (index == 0)
   {
#if DEBUG_SCOPE_DETECTION
      if (MIAMI_MEM_REUSE::verbose_level > 1)
         fprintf(stderr, "Image %d, addr=0x%" PRIxaddr ", level=%d --> get ScopeId=%d\n",
            img_id, saddr, level, nextScopeIndex);
#endif
      index = nextScopeIndex;
      nextScopeIndex += 1;
   }  // new scope index needed
   return (index);
}

void 
LoadModule::saveToFile(FILE *fd) const
{
   // save the image basic info only
   PrivateLoadModule::saveToFile(fd);
}

void
LoadModule::dumpStats(FILE *fd, string &rname) const
{
   unsigned int numRoutines = funcMap.size();
   if (fd)
      fprintf(fd, "%u) Image %s at 0x%p has %u routines\n======================\n", 
             img_id, img_name.c_str(), (void*)(base_addr), numRoutines);
   MIAMI::Routine **routArray = new MIAMI::Routine*[numRoutines];
   objsInVec = 0;
   funcMap.map (placeRoutinesInArray, routArray);
   for (unsigned int r=0 ; r<numRoutines ; ++r)
   {
      int genCFG = routArray[r]->Name().compare(rname);
      if (!genCFG)
         routArray[r]->CfgToDot();
   }
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
