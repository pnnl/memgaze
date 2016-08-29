/*
 * This file is part of the MIAMI framework. For copyright information, see    
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: load_module.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data bookkeeping at the load module level for a data reuse analysis tool.
 */

#ifndef MIAMI_MRDTOOL_LOAD_MODULE_H
#define MIAMI_MRDTOOL_LOAD_MODULE_H

// standard header
#include "private_load_module.h"
#include "bucket_hashmap.h"

#include "data_reuse.h"
#include <list>
#include <vector>
#include <map>

namespace MIAMI
{

/* Must associate ScopeEventRecords with edges and BBL end addresses.
 * Some edged can be assigned multiple events.
 */
typedef std::list<MIAMI_MEM_REUSE::ScopeEventRecord> ListOfEvents;
extern ListOfEvents* defEvListP;
// I need to use an STL map (balanced tree) to map scope events to instruction addresses
// I must search using the STL equal_range method for the range on a PIN BBL.
typedef std::map <addrtype, ListOfEvents*> ScopeEventsMap;
typedef MIAMIU::BucketHashMap <addrtype, ListOfEvents*, &defEvListP, 128> ScopeEventsHash;

class Routine;
extern MIAMI::Routine* defRoutineP;
extern int32_t defInt32;
typedef std::map<addrtype, Routine*> RoutineMap;
typedef MIAMIU::BucketHashMap <addrtype, MIAMI::Routine*, &defRoutineP, 64> RoutineHashMap;
typedef MIAMIU::BucketHashMap <uint64_t, int32_t, &defInt32, 128> BucketMapInt;

struct IntPair
{
   IntPair(int32_t _index=0, int32_t _scope=0) : index(_index), scope(_scope)
   { }
   
   int32_t index, scope;
};
extern struct IntPair defIntPair;
typedef MIAMIU::BucketHashMap <uint64_t, struct IntPair, &defIntPair, 128> BucketMapIntPair;

class LoadModule : public PrivateLoadModule
{
public:
   typedef std::vector<char> CharVector;
   typedef std::vector<int>  IntVector;
   
   LoadModule (uint32_t _id, addrtype _load_offset, addrtype _low_addr , std::string _name)
       : PrivateLoadModule(_id, _load_offset, _low_addr, _name)
   {
      nextInstIndex = 1;
      nextScopeIndex = 1;
      reverseInstIndex = 0;
      reverseScopeIndex = 0;
      savedInstIndex = savedScopeIndex = 0;
      
      scopeIsInner.resize(64);
      scopeParent.resize(64);
   }
   virtual ~LoadModule();
   
   void saveToFile(FILE *fd) const;
   void dumpStats(FILE *fd, std::string& rname) const;

   // The next four methods use and modify shared data. 
   // However, I invoke them only from PIN instrumentation callbacks
   // that are serialized by PIN, even when the application is multi-
   // threaded. Thus, they are safe within this use model, but be aware
   // of potential data races if they are invoked from an analysis step.
   int32_t getInstIndex(addrtype iaddr, int memop);
   IntPair& getInstIndexAndScope(addrtype iaddr, int memop);
   void setScopeIdForReference(addrtype iaddr, int memop, int32_t scopeId);
   
   int32_t getScopeIndex(addrtype saddr, int level);
   addrtype getAddressForInstIndex(int32_t iindex, int32_t& memop);
   addrtype getAddressForScopeIndex(int32_t sindex, int32_t& level);
   
   // check scope ancestry
   inline bool ScopeIsAncestor(int parent, int child)
   {
      if (!parent) return (false);
      while (child>parent)
         child = scopeParent[child];
      return (child==parent);
   }

   inline void SetParentForScope(int child, int parent)
   {
      long sz = scopeParent.size();
      if (sz <= child)
      {
         do {
            sz <<= 1;
         } while (sz <= child);
         scopeParent.resize(sz);
      }
      scopeParent[child] = parent;
   }

   inline Routine*& GetRoutineAt(addrtype pc) { return (funcMap[pc]); }
   
   Routine* HasRoutineAtPC(addrtype pc) const;
   void  AddRoutine(Routine *r);
   
   BucketMapIntPair instMapper;
   BucketMapInt scopeMapper;
   
   // I will just make these maps public for now. I would need methods to check
   // if there are events at a given address in each of the maps, methods to get
   // the events, or to add events. Easier to work with the map directly.
   ScopeEventsMap edgeEvents;
   ScopeEventsHash bblEvents;
   
   // find if a scope is an inner loop or not; inner loop is strict in the sense 
   // that it should not contain function calls either
   CharVector scopeIsInner;
   
private:
   RoutineMap     miamiRoutines;  // store routines per image
   RoutineHashMap funcMap;  // store routines per image
   int32_t nextInstIndex, nextScopeIndex;
   
   uint64_t *reverseInstIndex;
   uint64_t *reverseScopeIndex;
   int32_t savedInstIndex, savedScopeIndex;
   
   // Store parent information for scopes.
   // We know that an inner loop has a higher index than its enclosing scope.
   // Routines have parent set to 0.
   IntVector scopeParent;
};

}

#endif
