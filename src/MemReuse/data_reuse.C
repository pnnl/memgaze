/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: data_reuse.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements thread level data management and the tool specific logic for a 
 * data reuse analysis tool.
 */

#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <vector>
#include <algorithm>

#include "data_reuse.h"
#include "miami_allocator.h"
#include "PrivateCFG.h"

// uncomment the next define to have a simpler version of the library
// for debugging
//#define DEBUG_CARRY_LIB

#define DWORD_ALIGNED
#ifndef DEBUG_CARRY_LIB
#define SPLAY_TREE_HAS_KEY_IN_NODE
#endif

/* I need a data structure with O(1) access time that holds information
 * about every memory block that has been accessed.
 * Previously, I've used either a hashtable, or a 3-level page table structure.
 * With a 64-bit address space, I will likely need more than 3 levels. I am
 * worried that all these indirections will actually slow down access time.
 *
 * Idea: Use a two level structure: First level is a hashtable. Each table entry
 * maps to an array of entries that can cover a chunk of memory (128KB to 1MB).
 *
 * Problem: If I use address space sampling, the sampling is done on the low 
 * bits of the memory block index. Thus, my arrays on the second level would be
 * very sparse. I end up not saving any memory for this data structure through
 * sampling. Can I compress the indices by looking at the mask? I need it to
 * be fast ...
 *
 * Solution: We use fixed length (let's say 64 entries) arrays on the 2nd level.
 * We count the number of consecutive 1 bits in the lowest bits of the mask,
 * and add that count to the right shift value for an address. We then use the
 * next 6 bits to index into the 2nd level arrays. I need to use my custom 
 * allocator for this data structure.
 *
 * Also, make the bucket hashtable to auto-grow. Let's say, when the average
 * occupancy reaches 8, I quadruple the number of entries.
 */

#include "generic_pair.h"
#include "block_mapper.h"
#include "mrd_splay_tree.h"
#include "dense_container.h"
#include "self_sorted_list.h"
#include "miami_stack.h"
#include "miami_growvector.h"
#include "fast_hashmap.h"
#include "load_module.h"
#include "miami_data_sections.h"

namespace MIAMI_MEM_REUSE
{
   #define ABSOLUTE_BIN_LIMIT  0
   #define RELATIVE_BIN_LIMIT  0.03f

   /* Define structures to store the reuse information at reuse pattern and scope level
    */
   typedef uint32_t dist_t;  // distance is 32-bit, cannot have more than 2^32 mem blocks
   typedef uint64_t count_t; // count is 64-bit because we can touch the same location many times
   #define PRIdist  PRIu32
   #define PRIcount PRIu64

   count_t defCountType = 0;
   typedef MIAMIU::HashMap<dist_t, count_t, &defCountType, 8> HashMapCT;
   typedef MIAMIU::GenericPair<dist_t, count_t> DistCountPair;
   typedef std::vector<DistCountPair> DCVector;
   typedef std::vector<float> FloatArray;
   typedef std::vector<count_t> CountArray;

   // I want to understand how many scope errors I see at run-time (stack in 
   // unexpected state), what type of errors, and where they occur.
   // I will use a bucket hashtable to collect a distribution of errors
   // inside each image. The key is a 64-bit integer that encodes 4 fields:
   // 8-bits for error code, 8-bits for event code, 16-bits image Id, and
   // 32-bits scope ID.
   typedef enum {
       STACK_ERROR_NO_ERROR = 0, 
       STACK_ERROR_SCOPE_NOT_IN_STACK,
       STACK_ERROR_SCOPE_NOT_FOUND,
       STACK_ERROR_SCOPE_NOT_ON_TOP
   } StackErrorType;
   
   typedef MIAMIU::BucketHashMap<uint64_t, count_t, &defCountType, 64> ScopeErrorsMap;
   
   #define MAKE_STACK_ERROR_KEY(img,scope,event,error)  \
             (((((uint64_t)(img)<<32)+(scope))<<16) | (((uint64_t)(event)<<8)+(error)))

   // for each set I will collect separate reuse histograms based on the set 
   // they have reuse on. Collect cold misses separately. 
   // I will use a fast growing array for this. I do not need to delete elements
   // Search is in linear time, but it should be faster than a map for such small
   // arrays; Extend this to differentiate not only on the source set of the 
   // reuse, but also on the loop carrying the reuse.
   // 09/16/2010 gmarin: I need to differentiate also by the iteration distance of
   // the carrying scope
   //
   // 05/29/2013 gmarin: I have four fields to differentiate between different 
   // reuse patterns: source image ID, source instruction (set) index, carry 
   // image ID, and carry scope Idx. Testing equality for all four values is 
   // slow. I will use twp 64-bit ints to store all four fields, 32 bits for 
   // each. Define macros for accessing fields.
   //#define MAKE_PATTERN_KEY(img, inst, carry)  ((((uint64_t)(img))<<48) | (((uint64_t)(inst))<<24) | (uint64_t)(carry))
   #define MASK_32BIT  0xffffffffLL
   #define MAKE_PATTERN_KEY(img, idx)  (((img)<<32) | (idx))
   #define GET_PATTERN_IMG(p)     (((p)>>32) & (MASK_32BIT))
   #define GET_PATTERN_INDEX(p)   ((p) & (MASK_32BIT))


   /**** Declare some global variables ***/
   MIAMI::LoadModule **allImgs;
   uint32_t maxImgs = 0;
   uint32_t loadedImgs = 0;

   ScopeErrorsMap g_scopeErrors;
   count_t g_eventCount[MEMREUSE_EVTYPE_LAST];  // this is initialized to 0 by the compiler
   count_t g_libraryCalls = 0;  // how many times the event processing routine was called
   count_t g_badScopeCount = 0;
   dist_t g_memBlocksAccessed = 0;
   uint32_t g_numDistinctScopes = 0;
   uint32_t g_numDistinctInstructions = 0;
   
   addrtype addr_mask = 0;
   addrtype addr_value = 0;
   uint32_t addr_shift = 0;
   
   bool profileErrors = true;
   int  verbose_level = 0;
   bool compute_footprint = false;
   bool text_mode_output = false;

   class ArrayCT
   {
   public:
      ArrayCT() {
         filled = 0;
      }
      inline int addDistance(dist_t newDist, count_t cnt) {
         // first check if we already saw it
         for(int i=0 ; i<filled ; ++i)
            if (dists[i]==newDist)
            {
               counts[i] += cnt;
               return (i);
            }
         // not found; are all entries occupied?
         if (filled==5) // yep, it is full
            return (-1);
         dists[filled] = newDist;
         counts[filled++] = cnt;
         return (filled-1);
      }
      
      void addElementsToVector(DCVector &vec) const
      {
         for (int i=0 ; i<filled ; ++i)
            vec.push_back(DistCountPair(dists[i], counts[i]) );
      }
   
      count_t counts[5];
      dist_t  dists[5];
      int32_t filled;
   };
   typedef MIAMIU::MiamiAllocator<ArrayCT, 8> ArrayReuseAllocator;
   
   typedef enum {NO_REUSE_INFO, SCALAR_REUSE_INFO, ARRAY_REUSE_INFO,
                 HISTOGRAM_REUSE_INFO, NUM_REUSE_INFO} ReuseInfoState;
   class reuse_info_t
   {
   public:
      reuse_info_t (uint64_t _imgSrc, uint64_t _idxSrc, uint64_t _imgCarry, 
                 uint64_t _idxCarry) 
      { 
         sourceId = MAKE_PATTERN_KEY(_imgSrc, _idxSrc);
         carryId  = MAKE_PATTERN_KEY(_imgCarry, _idxCarry);
         dist = 0;
         u.count = 0;
         state = NO_REUSE_INFO;
      }

      ~reuse_info_t()
      {
         if (state==HISTOGRAM_REUSE_INFO && u.reuseHist)
            delete (u.reuseHist);
      }
      
      inline void setCount(long cc) {
         u.count = cc;
      }

      inline bool operator==(const reuse_info_t& ri) const
      {
         return (sourceId==ri.sourceId && carryId==ri.carryId);
      }
      inline bool operator!=(const reuse_info_t& ri) const
      {
         return (sourceId!= ri.sourceId || carryId!=ri.carryId);
      }
      
      union {
        HashMapCT *reuseHist;
        ArrayCT *reuseVec;
        long count;
      } u;
      uint64_t sourceId, carryId;
      dist_t dist;
      uint8_t state;
   };
   typedef MIAMIU::SelfSortedList <reuse_info_t, 4> RI_List;

   typedef struct _inst_info
   {
      // I use a self sorted list, so the most recently accessed element is always at 
      // the front of the list, I do not use an explicit cache anymore
      RI_List *reuseVec;  // list of reuse histograms, one entry for each reuse pattern
      // the number of cold misses is equal to the total program footprint. 
      // We cannot profile a program that uses more than 4 billion memory blocks because 
      // it would take forever to finish, so a 32-bit counter is safe for cold misses.
      dist_t coldMisses; 
   } InstInfo;

   // for each scope I should keep a histogram of its footprint on each execution
   // anything else?
   typedef HashMapCT ScopeInfo;

   typedef MIAMIU::DenseContainer<InstInfo, 256> IIContainer;
   typedef MIAMIU::DenseContainer<ScopeInfo, 64> SIContainer;

   // I must aggregate results at instruction and scope level. However, those IDs are
   // unique inside each image only, so I will define a structure to hold information
   // about each image.
   // I do not know the number of instruction images a priori, so I need a data structure 
   // with fast random access that also grows.
   typedef struct _image_info
   {
      IIContainer instData;
      SIContainer scopeData;
   } ImageInfo;
   typedef MIAMIU::DenseContainer<ImageInfo, 4> ImgContainer;

   // I need to maintain a stack of scopes; Keep only the scopeId and the 
   // value of the time stamp when we entered this scope;
   // Add also an iteration number, incremented on an ITERATION_CHANGE event
   #define UNKNOWN_SCOPE_ID   0
   class StackElem
   {
   public:
      StackElem (count_t _eTime, addrtype _sp, uint32_t sid, uint16_t img, uint16_t iter=0) : 
            enterTime(_eTime), sp(_sp), scopeId(sid), imgId(img), iterNumber(iter)
      { }
      
      count_t  enterTime;
      addrtype sp;         /* store the stack pointer value at scope entry */
      uint32_t scopeId;
      uint16_t imgId;      /* imgID is 0 for routines, >0 for loops. */
      uint16_t iterNumber; /* this can be only 0 or 1, so 15 bits are available. */
   };
   typedef MIAMIU::GrowingStack<StackElem> ScopeStack;
   
   #define STACK_FRAME_IS_DEEPER(sp,ref_sp)         ((sp)<(ref_sp))
   #define STACK_FRAME_IS_NOT_SHALLOWER(sp,ref_sp)  ((sp)<=(ref_sp))
   
   // I must store some bits of info about each accessed memory block
   class BlockInfo
   {
   public:
      BlockInfo(void *addrN=0, uint32_t img=0, uint32_t iidx=0) : 
            addrNode(addrN), lastIIdx(iidx), lastImgId(img)
      { 
      }
      
      void* addrNode;
      /* Store the image and instruction index that last accessed this memory 
       * block. Right now they are both 32-bit values because I want the structure
       * size to be a multiple of 8 bytes, but the image ID fits into a smaller
       * type (16-bit) if I ever need to use 16 bits for something else.
       */
      uint32_t lastIIdx;   /* 32-bits for the instruction index inside its image */
      uint32_t lastImgId;  /* 32-bits for the image index */
   };
   #define IMG_HAS_INST_INDEX(imgId)  (((imgId) >> 31) != 0)
   #define IMG_SET_INST_INDEX(imgId)  ((imgId) | (1<<31))
   #define IMG_GET_IMG_INDEX(imgId)  ((imgId) & 0x7fffffff)
   
   typedef MIAMIU::BlockMapper<BlockInfo> BlockMapperType;
   typedef MIAMIU::MRDSplayTree<>  MRDTree;

   class ThreadLocalData
   {
   public:
      ThreadLocalData(int _tid) {
         tid = _tid;
         memBlocksAccessed = 0;
         logicClock = 0;
         residualScopes = 0;

         memset(eventCount, 0, sizeof(eventCount));
         libraryCalls = 0;
         badScopeCount = 0;
         lastLocation = (addrtype)-1;
         lastEventWasBrToCall = false;
         lastAux = 0;
         
         reuseAllocator = new ArrayReuseAllocator(sizeof(ArrayCT)*64);
         
         bmapper = new BlockMapperType(addr_mask >> addr_shift);
         
         // Calls to ThreadStart are serialized by PIN; this is safe (w/ PIN)
         next = lastTLD;
         lastTLD = this;
      }
      ~ThreadLocalData() { 
         if (bmapper)
            delete (bmapper);
         if (reuseAllocator)
            delete (reuseAllocator);
      }
      
      inline int ProcessEvents(uint64_t numElems, struct MemRefRecord *ref);
      inline void PurgeStack();
      void OutputResultsToFile(FILE *fout);

   private:
      int tid;   // thread ID; I create one object for each thread
      
      // Below are the data structures needed during processing
      BlockMapperType *bmapper;  // indexing table for each accessed memory block
      MRDTree         tree;     // balanced search tree for computing MRD / footprint
      ScopeStack      stack;    // dynamic scope stack for computing carrying scopes
      
      // I also need data structures to aggregate the results at instruction and 
      // at the scope level respectively. However, instruction and scope IDs make
      // sense only with a corresponding Image ID, so they must be indexed by both 
      // the image ID and the instruction / scope IDs. Hmmm.
      ImgContainer imgData;

      // custom allocator for the short array reuse info class
      ArrayReuseAllocator *reuseAllocator;

      // also build a histogram of observed errors
      ScopeErrorsMap scopeErrors;

      // keep also a bunch of counters for the different types of events
      dist_t memBlocksAccessed;
      uint32_t numDistinctScopes;
      uint32_t numDistinctInstructions;
      uint32_t residualScopes;
      
      count_t logicClock;
      count_t eventCount[MEMREUSE_EVTYPE_LAST];
      count_t libraryCalls;  // how many times the event processing routine was called

      count_t badScopeCount;
      addrtype lastLocation;
      
      bool lastEventWasBrToCall;
      int lastAux;
      
      // maintain a daisy chain of these structures. Should I?
      // If a thread ends, do I deallocate its data structure?
      // Then, I have to maintain also a prev link to restore the chain.
      ThreadLocalData *next;

      static ThreadLocalData *lastTLD;
   };
   
   ThreadLocalData* ThreadLocalData::lastTLD = 0;
   
   void* CreateThreadLocalData(int tid)
   {
      return (new ThreadLocalData(tid));
   }

   void FinalizeThreadData(void *tdata, FILE *fout)
   {
      ThreadLocalData *tld = static_cast<ThreadLocalData*>(tdata);
      // I need to empty the entire stack first, compute footprint for
      // the residual scopes
      tld->PurgeStack();
      
      /* Dump results to file */
      if (fout)
         tld->OutputResultsToFile(fout);
      
      delete (tld);
   }

   int  
   ProcessBufferedElements(uint64_t numElems, struct MemRefRecord *ref, void *tdata)
   {
      ThreadLocalData *tld = static_cast<ThreadLocalData*>(tdata);
      return (tld->ProcessEvents(numElems, ref));
   }
   
   const char* 
   EventTypeToString(MemReuseEvType ev)
   {
      switch (ev)
      {
         case MEMREUSE_EVTYPE_INVALID:       return ("Invalid");
         case MEMREUSE_EVTYPE_MEM_LOAD:      return ("Memory Load");
         case MEMREUSE_EVTYPE_MEM_STORE:     return ("Memory Store");
         case MEMREUSE_EVTYPE_ENTER_SCOPE:   return ("Scope Entry");
         case MEMREUSE_EVTYPE_ROUTINE_ENTRY: return ("Routine Entry");
         case MEMREUSE_EVTYPE_ITER_CHANGE:   return ("Iteration Change");
         case MEMREUSE_EVTYPE_EXIT_SCOPE:    return ("Scope Exit");
         case MEMREUSE_EVTYPE_AFTER_CALL:    return ("After Call");
         case MEMREUSE_EVTYPE_BR_BYPASS_CALL:return ("Call Bypass");
         default:  return ("BadEvent");
      }
      return ("BadEvent");
   }

   inline void
   ThreadLocalData::PurgeStack()
   {
      libraryCalls += 1;
      
      while (!stack.empty()) 
      {
         StackElem sTop = stack.top();
         stack.pop();
         if (sTop.iterNumber>1)  // this is an iteration scope. 
         {
             // the next element should be the actual scope entry
             // check that we have the same scopeId and this is not the Top
             // I should check that imgIds are the same as well, but that is
             // paranoia ... the destroyer
             assert(!stack.empty() && stack.top().scopeId==sTop.scopeId);
             continue;
         }
         ++ residualScopes;
         
         // If we have to compute FOOTPRINT
         if (compute_footprint)
         {
            ImageInfo &simg = imgData[sTop.imgId];
            dist_t footp = tree.count_blocks(sTop.enterTime);
            ScopeInfo& siref = simg.scopeData[sTop.scopeId];
            siref[footp] += 1;
         }
      }  // while not stack empty
   }
   
   inline int
   ThreadLocalData::ProcessEvents(uint64_t numElems, struct MemRefRecord *ref)
   {
      unsigned int i;
      libraryCalls += 1;
      
      i = 0;
      if (lastEventWasBrToCall)
      {
         if (ref->u.info.type!=MEMREUSE_EVTYPE_AFTER_CALL)
         {
            // if the BYAPSS edge was an indirect branch, we could have been wrong.
            // Ignore the bypass event;
            if (lastAux!=MIAMI::PrivateCFG::MIAMI_INDIRECT_BRANCH_EDGE)
            {
               assert(ref->u.info.type==MEMREUSE_EVTYPE_AFTER_CALL || 
                    lastAux!=MIAMI::PrivateCFG::MIAMI_INDIRECT_BRANCH_EDGE);
            }
         } else
         {
            // skip first event
            i = 1;
            ++ ref;
         }
         lastEventWasBrToCall = false;
         lastAux = 0;
      }
      for ( ; i<numElems ; ++i, ++ref)
      {
         int evType = ref->u.info.type;
         int imgId = ref->u.info.imgid;
         
         eventCount[evType] += 1;
         switch (evType)
         {
            case MEMREUSE_EVTYPE_MEM_LOAD:
            case MEMREUSE_EVTYPE_MEM_STORE:
            {
               // increment the logial clock on each memory access
               logicClock += 1;
               int32_t scopeId = ref->ea2;
//               fprintf(stderr, "Img %d, ref %d is in scope %d\n", imgId, ref->iidx, scopeId);
               
               ImageInfo &iinfo = imgData[imgId];
               addrtype location = ref->ea >> addr_shift;
               // use *is_store* when you compute the dirty lines
               // keep it commented until then
               //unsigned int is_store = (evType == MEMREUSE_EVTYPE_MEM_STORE);
               
               InstInfo& iiref = iinfo.instData[ref->iidx];
               register dist_t dist = 0;

               // if the location is the same as lastLocation, I do not need to do anything,
               // but increment the counter associated with the instruction.
               
               BlockInfo *binfo = (*bmapper)[location];
               void*& naddr = binfo->addrNode;
               
               if (naddr == NULL)  // this is a cold miss, the easy case
               {
//                  ++ memBlocksAccessed;  // I sum up the cold misses later
                  iiref.coldMisses += 1;
                  naddr = tree.cold_miss();
                  // nothing else to do for this case
               } else
               {
                  // first find which scope carries the reuse
                  uint32_t carryId = UNKNOWN_SCOPE_ID;
                  uint32_t carryImg = 0;

                  uint64_t oldTime = static_cast<MIAMIU::MRDTreeNode*>(naddr)->key;
                  // traverse the stack until we find a scope that we entered 
                  // before _oldTime
                  ScopeStack::ElementsIterator stit (&stack);
                  while ((bool)stit)
                  {
                     StackElem *se = (StackElem*)stit;
                     if (se->enterTime < oldTime)  // found it
                     {
                        // Note the iterNumber of 1 means cross iteration
                        // and a value of 0 or 2 means same iteration.
                        // However, I do not insert ITER_CHANGE events for
                        // innermost loops with no function calls and for 
                        // routines, so I only have an entry with iterNumber=0 
                        // for them. Hmm ...
                        carryId = (se->scopeId << 1) + (se->iterNumber & 1);
                        carryImg = se->imgId;
                        break;
                     }
                     ++ stit;
                  }

                  if (location != lastLocation)
                     dist = tree.reuse_block(naddr);
                  // Compute some approximate distance, to see if we can reduce memory consumption
                  dist_t halfd = dist >> 8;

                  // Right now, sourceId is defined by the index of the 
                  // instruction. It may be better to use the ID of the
                  // scope containing the instruction, to cut on the number
                  // of distinct patterns.
                  // In the post-processing analysis I will need to aggregate
                  // all the reuse patterns with the same carrier and with 
                  // source instructions parts of the same scope.
                  uint32_t srcInst = binfo->lastIIdx;
                  uint32_t srcImg = binfo->lastImgId;
                  dist_t delta = halfd + halfd;
                  
                  if (!iiref.reuseVec)  // first time we see reuse with this instruction
                     iiref.reuseVec = new RI_List();
                     
                  reuse_info_t ritmp(srcImg, srcInst, carryImg, carryId);
                  reuse_info_t &reuseElem = iiref.reuseVec->insert(ritmp);
                  dist += halfd;
                  for (dist_t x=halfd, ll=2 ; x>0 ; ll<<=1)
                  {
                     delta |= x;
                     x = delta >> ll;
                  }
                  dist &= (~delta);
                  if (reuseElem.state == NO_REUSE_INFO)
                  {
                     reuseElem.dist = dist;
                     reuseElem.u.count = 1;
                     reuseElem.state = SCALAR_REUSE_INFO;
                  } else
                  if (reuseElem.state == SCALAR_REUSE_INFO)
                  {
                     if (reuseElem.dist == dist)
                        reuseElem.u.count += 1;
                     else   // distances do not match. Create short array
                     {
                        count_t oldCnt = reuseElem.u.count;
                        ArrayCT* &tempA = reuseElem.u.reuseVec;
                        reuseElem.state = ARRAY_REUSE_INFO;
                        tempA = reuseAllocator->new_elem();  //new ArrayCT();
                        tempA->filled = 0;
                        tempA->addDistance(dist, 1);
                        tempA->addDistance(reuseElem.dist, oldCnt);
                     }
                  } else
                  if (reuseElem.state == ARRAY_REUSE_INFO)
                  {
                     if (reuseElem.u.reuseVec->addDistance(dist, 1) < 0)
                     {
                        ArrayCT* oldVec = reuseElem.u.reuseVec;
                        HashMapCT* &tempH = reuseElem.u.reuseHist;
                        reuseElem.state = HISTOGRAM_REUSE_INFO;
                        tempH = new HashMapCT();
                        tempH->insert(dist) = 1;
                        for (int f=0 ; f<5 ; ++f)
                           tempH->insert(oldVec->dists[f]) = oldVec->counts[f];
                        reuseAllocator->delete_elem(oldVec);
                     }
                  } else  // it must be a histogram already
                  {
                     reuseElem.u.reuseHist->insert(dist) += 1;
                  }
               }

               static_cast<MIAMIU::MRDTreeNode*>(naddr)->key = logicClock;
               if (scopeId)
               {
                  binfo->lastIIdx = scopeId;
                  binfo->lastImgId = imgId;
               } else
               {
                  binfo->lastIIdx = ref->iidx;
                  binfo->lastImgId = IMG_SET_INST_INDEX(imgId);
               }

               lastLocation = location;

               break;
            }
               
            case MEMREUSE_EVTYPE_ROUTINE_ENTRY:
//               fprintf(stderr, "Img %d, routine %d was called from 0x%" PRIxaddr "\n", imgId, ref->iidx, ref->ea2);
               
            case MEMREUSE_EVTYPE_ENTER_SCOPE:
            {
               stack.push(StackElem(logicClock, ref->ea, ref->iidx, imgId, 0));
               break;
            }
            
            case MEMREUSE_EVTYPE_ITER_CHANGE:
            case MEMREUSE_EVTYPE_EXIT_SCOPE:
            case MEMREUSE_EVTYPE_AFTER_CALL:
            {
               StackErrorType stackError = STACK_ERROR_NO_ERROR;
               
               ScopeStack::ElementsIterator elit (&stack);
               // Look only at the Stack Pointer in a first pass. Skip all the 
               // stack frames deeper than current event. When I change 
               // iterations, or when I exit a loop, the stack should not be
               // any deeper than when I entered the loop, otherwise it would
               // keep growing with each iteration.
               int num_levels = 0;
               int add_scope = 0;  // determine if we have to add the scope to the stack first
               while ((bool)elit && STACK_FRAME_IS_DEEPER(elit->sp,ref->ea))
               {
#if 0
                  if (verbose_level>4)
                     fprintf(stderr, "1] Elem %d, Found stack entry with sp=0x%" PRIxaddr 
                          ", img=%d, scope=%d, iter=%d\n",
                          i, elit->sp, elit->imgId, elit->scopeId, elit->iterNumber);
#endif
                  ++ num_levels;
                  ++ elit;  // advance towards stack bottom
               }
               while ((bool)elit && STACK_FRAME_IS_NOT_SHALLOWER(elit->sp,ref->ea) &&
                       (elit->scopeId!=ref->iidx || elit->imgId!=imgId))
               {
#if 0
                  if (verbose_level>4)
                     fprintf(stderr, "2] Elem %d, Found stack entry with sp=0x%" PRIxaddr 
                          ", img=%d, scope=%d, iter=%d\n",
                          i, elit->sp, elit->imgId, elit->scopeId, elit->iterNumber);
#endif
                  ++ num_levels;
                  ++ elit;  // advance towards stack bottom
               }
               if ((bool)elit && elit->scopeId==ref->iidx && elit->imgId==imgId)
               {
                  // entry for correct scope found
                  // Understanding if we have seen more stack levels than we should
                  // considering that some scopes might have 2 entries on stack,
                  // and that some events may exit from multiple scopes, is a bit
                  // too expensive to maintain. As long as we find a hit, I do 
                  // not consider it an error.
//                  if (evType!=MEMREUSE_EVTYPE_AFTER_CALL && num_levels>ref->u.info.aux)
//                     stackError = STACK_ERROR_SCOPE_NOT_ON_TOP;
                  
                  if (evType == MEMREUSE_EVTYPE_EXIT_SCOPE)
                  {
                     // for EXIT_SCOPE, the specified scopeId is the outermost 
                     // exited scope, so it must be removed from the stack
                     if (elit->iterNumber>1)  // this is the ITER entry
                     {
                        ++ elit;   // move back one more entry; this should be the scope entry
                        assert((bool)elit && elit->scopeId==ref->iidx && 
                                elit->imgId==imgId && elit->iterNumber==1);
                     }
                  } else  // ITER_CHANCE or AFTER_CALL
                  {
                     // for ITER_CHANGE and AFTER_CALL, the specified scopeId
                     // must remain at the top of the stack
                     //
                     // next test applies only to ITER_CHANGE, but there is no
                     // harm if we do this for AFTER_CALL events too (avoid 
                     // one extra test)
                     if (evType==MEMREUSE_EVTYPE_ITER_CHANGE && elit->iterNumber==0)
                     {
                        // we only have the scope record; must add an iteration entry
                        // change iterNumber to 1 for the scope entry
                        elit->iterNumber = 1;
                        add_scope = 1;
                     }
                     -- elit;  // backtrack one level, so we can use the same
                               // code to remove scopes.
                  } 
               } else
               {
                  // entry not found; check if we have to backtrack a few positions to
                  // find the right spot to fill in the blanks
                  // It can be that we reached the bottom of the stack, or that
                  // we reached a stack frame that is shallower than current request
                  --elit;  // backtrack one element if possible
                  stackError = STACK_ERROR_SCOPE_NOT_FOUND;
                  while (!elit.IsBeyondTop() && elit->imgId==imgId && 
                        allImgs[imgId]->ScopeIsAncestor(elit->scopeId, ref->iidx))
                  {
#if 0
                     if (verbose_level>4)
                        fprintf(stderr, "3] Elem %d, Reverse stack entry with sp=0x%" PRIxaddr 
                             ", img=%d, scope=%d, iter=%d\n",
                             i, elit->sp, elit->imgId, elit->scopeId, elit->iterNumber);
#endif
                     -- elit;  // advance towards stack top
                  }
                  // must add both a scope and an iteration entry
                  add_scope = 2;
               }
               
               // elit is either beyond the Top of the stack, or it 
               // points to the deepest stack element that is not an 
               // ancestor to our scope. I must remove everything 
               // from elit and up, then add two entries for my scope.
               if ((bool)elit && !elit.IsBeyondTop())
               {
                  bool stopCond = false;
                  do {
                     if (elit.IsTop()) 
                        stopCond = true;  // this will be the last element removed
                     // Once I detect the stop condition, I still need to remove
                     // the last element
                     StackElem sTop = stack.top();
                     stack.pop();
#if 0
                     if (verbose_level>4)
                        fprintf(stderr, "POP] Elem %d, Popped entry with sp=0x%" PRIxaddr 
                             ", img=%d, scope=%d, iter=%d\n",
                             i, sTop.sp, sTop.imgId, sTop.scopeId, sTop.iterNumber);
#endif
                     // If we have to compute FOOTPRINT (add a command line option later)
                     if (sTop.iterNumber>1)  // this is an iteration scope. 
                     {
                         // the next element should be the actual scope entry
                         // check that we have the same scopeId and this is not the Top
                         // I should check that imgIds are the same as well, but that is
                         // paranoia ... the destroyer
                         assert(!stopCond && stack.top().scopeId==sTop.scopeId);
                         continue;
                     }
                     
                     if (compute_footprint)
                     {
                        ImageInfo &simg = imgData[sTop.imgId];
                        dist_t footp = tree.count_blocks(sTop.enterTime);
                        ScopeInfo& siref = simg.scopeData[sTop.scopeId];
                        siref[footp] += 1;
                     }
                  } while (!stopCond && !stack.empty());
                  assert(stopCond || !"Did not find the stack Top while popping elements!");
               }
               
               if (evType == MEMREUSE_EVTYPE_ITER_CHANGE)
               {
                  if (add_scope==2)  // add a scope entry
                     stack.push(StackElem(logicClock, ref->ea, ref->iidx, imgId, 1));
                  if (add_scope>0)
                     stack.push(StackElem(logicClock, ref->ea, ref->iidx, imgId, 2));
                  else
                     stack.top().enterTime = logicClock;
               }
               
               if (stackError!=STACK_ERROR_NO_ERROR)
               {
                  if (profileErrors)
                  {
                     // do something with stackError, scopeId and imgId
                     uint64_t errKey = MAKE_STACK_ERROR_KEY(imgId, ref->iidx, evType, stackError);
                     scopeErrors[errKey] += 1;
                  } else
                     badScopeCount += 1;
               }
               break;
            }
            
            case MEMREUSE_EVTYPE_BR_BYPASS_CALL:
            {
#if 0
               int32_t memop = 0;
               addrtype saddr = 0;
               MIAMI::LoadModule *myImg = allImgs[imgId];
               saddr = myImg->getAddressForScopeIndex((int32_t)ref->iidx, memop);
               fprintf(stderr, "BYPASS event, elem %d/%" PRIu64 ": imgid=%d (%s), instidx=%d, stackp=0x%" PRIxaddr 
                     ", scopePC=0x%" PRIxaddr ", level=%d\n",
                   i, numElems, imgId, myImg->Name().c_str(), ref->iidx, ref->ea, saddr, memop);
#endif
               // peek ahead if we are not at the end of the buffer, to see
               // if the next event in the buffer is an AFTER_CALL event. 
               // It should be. In this case, just skip both events.
               lastAux = ref->u.info.aux;
               if (i < (numElems-1))  // peek ahead
               {
                  i += 1;
                  ref += 1;
                  if (ref->u.info.type!=MEMREUSE_EVTYPE_AFTER_CALL)
                  {
                     if (lastAux==MIAMI::PrivateCFG::MIAMI_INDIRECT_BRANCH_EDGE)
                     {
                        // the BYAPSS edge was an indirect branch. We could have been wrong
                        // Ignore the bypass event; go back to the last element
                        i -= 1;
                        ref -= 1;
                        lastEventWasBrToCall = false;
                        lastAux = 0;
                     } else
                     {
#if 0
                        int32_t img2 = ref->u.info.imgid;
                        MIAMI::LoadModule *myImg2 = allImgs[img2];
                        saddr = myImg2->getAddressForInstIndex((int32_t)ref->iidx, memop);
                        fprintf(stderr, "ProcessBuffer, elem %d/%" PRIu64 ": expecting AFTER_CALL event, instead type=%d\n",
                                i, numElems, ref->u.info.type);
                        fprintf(stderr, " - imgid=%d (%s), instidx=%d, stackp=0x%" PRIxaddr
                               ", instPC=0x%" PRIxaddr ", memop=%d\n",
                                img2, myImg2->Name().c_str(), ref->iidx, ref->ea, saddr, memop);
                        fprintf(stderr, " - img base_addr=0x%" PRIxaddr ", low_offset=0x%" PRIxaddr "\n",
                                myImg2->BaseAddr(), myImg2->LowOffsetAddr());
#endif
                        assert(ref->u.info.type==MEMREUSE_EVTYPE_AFTER_CALL || 
                             lastAux!=MIAMI::PrivateCFG::MIAMI_INDIRECT_BRANCH_EDGE);
                     }
                  }
                  continue;
               } else
               {
                  lastEventWasBrToCall = true;
               }
               break;
            }
            
            default:
               fprintf(stderr, "ERROR: Invalid event type %d\n", evType);
         }
      }
      return (0);
   }
   
   static int
   printStackError(void *arg0, uint64_t key, void* value)
   {
      count_t *cnt = static_cast<count_t*>(value);
      FILE *fout = static_cast<FILE*>(arg0);
      if (!fout)
         fout = stderr;
      int error = key & 0xff;
      key >>= 8;
      int event = key & 0xff;
      key >>= 8;
      int scopeId = key & 0xffffffff;
      key >>= 32;
      int imgId = key & 0xffff;
      fprintf(fout, "Scope Error %d on event %s in IMG %d, scope %d --> %" PRIcount "\n",
           error, EventTypeToString((MemReuseEvType)event), imgId, scopeId, *cnt);
      return (0);
   }
   
   static int
   aggregateErrorHistograms(void *arg0, uint64_t key, void* value)
   {
      count_t *cnt = static_cast<count_t*>(value);
      ScopeErrorsMap *gMap = static_cast<ScopeErrorsMap*>(arg0);
      (*gMap)[key] += *cnt;
      return (0);
   }
   
   static int 
   processBinToVector(void* arg0, dist_t key, void* value)
   {
      DCVector* histogram = static_cast<DCVector*>(arg0);
      count_t* pValue = (count_t*)value;
      histogram->push_back(DistCountPair(key, *pValue) );
      return 0;
   }

   count_t
   WriteDCVectorToFile(DCVector &dcArray, FILE *fout)
   {
      // The complete histogram of the instruction must be in the dcArray array
      // Sort it and process it
      std::sort(dcArray.begin(), dcArray.end(), DistCountPair::OrderAscByKey1());
      
      // compress the histogram: aggregating bins with close 
      // absolute or relative distances into one bin
      uint32_t crtBin = 0;
      DCVector::iterator it = dcArray.begin();
      unsigned long long crtDistance = 0;
      count_t crtCount = 0;
      dist_t limit1 = it->first;
      count_t totalCount = 0;
      float crtAverage = it->first;
      FloatArray distances;
      CountArray counts;

      while (it != dcArray.end())
      {
         float difference = it->first - limit1;  // crtAverage;
         if (difference <= ABSOLUTE_BIN_LIMIT || (crtAverage>0 && difference/crtAverage <= RELATIVE_BIN_LIMIT))
         {
            crtDistance += ((unsigned long long)(it->first))*(it->second);
            crtCount += it->second;
            crtAverage = (float)crtDistance/crtCount;
            it++;
         }
         else
         {
            if (crtCount>0)
            {
               crtBin++;
               distances.push_back(crtAverage);
               counts.push_back(crtCount);
               totalCount += crtCount;
            }
            crtDistance = 0;
            crtCount = 0;
            crtAverage = (float)it->first;
            limit1 = it->first;
         }
      }
      if (crtCount > 0)  // there is data not accounted for
      {
         crtBin++;
         distances.push_back(crtAverage);
         counts.push_back(crtCount);
         totalCount += crtCount;
      }
      assert (crtBin==distances.size() && crtBin==counts.size());
      
      if (text_mode_output)
         fprintf(fout, " %" PRIcount " %d", totalCount, crtBin);
      else
      {
         fwrite(&totalCount, sizeof(count_t), 1, fout);
         fwrite(&crtBin, 4, 1, fout);
      }

      FloatArray::iterator fit = distances.begin();
      CountArray::iterator uit = counts.begin();
      for( ; fit!=distances.end() ; fit++, uit++ )
      {
         // round the floating point number to closest integer
         // May redeuce output file size
         dist_t avgDist = (dist_t)(*fit + .5);  // we know distances are >= 0
         if (text_mode_output)
            fprintf(fout, " %" PRIdist " %" PRIcount, avgDist, *uit);
//            fprintf(fout, " %4.2f %" PRIcount, *fit, *uit);
         else
         {
            fwrite(&(avgDist), sizeof(dist_t), 1, fout);
//            fwrite(&(*fit), sizeof(float), 1, fout);
            fwrite(&(*uit), sizeof(count_t), 1, fout);
         }
      }

      return (totalCount);
   }
   
   count_t
   WriteDistCountHistogramToFile(const HashMapCT *hist, FILE *fout)
   {
      DCVector dcArray;
      hist->map(processBinToVector, &dcArray);

      return (WriteDCVectorToFile(dcArray, fout));
   }

   count_t
   WriteDistCountVectorToFile(const ArrayCT *vec, FILE *fout)
   {
      DCVector dcArray;
      vec->addElementsToVector(dcArray);
      
      return (WriteDCVectorToFile(dcArray, fout));
   }
   
   
   void
   ThreadLocalData::OutputResultsToFile(FILE *fout)
   {
      unsigned int i;
      // Save everything: scope footprint information, instruction 
      // reuse patterns and distances, one image at a time.
      // I must convert instruction and scope indices into load offsets;
      // All profile data is in container imgData.
      uint32_t nImages = maxImgs;
      int64_t *fOffsets = new int64_t[nImages];
      for (i=0 ; i<nImages ; ++i)
         fOffsets[i] = -1;
      
      // Print first some information about the run: line size, block mask, block value ...
      addrtype tmask = addr_mask>>addr_shift;
      addrtype tval = addr_value>>addr_shift;
      if (text_mode_output)
         fprintf(fout, "%" PRIu32 " 0x%" PRIxaddr " 0x%" PRIxaddr "\n",
                 addr_shift, tmask, tval);
      else
      {
         fwrite(&addr_shift, 4, 1, fout);
         fwrite(&tmask, sizeof(addrtype), 1, fout);
         fwrite(&tval, sizeof(addrtype), 1, fout);
      }
      if (text_mode_output)
         fprintf(fout, "%ld\n", imgData.size());
         
      ImgContainer::const_iterator cit = imgData.first();
      while ((bool)cit)
      {
         int idx = cit.Index();
         assert (allImgs [idx]);
         
         MIAMI::LoadModule *myImg = allImgs[idx];
         if (! text_mode_output)
            fOffsets[idx] = ftell(fout);  // save offset where we start writing data for this image
         
         // For each image, we have information about the scopes,
         // and information about the instruction, but we have numeric indices.
         // Must convert indices back into program addresses.
         // A thread can finish at any time, but other threads may continue
         // to generate new indices; I need to compute the reverse map
         // every time a thread ends.
         // I implemented this logic in the LoadModule class. We compute the
         // reverse mapping on demand, only if it is stale.
         if (text_mode_output)
            myImg->printToFile(fout);
         else
            myImg->saveToFile(fout);  // save basic info about the image
         
         // Save scope footprint information if available
         uint32_t fsection = MIAMI::MIAMI_DATA_SECTION_SCOPE_FOOTPRINT;
         addrtype saddr = 0;
         int32_t level = 0;  // use as memOp for the instruction section
         uint32_t u32var;
         
         if (compute_footprint)
         {
            u32var = cit->scopeData.size();
            if (text_mode_output)
               fprintf(fout, "%" PRIu32 " %" PRIu32 "\n", fsection, u32var);
            else
            {
               fwrite(&fsection, 4, 1, fout);
               fwrite(&u32var, 4, 1, fout);   // number of scopes/entries in section
            }
            uint32_t counter = 0;
            SIContainer::const_iterator scit = cit->scopeData.first();
            while ((bool)scit)
            {
               int sindex = scit.Index();    // the scope index
               saddr = myImg->getAddressForScopeIndex(sindex, level);
               if (text_mode_output)
                  fprintf(fout, "0x%" PRIxaddr " %" PRId32 ":", saddr, level);
               else
               {
                  fwrite(&saddr, sizeof(addrtype), 1, fout);
                  fwrite(&level, 4, 1, fout);
               }
               
               // Now, I need to print the histogram collected for this scope
               // Write a separate function, because I need to print many
               // such histograms
               WriteDistCountHistogramToFile((const HashMapCT*)scit, fout);
               if (text_mode_output)
                  fprintf(fout, "\n");
               
               ++ counter;
               ++ scit;
            }
            if (u32var != counter)
            {
               fprintf(stderr, "The size of the scopes container does not match the number of entries found with the iterator. Expected %" 
                            PRIu32 " entries, iterated over=%" PRIu32 "\n",
                    u32var, counter);
            }
            numDistinctScopes += counter;
         }
         
         // Save instruction reuse information next
         fsection = MIAMI::MIAMI_DATA_SECTION_MEM_REUSE;
         u32var = cit->instData.size();
         if (text_mode_output)
            fprintf(fout, "%" PRIu32 " %" PRIu32 "\n", fsection, u32var);
         else
         {
            fwrite(&fsection, 4, 1, fout);
            fwrite(&u32var, 4, 1, fout);   // number of scopes/entries in section
         }
         
         uint32_t icnt = 0;
         IIContainer::const_iterator icit = cit->instData.first();
         while ((bool)icit)
         {
            char isInstIdx = 0;
            int iindex = icit.Index();    // the instruction index
            saddr = myImg->getAddressForInstIndex(iindex, level);
            if (text_mode_output)
               fprintf(fout, "0x%" PRIxaddr " %" PRId32 "\n", saddr, level);
            else
            {
               fwrite(&saddr, sizeof(addrtype), 1, fout);
               fwrite(&level, 4, 1, fout);
            }
            
            count_t totFreq = 0;
            // Now, I need to print the reuse histogram collected for this 
            // instruction, plus the number of cold misses. 
            // I have a list of reuse patterns
            if (icit->reuseVec)
            {
               RI_List::iterator lit = icit->reuseVec->first();
               while ((bool)lit)  // iterate over all reuse patterns
               {
                  // For each pattern, I must print the sourceId and the carryId
                  // sourceId is an instruction index; it is defined by image ID,
                  // instruction address and memory operand index
                  // carryId is a scope index; it is defined by image ID, scope
                  // address, and level
                  int32_t imgSrcID = GET_PATTERN_IMG(lit->sourceId);
                  int32_t imgSrc = IMG_GET_IMG_INDEX(imgSrcID);
                  int32_t indexSrc = GET_PATTERN_INDEX(lit->sourceId);
                  isInstIdx = IMG_HAS_INST_INDEX(imgSrcID);
                  assert (allImgs [imgSrc]);
                  MIAMI::LoadModule *auxImg = allImgs[imgSrc];
                  if (isInstIdx)
                     saddr = auxImg->getAddressForInstIndex(indexSrc, level);
                  else
                     saddr = auxImg->getAddressForScopeIndex(indexSrc, level);
                  if (text_mode_output)
                     fprintf(fout, "%" PRId32 "/%d/0x%" PRIxaddr "/%" PRId32, 
                              imgSrc, (int)isInstIdx, saddr, level);
                  else
                  {
                     fwrite(&imgSrc, 4, 1, fout);                // image ID
                     fwrite(&isInstIdx, sizeof(char), 1, fout);  // next address is instruction or scope
                     fwrite(&saddr, sizeof(addrtype), 1, fout);  // instr address
                     fwrite(&level, 4, 1, fout);                 // memory operand
                  }
                  
                  int32_t imgCarry = 0;
                  int32_t iter = 0;
                  if (lit->carryId)
                  {
                     imgCarry = GET_PATTERN_IMG(lit->carryId);
                     int32_t indexCarry = GET_PATTERN_INDEX(lit->carryId);
                     assert (allImgs [imgCarry]);
                     auxImg = allImgs[imgCarry];
                     iter = indexCarry & 1;
                     indexCarry >>= 1;
                     saddr = auxImg->getAddressForScopeIndex(indexCarry, level);
                  } else
                  {
                     saddr = 0;
                     level = 0;
                  }
                  if (text_mode_output)
                     fprintf(fout, " %" PRId32 "/0x%" PRIxaddr "/%" PRId32 "/%" PRId32 ":", 
                              imgCarry, saddr, level, iter);
                  else
                  {
                     fwrite(&imgCarry, 4, 1, fout);              // image ID
                     fwrite(&saddr, sizeof(addrtype), 1, fout);  // scope address
                     fwrite(&level, 4, 1, fout);                 // scope level
                     fwrite(&iter, 4, 1, fout);                  // cross iteration
                  }
                  
                  assert (lit->u.reuseHist);
                  if (lit->state == SCALAR_REUSE_INFO)
                  {
                     count_t cnt = lit->u.count;
                     totFreq += cnt;
                     if (text_mode_output)
                        fprintf(fout, " %" PRIcount " 1 %" PRIdist " %" PRIcount "\n", 
                             cnt, lit->dist, cnt);
                     else
                     {
                        int32_t numbin = 1;
                        fwrite(&cnt, sizeof(count_t), 1, fout);
                        fwrite(&numbin, 4, 1, fout);
                        fwrite(&lit->dist, sizeof(dist_t), 1, fout);
                        fwrite(&cnt, sizeof(count_t), 1, fout);
                     }
                  }
                  else
                  if (lit->state == ARRAY_REUSE_INFO)
                  {
                     totFreq += WriteDistCountVectorToFile(lit->u.reuseVec, fout);
                     if (text_mode_output)
                        fprintf(fout, "\n");
                  } else
                  {
                     assert (lit->state == HISTOGRAM_REUSE_INFO);
                     totFreq += WriteDistCountHistogramToFile(lit->u.reuseHist, fout);
                     if (text_mode_output)
                        fprintf(fout, "\n");
                  }
                  ++ lit;
               }  // iterate over reuse patterns
            }  // If any reuse patterns
               
            // After saving all the reuse patterns, I need to save an entry
            // with the total frequency and the cold miss count.
            // Mark it with a source ID of all zeros
            saddr = 0; level = 0; isInstIdx = 0;
            if (text_mode_output)
               fprintf(fout, "0/0/0/0:");
            else
            {
               fwrite(&level, 4, 1, fout);    // img ID
               fwrite(&isInstIdx, sizeof(char), 1, fout);  // next address is instruction or scope
               fwrite(&saddr, sizeof(addrtype), 1, fout);  // inst addr
               fwrite(&level, 4, 1, fout);    // memory operand
            }
            
            // Now save total Frequency and cold misses for this instruction
            if (text_mode_output)
               fprintf(fout, " %" PRIcount " %" PRIdist "\n", totFreq, icit->coldMisses);
            else
            {
               fwrite(&totFreq, sizeof(count_t), 1, fout);
               fwrite(&icit->coldMisses, sizeof(dist_t), 1, fout);
            }
            memBlocksAccessed += icit->coldMisses;
            
            ++ icnt;
            ++ icit;
         }  // Iterate over instructions
         if (u32var != icnt)
         {
            fprintf(stderr, "The size of the instruction container does not match the number of entries found with the iterator. Expected %" 
                         PRIu32 " entries, iterated over=%" PRIu32 "\n",
                 u32var, icnt);
         }
         numDistinctInstructions += icnt;
         
         // Mark the end of an image with a line of two zeros
         if (text_mode_output)
            fprintf(fout, "0 0\n");
         else
         {
            u32var = 0;
            fwrite(&u32var, 4, 1, fout);   // invalid section 0
            fwrite(&u32var, 4, 1, fout);   // 0 entries in section
         }
         
         ++ cit;
      }  // Iterate over images
      
      // Save the table of file offsets for each image
      if (! text_mode_output)
      {
         // at the end of the file, save an index table with the file offset for each image
         // I will save the number of images and then for each image save 
         // the image name (in length prefix format) + file offset where it starts
         // Finally, the last 8 bytes store the file offset where the table begins
         int64_t tableOffset = ftell(fout);
         int32_t num_imgs = imgData.size();
         fwrite(&num_imgs, 4, 1, fout);
         for (i=0 ; i<nImages ; ++i)
         {
            if (fOffsets[i]>=0)
            {
               assert (allImgs[i]);
               MIAMI::LoadModule *myImg = allImgs[i];
               uint32_t len = myImg->Name().length();
               fwrite(&len, 4, 1, fout);
               fwrite(myImg->Name().c_str(), 1, len, fout);
               fwrite(&fOffsets[i], 8, 1, fout);
            }
         }
         // finally, save the offset where the table starts
         fwrite(&tableOffset, 8, 1, fout);
      }
      delete[] fOffsets;
      
      // update the global counts of relevant events
      g_libraryCalls += libraryCalls;
      for (i=0 ; i<MEMREUSE_EVTYPE_LAST ; ++i)
         g_eventCount[i] += eventCount[i];
      g_memBlocksAccessed += memBlocksAccessed;
      g_numDistinctScopes += numDistinctScopes;
      g_numDistinctInstructions += numDistinctInstructions;
      
      if (profileErrors)
         scopeErrors.map(aggregateErrorHistograms, &g_scopeErrors);
      else
         g_badScopeCount += badScopeCount;
      
      // Print summary statistics for thread
      const char* prefix = "MIAMI_THREAD_SUMMARY:";
      fprintf(stderr, "%s T%d: Memory blocks accessed = %" PRIdist "\n", prefix, tid, memBlocksAccessed);
      if (compute_footprint)
         fprintf(stderr, "%s T%d: Distinct scopes executed = %" PRIu32 "\n", prefix, tid, numDistinctScopes);
      fprintf(stderr, "%s T%d: Distinct references executed = %" PRIu32 "\n", prefix, tid, numDistinctInstructions);
      fprintf(stderr, "%s T%d: Number of library calls = %" PRIcount "\n", prefix, tid, libraryCalls);
      
      // Compute total number of dynamic events
      count_t totEvents = 0;
      for (i=0 ; i<MEMREUSE_EVTYPE_LAST ; ++i)
         totEvents += eventCount[i];
      fprintf(stderr, "%s T%d: Dynamic event count = %" PRIcount "\n", prefix, tid, totEvents);
      for (i=0 ; i<MEMREUSE_EVTYPE_LAST ; ++i)
         if (eventCount[i])
            fprintf(stderr, "%s T%d:      - %s count = %" PRIcount "\n", prefix, tid, 
                   EventTypeToString((MemReuseEvType)i), eventCount[i]);
      

      fprintf(stderr, "%s T%d: Residual scopes on stack = %" PRIu32 "\n", prefix, tid, residualScopes);
      fprintf(stderr, "%s T%d: Bad stack events = %" PRIcount "\n", prefix, tid, badScopeCount);
      if (profileErrors)
      {
         // print a statistic of observed stack errors
         fprintf(stderr, "%s Scope Errors for Thread %d\n", prefix, tid);
         scopeErrors.map(printStackError, stderr);
      }
   }

   void PrintGlobalStatistics()
   {
      unsigned int i;
      // Print summary statistics for program
      const char* prefix = "MIAMI_SIMULATION_SUMMARY";
      fprintf(stderr, "==============================================================================\n");
      fprintf(stderr, "%s: Number of library calls = %" PRIcount "\n", prefix, g_libraryCalls);
      
      // Compute total number of dynamic events
      count_t totEvents = 0;
      for (i=0 ; i<MEMREUSE_EVTYPE_LAST ; ++i)
         totEvents += g_eventCount[i];
      fprintf(stderr, "%s: Dynamic event count = %" PRIcount "\n", prefix, totEvents);
      for (i=0 ; i<MEMREUSE_EVTYPE_LAST ; ++i)
         if (g_eventCount[i])
            fprintf(stderr, "%s:      - %s count = %" PRIcount "\n", prefix, 
                   EventTypeToString((MemReuseEvType)i), g_eventCount[i]);
      
      fprintf(stderr, "%s: Bad stack events = %" PRIcount "\n", prefix, g_badScopeCount);
      if (profileErrors)
      {
         // print a statistic of observed stack errors
         fprintf(stderr, "%s: Scope Errors By Type\n", prefix);
         g_scopeErrors.map(printStackError, stderr);
      }
      fprintf(stderr, "==============================================================================\n");
   }
   
}  /* namespace MIAMI_MEM_REUSE */

