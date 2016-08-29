/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: history_buffer.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements a buffer of recent memory accesses and the logic to identify
 * new streams based on the recent memory access history.
 */

#ifndef HISTORY_BUFFER_H
#define HISTORY_BUFFER_H

#include <set>
#include "miami_types.h"
#include "stream_table.h"
#include "mrd_splay_tree.h"

namespace StreamRD 
{
using MIAMI::addrtype;

#define NO_STREAM       (-3)
#define REPEAT_ACCESS   (-2)
#define NEW_STREAM      (-1) 

// customize storage type and setters/getters for the strides container
// right now I am using a 32-bit word for positive strides and one 32-bit
// word for negative strides
typedef uint32_t stride_storage;
#define MAX_ALLOWED_STRIDE 32
#define RESET_POS_STRIDES  pstrides=0
#define RESET_NEG_STRIDES  nstrides=0
#define SET_POS_STRIDE(s) pstrides |= (1<<((s)-1))
#define SET_NEG_STRIDE(s) nstrides |= (1<<((s)-1))
#define HAS_POS_STRIDE(x,s) ((x)->pstrides & (1<<((s)-1)))
#define HAS_NEG_STRIDE(x,s) ((x)->nstrides & (1<<((s)-1)))

/* History table entry classes
 */
struct HistoryEntry {
    HistoryEntry *prev;
    HistoryEntry *next;
    addrtype addr;
    stride_storage pstrides;
    stride_storage nstrides;
    int8_t allocated;
};

class CompareHistoryEntries
{
public:
   bool operator () (HistoryEntry* const &he1, HistoryEntry* const &he2) const
   {
      return (he1->addr < he2->addr);
   }
};

class HistoryTable {
private:

    typedef std::multiset<HistoryEntry*, CompareHistoryEntries> History;
    typedef MIAMIU::MRDSplayTree<> MRDTree;
    
    StreamTable *table;
    History history;
    HistoryEntry *historyEntries;
    HistoryEntry *lruEntry, *mruEntry;
    History::iterator *histIters;
    MRDTree sTree;
    HistoryEntry lTemp, uTemp;
    stride_storage pstrides, nstrides;
 
    int num_history;
    int max_history;
    int max_stride;
    bool largeTable;
 
public:
    HistoryTable(int s, int h, int m)  {
        max_history = h;
        max_stride = m;
        num_history = 0;
        lruEntry = mruEntry = 0;
        if (max_stride>MAX_ALLOWED_STRIDE) {
           fprintf(stderr, "Maximum stride of %d is too large. The maximum stride cannot be larger than %d.\n",
                 max_stride, MAX_ALLOWED_STRIDE);
           assert (max_stride <= MAX_ALLOWED_STRIDE);
        }
        
        // allocate table; if size larger than 8, use large table
        // otherwise use small table
        largeTable = false;
        if (s < 8)
           table = new SmallStreamTable(s);
        else
        {
           largeTable = true;
           table = new LargeStreamTable(s);
        }
        
        historyEntries = (HistoryEntry*)malloc(max_history*sizeof(HistoryEntry));
        memset(historyEntries, 0, max_history*sizeof(HistoryEntry));
        histIters = new History::iterator[max_history];
    }
 
    ~HistoryTable() { 
       delete (table);
       free (historyEntries);
       delete[] histIters;
    }
 
    // return stream reuse distance for this accessed location, or
    // a special value indicating a non-stream 
    // value >= 0, it is a hit in the stream table, value is reuse distance
    // value == -2, no stream detected
    // value == -1, newly created stream
    int reference(addrtype baddr, int& xstride, int& conflict) 
    {
//        fprintf(stderr, "Ref %" PRIxaddr ", numH=%d, maxH=%d\n", baddr, num_history, max_history);
        int distance = NO_STREAM;  
        xstride = 0;
        // first, check if we get a hit in the stream table
        void *stream_node = 0;
        if (largeTable)
           stream_node = static_cast<LargeStreamTable*>(table)->reference(baddr, xstride, conflict);
        else
           stream_node = static_cast<SmallStreamTable*>(table)->reference(baddr, xstride, conflict);
        
        if (! stream_node)  // no hit
        {
           // check against recent accesses to detect new streams
           History::iterator lit, uit, hit;
           RESET_POS_STRIDES;
           RESET_NEG_STRIDES;
           lTemp.addr = baddr - max_stride;
           uTemp.addr = baddr + max_stride;
           if (lTemp.addr>baddr)  // underflow
              lTemp.addr = 0;
           if (uTemp.addr<baddr)  // overflow
              uTemp.addr = (addrtype)-1;
           lit = history.lower_bound(&lTemp);
           uit = history.upper_bound(&uTemp);
           int64_t min_abs_match = 0x0fffffff;
           int64_t min_match = min_abs_match;
//           bool repeat_access = false;
           HistoryEntry *repeatElem = 0, *minElem = 0;
           for (hit=lit ; hit!=uit ; ++hit)
           {
              int64_t stride = baddr - (*hit)->addr;
              if (stride > 0)
              {
                 SET_POS_STRIDE(stride);
                 if (HAS_POS_STRIDE((*hit), stride) && stride<min_abs_match)
                 {
                    // found a matching stride
                    min_match = stride;
                    min_abs_match = stride;
                    minElem = *hit;
                 }
              } else
              if (stride < 0)
              {
                 int64_t abs_stride = -stride;
                 SET_NEG_STRIDE(abs_stride);
                 if (HAS_NEG_STRIDE((*hit), abs_stride) && abs_stride<min_abs_match)
                 {
                    // found a matching stride
                    min_match = stride;
                    min_abs_match = abs_stride;
                    minElem = *hit;
                 }
              } else  // stride==0 (same address)
              {
                 // I should only find one repeat entry ...
                 assert(!repeatElem);
                 repeatElem = *hit;
                 distance = REPEAT_ACCESS;
              }
           }  // iterate over closeby elements
           
           // see if the minimum match is valid;
           // if we have a minimum match, create a stream for it
           if (min_abs_match<=max_stride && minElem && !minElem->allocated)  // good one
           {
              void **tree_node = 0;
              if (largeTable)
                 tree_node = static_cast<LargeStreamTable*>(table)->allocateStream(baddr+min_match, 
                          min_match, conflict);
              else
                 tree_node = static_cast<SmallStreamTable*>(table)->allocateStream(baddr+min_match, 
                          min_match, conflict);
              minElem->allocated = 1;
#if DEBUG_STREAM_DETECTION
              fprintf(stderr, "After allocateStream, tree_node=%p\n", *tree_node);
              fflush(stderr);
#endif
              // I need to update the MRD tree
              if (*tree_node == 0) {
                 // table was not full; this is a new allocated entry
#if DEBUG_STREAM_DETECTION
                 fprintf(stderr, "Must call MRD.cold_miss\n");
                 fflush(stderr);
#endif
                 *tree_node = sTree.cold_miss();
#if DEBUG_STREAM_DETECTION
                 fprintf(stderr, "After MRD.cold_miss tree_node=%p\n", *tree_node);
                 fflush(stderr);
#endif
              } else {
                 // table was full, or this was a conflict;
                 // either way, one old entry has been evicted.
                 // we are just going to reuse the block corresponding to the
                 // evicted entry. We do not record the distance, we just do not
                 // increase the tree any further. The tree should have at most 
                 // as many nodes as the stream table 
#if DEBUG_STREAM_DETECTION
                 fprintf(stderr, "Must call MRD.reuse_block(tree_node=%p)\n", *tree_node);
                 fflush(stderr);
#endif
                 sTree.reuse_block(*tree_node);
#if DEBUG_STREAM_DETECTION
                 fprintf(stderr, "After MRD.reuse_block tree_node=%p\n", *tree_node);
                 fflush(stderr);
#endif
              }
              xstride = min_match;
              distance = NEW_STREAM;
//              return (NEW_STREAM);
           }
        
           // in all cases (stream hit or not), I have to add the access to the history
           // buffer. I need to keep track of the LRU entry and its set iterator, brrr ...
           // I am not going to add the location to the history table if it was a table hit,
           // or if it is a repeat access to a location already in the history buffer
           // If repeat_access, then update the old entry
           HistoryEntry *elem = 0;
           int eIndex = -1;
//           if (repeat_access)
//              return (REPEAT_ACCESS);
           
           if (repeatElem)
           {
              elem = repeatElem;
              // remove elem from the lru-mru list
              if (elem->prev)
                 elem->prev->next = elem->next;
              else  // this must have been the LRU entry, assert
              {
                 assert(elem==lruEntry);
                 lruEntry = elem->next;
              }
              if (elem->next)
                 elem->next->prev = elem->prev;
              else  // this must have been the MRU entry, assert
              {
                 assert(elem==mruEntry);
                 mruEntry = elem->prev;
              }
              // set the MRU
              // list is not empty and we are not MRU; there must be another entry which is MRU
              elem->next = NULL;
              elem->prev = mruEntry;
              if (mruEntry!=NULL)
                 mruEntry->next = elem;
              else
                 lruEntry = elem;
              mruEntry = elem;
              
              // no need to delete from the tree in this case
              // address stays the same
//              distance = REPEAT_ACCESS;
           }
           else
           if (num_history < max_history)   // buffer is not full yet
           {
              assert (history.size() < (unsigned)max_history);
              // elem should be zero at this point; create a new History element
              eIndex = num_history++;
              elem = &historyEntries[eIndex];
              elem->addr = baddr;
//              elem->allocated = 0;
              elem->next = NULL;
              elem->prev = mruEntry;
              if (mruEntry!=NULL)
                 mruEntry->next = elem;
              else
                 lruEntry = elem;
              mruEntry = elem;
              histIters[eIndex] = history.insert(elem);
           } else     // table is full; replace the LRU block
           {
              // history size should be greater than zero
              // lruEntry and mruEntry should be non-NULL at this point
              // pointer arithmetic; this should provide the index, right?
              // I do not have to divide by sizeof(HistoryEntry) ...
              elem = lruEntry;
              lruEntry = elem->next;
              lruEntry->prev = NULL;
              
              eIndex = elem - historyEntries;  
              assert (eIndex>=0 && eIndex<num_history &&
                      histIters[eIndex] != history.end());
              history.erase(histIters[eIndex]);
              
              elem->prev = mruEntry;
              mruEntry->next = elem;
              mruEntry = elem;
              elem->next = NULL;
              elem->addr = baddr;
//              elem->allocated = 0;
              histIters[eIndex] = history.insert(elem);
           }
           elem->allocated = 0;
           elem->pstrides = pstrides;
           elem->nstrides = nstrides;
        } else  // it is a stream hit
        {
           // compute and store the stream reuse distance
#if DEBUG_STREAM_DETECTION
           fprintf(stderr, "-> Calling sTree.reuse_block(stream_node=%p)\n", stream_node);
           fflush(stderr);
#endif
           distance = sTree.reuse_block(stream_node);
#if DEBUG_STREAM_DETECTION
           fprintf(stderr, "<- Back from sTree.reuse_block, distance=%d\n", distance);
           fflush(stderr);
#endif
           // do not add the address to the history table in case of a table hit
        }
        return distance;
    }
};

}  /* namespace StreamRD */
 
#endif
 