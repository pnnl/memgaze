/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: cache_sim.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements a typical cache simulator using an LRU replacement
 * policy and different data structures for small and large cache sets.
 */  

#include "cache_sim.h"
#include "miami_utils.h"

namespace MIAMIU
{
   using MIAMI::addrtype;
   
   const char *accessTypeName[NUM_ACCESS_TYPES] = { "InvalidAccess", "CacheHits", "CapacityMisses",
                 "ConflictMisses", "ConflictHits" };
   SimpleList *defSimpleListPtr = 0;

   void 
   CacheSet::dumpToFile (FILE* fd)
   {
      uint64_t totalCount = 0;
      for (int i=0 ; i<NUM_ACCESS_TYPES ; ++i)
      {
         totalCount += accessCount[i];
         fprintf(fd, "%s: %" PRIu64 "\n", accessTypeName[i], accessCount[i]);
      }
      fprintf (fd, "TotalCount: %" PRIu64 "\n", totalCount);
      fprintf (fd, "LinesWrittenBack: %" PRIu64 "\n", written_back_lines);
   }

   CacheSim::CacheSim(uint32_t _size, uint32_t _line, uint32_t _assoc,
                bool compute_conflict_misses)
   {
      uint32_t i;
      numLines = _size / _line;
      size = _size;
      assocLevel = _assoc;
      lineSize = _line;
      lineShift = MIAMIU::FloorLog2 (_line);
      largeSet = largeAssoc = false;

      if (_assoc==0 || _assoc>=numLines)   // target cache is fully-assoc
      {
         setAssoc = 0;
         numSets = 1;
         setMask = 0;
      } else
      {
         numSets = numLines / _assoc;
         setMask = numSets - 1;
         assert (numSets * _assoc == numLines);
         setAssoc = new CacheSet* [numSets];
         if (_assoc < 8)
         {
            for (i=0 ; i<numSets ; ++i)
               setAssoc [i] = new SmallCacheSet (_assoc);
         }
         else
         {
            largeSet = true;
            for (i=0 ; i<numSets ; ++i)
               setAssoc [i] = new LargeCacheSet (_assoc);
         }
      }
      
      if (setAssoc==0 || compute_conflict_misses)
      {
         if (numLines < 8)
            fullyAssoc = new SmallCacheSet (numLines);
         else
         {
            largeAssoc = true;
            fullyAssoc = new LargeCacheSet (numLines);
         }
      } else
         fullyAssoc = 0;
   }
   
   CacheSim::~CacheSim()
   {
      uint32_t i;
      if (fullyAssoc)
         delete fullyAssoc;
      if (setAssoc)
      {
         for (i=0 ; i<numSets ; ++i)
            delete setAssoc[i];
         delete[] setAssoc;
      }
   }

   void 
   CacheSim::dumpToFile (FILE* fd, bool detailed)
   {
      uint32_t ii, jj;
      if (fullyAssoc && (!setAssoc || detailed))
      {
         fprintf (fd, "\n====================================================================\n");
         fprintf (fd, "Fully-associative cache with %u lines of %u bytes:\n", numLines, lineSize);
         fprintf (fd, "--------------------------------------------------------------------\n");
         if (largeAssoc)
            static_cast<LargeCacheSet*>(fullyAssoc)->dumpToFile (fd);
         else
            static_cast<SmallCacheSet*>(fullyAssoc)->dumpToFile (fd);
      }
      if (setAssoc)
      {
         fprintf (fd, "\n====================================================================\n");
         fprintf (fd, "%u way set-associative cache with %u lines of %u bytes organized in %u sets:\n",
                 assocLevel, numLines, lineSize, numSets);
         fprintf (fd, "--------------------------------------------------------------------\n");
         uint64_t assocCounts[NUM_ACCESS_TYPES];
         uint64_t written_back_lines = 0;
         for (jj=0 ; jj<NUM_ACCESS_TYPES ; ++jj)
            assocCounts [jj] = 0;
         for (ii=0 ; ii<numSets ; ++ii)
         {
            written_back_lines += setAssoc[ii]->LinesWrittenBack();
            for (jj=0 ; jj<NUM_ACCESS_TYPES ; ++jj)
               assocCounts[jj] += setAssoc[ii]->countOfType (jj);
         }
         uint64_t totalCount = 0;
         for (jj=0 ; jj<NUM_ACCESS_TYPES ; ++jj)
         {
            totalCount += assocCounts[jj];
            fprintf (fd, "%s: %" PRIu64 "\n", accessTypeName[jj], assocCounts[jj]);
         }
         fprintf (fd, "TotalCount: %" PRIu64 "\n", totalCount);
         fprintf (fd, "LinesWrittenBack: %" PRIu64 "\n", written_back_lines);
    
         if (detailed)
            for (ii=0 ; ii<numSets ; ++ii)
            {
               fprintf (fd, "\n--------------------------------------------------------------------\n");
               fprintf(fd, "Set number %d:\n", ii);
               fprintf (fd, "--------------------------------------------------------------------\n");
               if (largeSet)
                  static_cast<LargeCacheSet*>(setAssoc[ii])->dumpToFile (fd);
               else
                  static_cast<SmallCacheSet*>(setAssoc[ii])->dumpToFile (fd);
            }
      }
   }
   
   int
   CacheSim::getTotalCounts(uint64_t *counters, uint64_t& lines_written_back)
   {
      unsigned int ii, jj;
      for (jj=0 ; jj<NUM_ACCESS_TYPES ; ++jj)
         counters[jj] = 0;
      lines_written_back = 0;
      
      if (setAssoc)
      {
         for (ii=0 ; ii<numSets ; ++ii)
         {
            lines_written_back += setAssoc[ii]->LinesWrittenBack();
            for (jj=0 ; jj<NUM_ACCESS_TYPES ; ++jj)
               counters[jj] += setAssoc[ii]->countOfType(jj);
         }
      } else
      {
         assert(fullyAssoc);
         lines_written_back = fullyAssoc->LinesWrittenBack();
         for (jj=0 ; jj<NUM_ACCESS_TYPES ; ++jj)
            counters[jj] = fullyAssoc->countOfType(jj);
      }
      return (0);
   }

}  /* namespace MIAMIU */

