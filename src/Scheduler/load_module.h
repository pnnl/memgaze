/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: load_module.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements data bookkeeping and analysis associated with an individual 
 * load module.
 */

#ifndef MIAMI_SCHEDULER_LOAD_MODULE_H
#define MIAMI_SCHEDULER_LOAD_MODULE_H

// standard header
#include <list>
#include <vector>



#include "miami_types.h"
#include "private_load_module.h"
#include "prog_scope.h"
#include "miami_options.h"
#include "slice_references.h"
#include "bucket_hashmap.h"
#include "scope_implementation.h"
#include "hashmaps.h"
#include "dense_container.h"

#include <BPatch.h> // must be included after miami headers to compile

namespace MIAMI
{
// Forward declaration
class Routine;
typedef std::list<Routine*> RoutineList;

typedef std::vector<int32_t> IntVector;
typedef MIAMIU::DenseContainer<IntVector> IntVectorContainer;

extern int32_t defInt32;
typedef MIAMIU::BucketHashMap <uint64_t, int32_t, &defInt32, 128> BucketMapInt;
typedef MIAMIU::BucketHashMap <int32_t, int32_t, &defInt32, 128> HashIntMap;

typedef std::map<addrtype,double> InstLatMap;
//ozgurS


struct memStruct {
   int level;
   int hitCount;
   double latency;
};

typedef std::map<int,memStruct> InstlvlMap;
typedef std::map< addrtype, std::map<int,memStruct> > InstMemMap;
//ozgurE

class LoadModule : public PrivateLoadModule
{
   typedef std::vector<int32_t>  Int32Vector;
   typedef std::vector<ScopeImplementation*>  SIVector;
   
public:
   LoadModule (uint32_t _id, addrtype _load_offset, addrtype _low_addr_offset, std::string _name,
            uint32_t _hash)
       : PrivateLoadModule(_id, _load_offset, _low_addr_offset, _name),
            hash(_hash)
   {
      reloc_offset = 0; 
      nextInstIndex = nextScopeIndex = nextSetIndex = 1;
      img_scope = 0;
      undefScope = 0;
      dyn_image = 0;
   }

   virtual ~LoadModule();
   
   int loadFromFile(FILE *fd, bool parse_routines=false);

   int analyzeRoutines(FILE *fd, ProgScope *prog, const MiamiOptions *mo);
   
   GroupScope* GetDefaultScope() {
      if (! undefScope)
      {
         // if the default scope is not created, create it under the unknown-file??
         undefScope = new GroupScope(img_scope, "UndefinedScope", 0);
      }
      return (undefScope);
   }
   
   addrtype LowAddressOffsetChange() const 
   {
      return (low_addr_offset - old_low_offset);
   }

   virtual addrtype RelocationOffset() const    { return (reloc_offset); }
   addrtype CfgBaseAddress() const    { return (cfg_base_addr); }
   
   int32_t GetScopeIndexForReference(int32_t iidx) const
   {
      assert(iidx>=0 && iidx<(int)refParent.size());
      return (refParent[iidx]);
   }

   void    SetScopeIndexForReference(int32_t iidx, int32_t sidx);

   int32_t AllocateIndexForInstPC(addrtype iaddr, int memop);
   int32_t AllocateIndexForScopePC(addrtype saddr, int level);
   int32_t AllocateIndexForInstOffset(addrtype iaddr, int memop);
   int32_t AllocateIndexForScopeOffset(addrtype saddr, int level);
   int32_t GetIndexForInstPC(addrtype iaddr, int memop);  // return neg value if not a member
   int32_t GetIndexForScopePC(addrtype saddr, int level); // return neg value if not a member
   int32_t GetIndexForInstOffset(addrtype iaddr, int memop);  // return neg value if not a member
   int32_t GetIndexForScopeOffset(addrtype saddr, int level); // return neg value if not a member
   
   void SetSIForScope(int child, ScopeImplementation *pscope);
   inline ScopeImplementation* GetSIForScope(int child) {
      if (child<0 || child>=(int)scopeSIPs.size())
         return (0);
      else
         return (scopeSIPs[child]);
   }
   
   int32_t GetSetIndexForReference(int32_t iidx);
   int32_t AddReferenceToSet(int32_t iidx, int32_t setId=0);
   void SetNameForSetIndex(int32_t setId, const char* name);
   const char* GetNameForSetIndex(int32_t setId);
   const char* GetNameForReference(int32_t iidx);
   
   void SetIrregularAccessForSet(int32_t setId, int loop);
   bool SetHasIrregularAccessAtDistance(int32_t setId, int dist);


   void createDyninstImage(BPatch& bpatch);
   int dyninstAnalyzeRoutine(std::string name, ProgScope *prog, const MiamiOptions *mo);
   BPatch_image* getDyninstImage()
   {
      return dyn_image;
   }
//ozgurS
   double calculateMissRatio(InstlvlMap lvlMap, int lvl);
   InstlvlMap getMemLoadData(addrtype insn);
//ozgurE   
   double getMemLoadLatency(addrtype insn);
private:   
   int32_t nextInstIndex, nextScopeIndex, nextSetIndex;
   Int32Vector refParent;
   SIVector scopeSIPs;  // Scope Implementation Pointers (SIPs)
   BucketMapInt instMapper;
   BucketMapInt scopeMapper;
   
   int loadRoutineData(FILE *fd);
   Routine* loadOneRoutine(FILE *fd, uint32_t r);
   
   addrtype cfg_base_addr;
   addrtype reloc_offset;
   addrtype old_low_offset;
   RoutineList    funcList; // alternative storage, list of routines
   uint32_t hash;

   // Most of the analysis' results should be stored separately for each
   // image, because shared library may be loaded and unloaded, with multiple
   // images spanning the same PCs at different points in time.
   // Thus, binary addresses only make sense as offsets inside a particular
   // image.
   RefNameMap refNames;  // map set indices to variable names
   // I need some data structures to map instruction indices to sets
   HashIntMap ref2Set;   // store the set index for each ref index
   // Do I need to keep track of which refs are part of each set?
   // I can use a DenseContainer type, where the key is the set index and
   // the value is a set of ref indecies. The value can be a set, vector or list.
   IntVectorContainer set2Refs;
   
   // Store a unique copy of all the names associated with this image
   CharPSet nameStorage;
   
   // Store information about which ref sets have irregular access pattern
   BucketMapInt irregSetInfo;
   
   // Define a default ScopeImplementation object for unidentified scopes
   GroupScope *undefScope;
   ImageScope *img_scope;

   
   BPatch_image* dyn_image;
   InstLatMap instLats;
   
   InstMemMap instMemMap;
};

}

#endif
