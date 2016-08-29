/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: cache_sim.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements a typical cache simulator using an LRU replacement
 * policy and different data structures for small and large cache sets.
 */

#ifndef MIAMI_CACHE_SIM_H_
#define MIAMI_CACHE_SIM_H_

#include "miami_types.h"
#include "bucket_hashmap.h"
#include <stdio.h>

namespace MIAMIU
{
   using MIAMI::addrtype;
   
   typedef enum {CACHE_INVALID, CACHE_HIT, CAPACITY_MISS, CONFLICT_MISS, CONFLICT_HIT,
           NUM_ACCESS_TYPES } CACHE_AccessType;

   class SimpleList
   {
   public:
      SimpleList () { }

      SimpleList *next;
      SimpleList *prev;
      addrtype location;
      uint32_t pre_hash;
      uint8_t is_dirty;
   };

   extern SimpleList *defSimpleListPtr;
   typedef MIAMIU::BucketHashMap <addrtype, SimpleList*, &defSimpleListPtr> HashMapSLP;

   /* define two CacheSet classes. One for high associativity caches (including 
    * fully-associative), and another one for small associativity caches 
    * (including direct mapped).
    * LargeCacheSet uses an hash table to store the lines (works better for high 
    * associativity). SmallCacheSet uses an array to store the lines and works 
    * best for small associativity.
    */

   class CacheSet
   {
   public:
      CacheSet (uint32_t _numLines) {
         numLinesInSet = _numLines;
         written_back_lines = 0;
         for (int i=0 ; i<NUM_ACCESS_TYPES ; ++i)
            accessCount[i] = 0;
      }
      virtual ~CacheSet () { }
      
      inline CACHE_AccessType 
      reference (addrtype _location, 
                    CACHE_AccessType refType, uint8_t is_store, uint8_t *wroteBack)
      { 
         return (CACHE_INVALID);
      }
      
      void dumpToFile (FILE* fd);
      
      inline uint64_t countOfType (int type)
      {
         return (accessCount[type]);
      }

      inline uint64_t LinesWrittenBack ()
      {
         return (written_back_lines);
      }

   protected:
      uint32_t numLinesInSet;
      uint64_t written_back_lines;
      uint64_t accessCount[NUM_ACCESS_TYPES];
   };

   class LargeCacheSet : public CacheSet
   {
   public:
      LargeCacheSet (uint32_t _numLines) : CacheSet (_numLines)
      {
         lruBlock = mruBlock = NULL;
         uint32_t hashSize = 1;
         while (_numLines>0)
         {
            hashSize <<= 1;
            _numLines >>= 1;
         }
         // hashSize will be the minimum power of two that is strictly greater
         // than _numLines/4; If _numLines is a power of 2, then hashSize will be
         // the next power of two (allow for some slack in the hashtable)
         setLines = new HashMapSLP (hashSize);
      }
      
      virtual ~LargeCacheSet ()
      {
         delete setLines;
         SimpleList *iter = lruBlock;
         while (iter!=NULL)
         {
            lruBlock = iter->next;
            delete iter;
            iter = lruBlock;
         }
      }
      
      inline CACHE_AccessType 
      reference (addrtype _location, 
                 CACHE_AccessType refType, uint8_t is_store, uint8_t *wroteBack)
      {
         CACHE_AccessType result;
         *wroteBack = 0;
         unsigned int loc_hash = setLines->pre_hash(_location);
         if (setLines->is_member (_location, loc_hash))
         // location is in cache
         {
            result = (refType==CAPACITY_MISS ? CONFLICT_HIT : CACHE_HIT);
            // must update position of block in the list
            SimpleList* elem = setLines->insert(_location, loc_hash);
            
            // if it is a store, mark the block as dirty
            elem->is_dirty += is_store;
            if (elem->next != NULL)   // only if it is not already the MRU block
            {
               elem->next->prev = elem->prev;
               if (elem->prev)
                  elem->prev->next = elem->next;
               else
                  lruBlock = elem->next;
               elem->prev = mruBlock;
               elem->next = NULL;
               mruBlock->next = elem;
               mruBlock = elem;
            }
         } else   // location is not in cache
         {
            result = (refType==CACHE_HIT ? CONFLICT_MISS : CAPACITY_MISS);
            
            if (setLines->size() < numLinesInSet)   // cache is not full yet
            {
               SimpleList*& elem = setLines->insert(_location, loc_hash);
               // elem should be zero at this point; create a new List element
               elem = new SimpleList();
   //            elem = list_allocator->new_elem(); //new SimpleList();
               elem->location = _location;
               elem->next = NULL;
               elem->prev = mruBlock;
               elem->pre_hash = loc_hash;
               elem->is_dirty = is_store;
               if (mruBlock!=NULL)
                  mruBlock->next = elem;
               else
                  lruBlock = elem;
               mruBlock = elem;
            } else     // cache is full; replace the LRU block
            {
               // number of lines should be greater than zero
               // lruBlock and mruBlock should be non-NULL at this point
               setLines->erase (lruBlock->location, lruBlock->pre_hash);
               setLines->insert(_location, loc_hash) = lruBlock;
               register SimpleList* elem = lruBlock;
               lruBlock = elem->next;
               lruBlock->prev = NULL;
               elem->prev = mruBlock;
               mruBlock->next = elem;
               mruBlock = elem;
               elem->next = NULL;
               elem->location = _location;
               elem->pre_hash = loc_hash;
               if (elem->is_dirty)
               {
                  ++ written_back_lines;
                  *wroteBack = 1;
               }
               elem->is_dirty = is_store;
            }
         }
         ++ accessCount[result];
         return (result);
      }
      
      void dumpToFile (FILE* fd)
      {
         fprintf(fd, "*** Report for LargeCacheSet with %d lines. There are %d used entries. ***\n", 
                 numLinesInSet, setLines->size() );
         setLines->print_stats(fd);
         CacheSet::dumpToFile (fd);
      }
      
   private:
      SimpleList* lruBlock;
      SimpleList* mruBlock;
      HashMapSLP *setLines;
   };

            
   class SmallCacheSet : public CacheSet
   {
   public:
      SmallCacheSet (uint32_t _numLines) : CacheSet (_numLines)
      {
         _fill = 0;
         logicalClock = 0;
         cache = (addrtype*) memalign (16, numLinesInSet * sizeof(addrtype));
         dirty = (uint8_t*) memalign (16, numLinesInSet * sizeof(uint8_t));
         acTime = (uint64_t*) memalign (16, numLinesInSet * sizeof(uint64_t));
      }

      ~SmallCacheSet()
      {
         free (cache);
         free (dirty);
         free (acTime);
      }
      
      inline CACHE_AccessType 
      reference (addrtype _location, 
                   CACHE_AccessType refType, uint8_t is_store, uint8_t *wroteBack)
      {
         CACHE_AccessType result;
         int index;
         unsigned int i;
         ++ logicalClock;
         *wroteBack = 0;
         
         // find if the element is in cache
         // assume that the location was recently accessed (though the real reason
         // for traversing in reverse order is to get negative index if not found)
         for (index=_fill-1 ; index>=0 ; --index)
            if (cache[index] == _location)
               break;
         if (index>=0)   // location is in cache
         {
            if (refType == CAPACITY_MISS)
               result = CONFLICT_HIT;
            else
               result = CACHE_HIT;

            // must update the access time for this block
            acTime[index] = logicalClock;
            // must check if current access is store to mark the line as dirty
            dirty[index] += is_store;
         } else   // location is not in cache
         {
            if (refType == CACHE_HIT)
               result = CONFLICT_MISS;
            else
               result = CAPACITY_MISS;
            
            if (_fill < numLinesInSet)   // cache is not full yet
            {
               cache[_fill] = _location;
               acTime[_fill] = logicalClock;
               dirty[_fill] = is_store;
               ++ _fill;
            } else  
            {
               // this is the worst case;
               // I have to find the lru block and replace it with the
               // new location
               uint64_t minimClock = acTime[0]; 
               // cache must have at least one line
               index = 0;
               for (i=1 ; i<_fill ; ++i)
                  if (acTime[i]<minimClock)
                  {
                     minimClock = acTime[i];
                     index = i;
                  }
               cache[index] = _location;
               acTime[index] = logicalClock;
               if (dirty[index])
               {
                  ++ written_back_lines;
                  *wroteBack = 1;
               }
               dirty[index] = is_store;
            }
         }
         ++ accessCount[result];
         return (result);
      }
      
      
      void dumpToFile (FILE* fd)
      {
         fprintf(fd, "*** Report for SmallCacheSet with %d lines. There are %d used entries. ***\n", 
                 numLinesInSet, _fill );
         CacheSet::dumpToFile (fd);
      }
      
   private:
      uint64_t logicalClock;
      uint32_t _fill;
      // allocate three arrays; second array will keep the state
      // of each cache line in the cache: dirty or not dirty
      addrtype *cache;
      uint8_t *dirty;
      uint64_t *acTime;
   };
   
   class CacheSim
   {
   public:
      CacheSim (uint32_t _size, uint32_t _line, uint32_t _assoc,
              bool compute_conflict_misses = false);
      ~CacheSim ();
       
      void dumpToFile (FILE* fd, bool detailed=false);

      int getTotalCounts(uint64_t *counters, uint64_t& lines_written_back);

      inline CACHE_AccessType 
      reference (addrtype _location, uint8_t is_store, uint8_t *wroteBack)
      {
         // the received location value already accounts for the line size.
         //_location >>= lineShift;
         
         CACHE_AccessType realResult = CONFLICT_HIT;
         if (fullyAssoc)
         {
            if (largeAssoc)
               realResult = static_cast<LargeCacheSet*>(fullyAssoc)->reference (_location, 
                             CONFLICT_HIT, is_store, wroteBack);
            else
               realResult = static_cast<SmallCacheSet*>(fullyAssoc)->reference (_location, 
                             CONFLICT_HIT, is_store, wroteBack);
         }
         if (setAssoc)
         {
            unsigned int setNumber = _location & setMask;
            if (largeSet)
               realResult = static_cast<LargeCacheSet*>(setAssoc[setNumber])->reference (_location,
                                      realResult, is_store, wroteBack);
            else
               realResult = static_cast<SmallCacheSet*>(setAssoc[setNumber])->reference (_location,
                                      realResult, is_store, wroteBack);
         }
         
         return (realResult);
      }

      inline CACHE_AccessType 
      reference (addrtype _location, uint8_t is_store)
      {
         uint8_t temp;
         // the received location value already accounts for the line size.
         //_location >>= lineShift;

         CACHE_AccessType realResult = CONFLICT_HIT;
         if (fullyAssoc)
         {
            if (largeAssoc)
               realResult = static_cast<LargeCacheSet*>(fullyAssoc)->reference (_location, 
                             CONFLICT_HIT, is_store, &temp);
            else
               realResult = static_cast<SmallCacheSet*>(fullyAssoc)->reference (_location, 
                             CONFLICT_HIT, is_store, &temp);
         }
         if (setAssoc)
         {
            unsigned int setNumber = _location & setMask;
            if (largeSet)
               realResult = static_cast<LargeCacheSet*>(setAssoc[setNumber])->reference (_location,
                                      realResult, is_store, &temp);
            else
               realResult = static_cast<SmallCacheSet*>(setAssoc[setNumber])->reference (_location,
                                      realResult, is_store, &temp);
         }
          
         return (realResult);
      }
       
   private:
      uint32_t size;
      uint32_t numLines;
      uint32_t lineSize;
      uint32_t assocLevel;
      uint32_t numSets;
      uint32_t lineShift;
      uint32_t setMask;
      CacheSet *fullyAssoc;
      CacheSet **setAssoc;
      bool largeSet, largeAssoc;
   };

}  /* namespace MIAMIU */

#endif
