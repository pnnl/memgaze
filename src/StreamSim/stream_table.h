/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: stream_table.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements a table for storing data streams that has O(1) access on a 
 * stream advance.
 */

#ifndef STREAM_TABLE_H
#define STREAM_TABLE_H

#include "miami_types.h"
#include "bucket_hashmap.h"

#define DEBUG_STREAM_DETECTION 0

namespace StreamRD
{
using MIAMI::addrtype;

/* Stream table entry classes
 */
struct StreamEntry {
    addrtype addr;
    int32_t stride;
    StreamEntry *next;
    StreamEntry *prev;
    void* treeNode;
};
 
extern StreamEntry *defStreamEntryPtr;
typedef MIAMIU::BucketHashMap <addrtype, StreamEntry*, &defStreamEntryPtr> HashMapSEP;


/* define two CacheSet classes. One for high associativity caches (including 
 * fully-associative), and another one for small associativity caches 
 * (including direct mapped).
 * LargeCacheSet uses an hash table to store the lines (works better for high 
 * associativity). SmallCacheSet uses an array to store the lines and works 
 * best for small associativity.
 */

class StreamTable
{
public:
   StreamTable (uint32_t _numLines) {
      max_streams = _numLines;
      num_streams = 0;
      streamEntries = (StreamEntry*)malloc(max_streams*sizeof(StreamEntry));
      memset(streamEntries, 0, max_streams*sizeof(StreamEntry));
   }
   virtual ~StreamTable () { 
      free (streamEntries);
   }
   
   void* reference (addrtype _location, int& xstride, int& conflict)
   {
      return (0);
   }
   void** allocateStream(addrtype _location, int stride, int& conflict)
   {
      return (0);
   }

protected:
   int max_streams;
   int num_streams;
   StreamEntry *streamEntries;
};

class LargeStreamTable : public StreamTable
{
public:
   LargeStreamTable (uint32_t _numLines) : StreamTable (_numLines)
   {
      lruBlock = mruBlock = NULL;
      freeList = 0;
      uint32_t hashSize = 1;
      while (_numLines>0)
      {
         hashSize <<= 1;
         _numLines >>= 1;
      }
      // hashSize will be the minimum power of two that is strictly greater
      // the _numLines; If _numLines is a power of 2, then hashSize will be
      // the next power of two (allow for some slack in the hashtable)
      // Since we changed to BucketHashMap, we do not need any slack. In fact,
      // we allocate a table slightly smaller than the number of entries.
      // We can store several entries into a bucket.
      setLines = new HashMapSEP (hashSize);
   }
   
   virtual ~LargeStreamTable ()
   {
      delete setLines;
   }
   
   void* reference (addrtype _location, int& xstride, int& conflict)
   {
#if DEBUG_STREAM_DETECTION
      fprintf(stderr, "Reference: NumStreams=%d, hash size=%d, hash cap=%d\n",
          num_streams, setLines->size(), setLines->capacity());
      fflush(stderr);
#endif
      unsigned int loc_hash = setLines->pre_hash(_location);
      if (setLines->is_member (_location, loc_hash))
      // location is in cache
      {
#if DEBUG_STREAM_DETECTION
         fprintf(stderr, "Referencing elem with hash %x\n", loc_hash);
         fflush(stderr);
#endif
         // must update position of block in the list
         StreamEntry* elem = setLines->insert(_location, loc_hash);
         
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
         // remove elem from hash, elem->addr should be equal to _location
         setLines->erase(elem->addr, loc_hash);
         // in all cases I have to update the expected location
         elem->addr = elem->addr + elem->stride;
         xstride = elem->stride;
         // I also need to reinsert the entry into the hashtable, bah
         // check for conflicts though
         
         // I have a new elem->addr value; cannot use the loc_hash
         StreamEntry*& new_elem = setLines->insert(elem->addr);
#if DEBUG_STREAM_DETECTION
         bool debug = false;
#endif
         if (new_elem)  // conflict
         {
            conflict = 1;
#if DEBUG_STREAM_DETECTION
            fprintf(stderr, "LargeStreamTable::reference, advancing stream (0x%" PRIxaddr 
                    ",%" PRId32 ") conflicts with existing stream (0x%" PRIxaddr ",%" PRId32 ")\n",
                    elem->addr, elem->stride, new_elem->addr, new_elem->stride);
            fflush(stderr);
#endif
//            num_streams -= 1;
            // The block pointed to by new_elem will be freed, add it to a free list
            if (new_elem->prev)
               new_elem->prev->next = new_elem->next;
            if (new_elem->next)
               new_elem->next->prev = new_elem->prev;
            
            if (lruBlock == new_elem)
            {
               lruBlock = new_elem->next;
               lruBlock->prev = 0;
            }
            if (mruBlock == new_elem)
            {
               assert(!"It cannot be!!, mruBlock is elem");
               mruBlock = new_elem->prev;
               mruBlock->next = 0;
            }
            
#if DEBUG_STREAM_DETECTION
            fprintf(stderr, "1) FreeList %p, new_elem %p, elem %p\n", freeList, new_elem, elem);
            fflush(stderr);
#endif
            new_elem->next = freeList;
            freeList = new_elem;
#if DEBUG_STREAM_DETECTION
            fprintf(stderr, "2) FreeList %p, freeList->next %p\n", freeList, freeList->next);
            StreamEntry *iter = freeList;
            fprintf(stderr, "Full FreeList:");
            while (iter) {
               fprintf(stderr, "  %p", iter);
               iter = iter->next;
            }
            fprintf(stderr, "\n");
            fflush(stderr);
            debug = true;
#endif
         }
         new_elem = elem;
#if DEBUG_STREAM_DETECTION
         if (debug)
         {
            fprintf(stderr, "3) FreeList %p, new_elem %p, setLines[elem->addr] %p\n", 
                 freeList, new_elem, (*setLines)[elem->addr]);
            fflush(stderr);
         }
#endif
         return (elem->treeNode);
         
      } else   // location is not in table
         return (0);
   }
   
   void** allocateStream(addrtype _location, int stride, int& conflict)
   {
#if DEBUG_STREAM_DETECTION
      fprintf(stderr, "AllocStream: NumStreams=%d, hash size=%d, hash cap=%d, loc 0x%" PRIxaddr ", stride %d\n",
          num_streams, setLines->size(), setLines->capacity(), _location, stride);
#endif
      StreamEntry*& elem = setLines->insert(_location);
      if (elem)  // conflict with an existing stream
      {
         conflict = 1;
#if DEBUG_STREAM_DETECTION
         fprintf(stderr, "LargeStreamTable, inserting new stream (0x%" PRIxaddr 
                    ",%d) conflicts with existing stream (0x%" PRIxaddr ",%" PRId32 ")\n",
                    _location, stride, elem->addr, elem->stride);
#endif
         // we are going to use this entry, do we do not have to update
         // the location field since that is the same, but update stride
         // and move block to MRU position if not there already
         elem->stride = stride;
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
      } else   // not a conflict, multiple cases possible
      {
         if (num_streams < max_streams)   // cache is not full yet
         {
            assert (setLines->size() < (unsigned)max_streams);
            // elem should be zero at this point; create a new List element
            // assert (!elem);
            elem = &streamEntries[num_streams++];
            // otherwise we have an address conflict, print something??
         } else  // table is full; replace the LRU block
         {
            // number of lines should be greater than zero
            // lruBlock and mruBlock should be non-NULL at this point
            // one more thing; check if there are any elements in the freeList
            if (freeList)  // yay, do not evict LRU
            {
               elem = freeList;
               freeList = freeList->next;
#if DEBUG_STREAM_DETECTION
               fprintf(stderr, "Allocated from freeList %p, new freeList %p\n", elem, freeList);
#endif
            } else
            {
               // no free entries, evict the LRU element
               register addrtype lruLoc = lruBlock->addr;
               setLines->erase (lruLoc);
               elem = lruBlock;
               lruBlock = lruBlock->next;
               lruBlock->prev = NULL;
#if DEBUG_STREAM_DETECTION
               fprintf(stderr, "Allocated the LRU block %p, new LRU %p\n", elem, lruBlock);
#endif
            }
         }
         elem->addr = _location;
         elem->stride = stride;
         elem->next = NULL;
         elem->prev = mruBlock;
         if (mruBlock!=NULL)
            mruBlock->next = elem;
         else
            lruBlock = elem;
         mruBlock = elem;
      }

#if DEBUG_STREAM_DETECTION
      fflush(stderr);
#endif

      // our new stream will be stored in elem; update data for elem and add it to the MRU position
      return (&elem->treeNode);
   }
   
   void dumpToFile (FILE* fd)
   {
      fprintf(fd, "*** Report for LargeStreamTable with %d lines. There are %d used entries. ***\n", 
              max_streams, setLines->size() );
//      StreamTable::dumpToFile (fd);
   }
   
private:
   StreamEntry* lruBlock;
   StreamEntry* mruBlock;
   StreamEntry* freeList;
   HashMapSEP *setLines;
};

class SmallStreamTable : public StreamTable
{
public:
   SmallStreamTable (uint32_t _numLines) : StreamTable (_numLines)
   {
      _fill = 0;
      logicalClock = 0;
      cache = (addrtype*) memalign (16, max_streams * sizeof(addrtype));
      stride = (int*) memalign (16, max_streams * sizeof(int));
      treeNode = (void**) memalign (16, max_streams * sizeof(void*));
      acTime = (uint64_t*) memalign (16, max_streams * sizeof(uint64_t));
   }

   ~SmallStreamTable()
   {
      free (cache);
      free (stride);
      free (acTime);
      free (treeNode);
   }
   
   void* reference (addrtype _location, int& xstride, int& conflict)
   {
      int index;
      ++ logicalClock;
      
      // find if the element is in cache
      // assume that the location was recently accessed (though the real reason
      // for traversing in reverse order is to get negative index if not found)
      for (index=_fill-1 ; index>=0 ; --index)
         if (cache[index] == _location)
            break;
      if (index>=0)   // location is in cache
      {
         // must update the access time for this block
         acTime[index] = logicalClock;
         // must check if current access is store to mark the line as dirty
         cache[index] = cache[index] + stride[index];
         return (treeNode[index]);
      } 
      
      return (0);
   }
   
   void** allocateStream(addrtype _location, int _stride, int& conflict)
   {
      if (_fill < max_streams)   // cache is not full yet
      {
            cache[_fill] = _location;
            acTime[_fill] = logicalClock;
            stride[_fill] = _stride;
            treeNode[_fill] = 0;
            ++ _fill;
            return (&treeNode[_fill-1]);
      } else  
      {
            // this is the worst case;
            // I have to find the lru block and replace it with the
            // new location
            uint64_t minimClock = acTime[0]; 
            // cache must have at least one line
            int index = 0;
            for (int i=1 ; i<_fill ; ++i)
               if (acTime[i]<minimClock)
               {
                  minimClock = acTime[i];
                  index = i;
               }
            cache[index] = _location;
            stride[index] = _stride;
            acTime[index] = logicalClock;
            return (&treeNode[index]);
      }
   }
   
   
   void dumpToFile (FILE* fd)
   {
      fprintf(fd, "*** Report for SmallStreamTable with %d lines. There are %d used entries. ***\n", 
              max_streams, _fill );
//      StreamTable::dumpToFile (fd);
   }
   
private:
   uint64_t logicalClock;
   int _fill;
   // allocate three arrays; second array will keep the state
   // of each cache line in the cache: dirty or not dirty
   addrtype *cache;
   int *stride;
   void  **treeNode;
   uint64_t *acTime;
};

}  /* namespace StreamRD */

#endif
