/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: stream_reuse.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements thread level data management and the tool specific logic for a 
 * streaming concurrency tool.
 */

#include "miami_types.h"
  
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <vector>
#include <algorithm>

#include "bucket_hashmap.h"
#include "dense_container.h"
#include "self_sorted_list.h"
#include "generic_pair.h"
#include "cache_sim.h"
#include "history_buffer.h"
#include "stream_reuse.h"
#include "load_module.h"

namespace StreamRD
{
   typedef uint64_t count_type;
   #define PRIcount PRIu64
   
   MIAMI::LoadModule **allImgs;
   uint32_t maxImgs = 0;
   uint32_t loadedImgs = 0;
   int verbose_level = 0;

   bool noStreamSim = false;
   int line_shift = 0;
   
   StreamEntry *defStreamEntryPtr = 0;
   
   uint32_t stream_table_size=0, 
            stream_history_size=0, 
            stream_max_stride=0;
   uint32_t stream_cache_size=0, 
            stream_line_size=0, 
            stream_cache_assoc=0;

//   uint32_t g_numDistinctInstructions = 0;
   count_type g_libraryCalls = 0;
   count_type g_tot_accesses = 0;
   count_type g_tot_misses = 0;
   count_type g_tot_newStreams = 0;
   count_type g_tot_streamReuse = 0;
   
   count_type defCountType = 0;
   typedef MIAMIU::BucketHashMap<uint32_t, count_type, &defCountType, 16> HashMapCT;

   typedef MIAMIU::GenericPair<unsigned int, count_type> CTPair;
   typedef std::vector<CTPair> CTPairArray;

   class InstInfo
   {
   public:
      class StrideInfo
      {
      public:
         StrideInfo(int _stride)
         {
            reuseHist = 0;
            newStream = 0;
            newConflict = 0;
            hitConflict = 0;
            stride = _stride;
         }
         ~StrideInfo() {
            if (reuseHist)
               delete (reuseHist);
         }
         
         inline bool operator==(const StrideInfo& si) const
         {
            return (stride == si.stride);
         }
         inline bool operator!=(const StrideInfo& si) const
         {
            return (stride != si.stride);
         }
         
         HashMapCT *reuseHist;
         count_type newStream;
         count_type newConflict;
         count_type hitConflict;
         int stride;
      };
      typedef MIAMIU::SelfSortedList<StrideInfo, 4> StrideInfoList;
      
      InstInfo () {
         cacheHits = 0;
         noStream = 0;
         repeatAccess = 0;
         siList = new StrideInfoList();
      }
      ~InstInfo() {
         if (siList)
            delete (siList);
      }
   
      count_type cacheHits;
      count_type noStream;
      count_type repeatAccess;
      StrideInfoList *siList;
   };
   typedef MIAMIU::DenseContainer<InstInfo, 128> IIContainer;

   // I must aggregate results at instruction (and maybe scope level). However, those 
   // IDs are unique inside each image only, so I will define a structure to hold 
   // information about each image.
   // I do not know the number of images a priori, so I need a data structure 
   // with fast random access that also grows.
   typedef struct _image_info
   {
      IIContainer instData;
   } ImageInfo;
   typedef MIAMIU::DenseContainer<ImageInfo, 4> ImgContainer;

   
   class ThreadLocalData
   {
   public:
      ThreadLocalData(int _tid) {
         tid = _tid;
         lastImgId = -1;
         imgInfo = 0;

         numDistinctInstructions = 0;
         libraryCalls = 0;
         tot_accesses = 0;
         tot_misses = 0;
         tot_newStreams = 0;
         tot_streamReuse = 0;
         
         if (noStreamSim)
            aHistory = 0;
         else
            aHistory = new HistoryTable ( stream_table_size,
                                          stream_history_size,
                                          stream_max_stride);
         
         cache = new MIAMIU::CacheSim(stream_cache_size,
                         stream_line_size,
                         stream_cache_assoc,
                         false);
         
         // Calls to ThreadStart are serialized by PIN; this is safe (w/ PIN)
         next = lastTLD;
         lastTLD = this;
      }
      ~ThreadLocalData() { 
         if (cache)
            delete (cache);
         if (aHistory)
            delete (aHistory);
      }
      
      inline int ProcessEvents(uint64_t numElems, struct MemRefRecord *ref);
      void OutputResultsToFile(FILE *fout);

   private:
      int tid;   // thread ID; I create one object for each thread
      
      // Below are the data structures needed during processing
      HistoryTable *aHistory;  // access history table
      MIAMIU::CacheSim *cache;
      
      // I also need data structures to aggregate the results at instruction and 
      // at the scope level respectively. However, instruction and scope IDs make
      // sense only with a corresponding Image ID, so they must be indexed by both 
      // the image ID and the instruction / scope IDs. Hmmm.
      ImgContainer imgData;
      ImageInfo *imgInfo;
      int32_t lastImgId;

      // keep also a bunch of counters for the different types of events
      uint32_t numDistinctInstructions;
      
      count_type libraryCalls;  // how many times the event processing routine was called
      count_type tot_accesses;
      count_type tot_misses;
      count_type tot_newStreams;
      count_type tot_streamReuse;

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
      
      /* Dump results to file */
      if (fout)
         tld->OutputResultsToFile(fout);
      
      delete (tld);
   }

   void PrintGlobalStatistics()
   {
      // Print summary statistics for program
      const char* prefix = "MIAMI_SIMULATION_SUMMARY";
      fprintf(stderr, "==============================================================================\n");
      fprintf(stderr, "%s: Number of library calls = %" PRIcount "\n", prefix, g_libraryCalls);
      fprintf(stderr, "%s: Number of memory accesses = %" PRIcount "\n", prefix, g_tot_accesses);
      fprintf(stderr, "%s: Number of cache misses = %" PRIcount "\n", prefix, g_tot_misses);
      fprintf(stderr, "%s: Number of streams created = %" PRIcount "\n", prefix, g_tot_newStreams);
      fprintf(stderr, "%s: Number of stream advances = %" PRIcount "\n", prefix, g_tot_streamReuse);
      fprintf(stderr, "==============================================================================\n");
   }

   int  
   ProcessBufferedElements(uint64_t numElems, struct MemRefRecord *ref, void *tdata)
   {
      ThreadLocalData *tld = static_cast<ThreadLocalData*>(tdata);
      return (tld->ProcessEvents(numElems, ref));
   }

   inline int
   ThreadLocalData::ProcessEvents(uint64_t numElems, struct MemRefRecord *ref)
   {
      unsigned int i;
      libraryCalls += 1;
      
      for (i=0 ; i<numElems ; ++i, ++ref)
      {
         if (ref->imgId != lastImgId)
         {
            lastImgId = ref->imgId;
            imgInfo = &(imgData[ref->imgId]);
         }

         InstInfo* iiref = &(imgInfo->instData[ref->iidx]);
         
         // first perform the cache simulation
         register addrtype addr = ref->ea >> line_shift;
         const MIAMIU::CACHE_AccessType result = cache->reference(addr, 0);
         if (result == MIAMIU::CACHE_HIT)
         {
            iiref->cacheHits += 1;
            continue;
         }
         
         if (noStreamSim)
            continue;
          
         // otherwise, it is a cache miss; do the stream reuse analysis
#if DEBUG_STREAM_DETECTION
         fprintf(stderr, "IM 0x%" PRIxaddr ", index %d ", addr, i);
         fflush(stderr);
#endif
         int conflict = 0, xstride = 0;
         int distance = aHistory->reference(addr, xstride, conflict);
#if DEBUG_STREAM_DETECTION
         fprintf(stderr, " -> dist=%d\n", distance);
         fflush(stderr);
#endif

         switch(distance)
         {
            case NO_STREAM:
               iiref->noStream += 1;
               break;
                
            case REPEAT_ACCESS:
               iiref->repeatAccess += 1;
               break;
               
            default:
            {
               // find the StrideInfo object for this xstride
               InstInfo::StrideInfo sinfo(xstride);
               InstInfo::StrideInfo &si_obj = iiref->siList->insert(sinfo);
               if (distance == NEW_STREAM)
               {
                  si_obj.newStream += 1;
                  si_obj.newConflict += conflict;
               } else
               {
                  assert(distance >= 0);
                  HashMapCT* &tii = si_obj.reuseHist;
                  if (tii == NULL)
                     tii = new HashMapCT();
                  tii->insert(distance) += 1;
                  si_obj.hitConflict += conflict;
               }
               break;
            }
         }
      }
      return (0);
   }  // ProcessEvents



   static int
   placeKeysInArray(void* arg0, addrtype key, void* value)
   {
      addrtype *addrArray = (addrtype*)arg0;
      int32_t intVal = *((int32_t*)value);
      addrArray[intVal] = key;
      return (0);
   }

   static int 
   processBinToVector(void* arg0, unsigned int key, void* value)
   {
      CTPairArray* histogram = (CTPairArray*)arg0;
      count_type* pValue = (count_type*)value;
      histogram->push_back(CTPair(key, *pValue) );
      return 0;
   }


   void 
   ThreadLocalData::OutputResultsToFile(FILE *fd)
   {
      // First of all, print the simulation parameters: line size, max stride, table size, ...
      fprintf(fd, "# HistorySize, TableSize, LineSize, MaxStride(lines), CacheSize, Associativity\n");
      fprintf(fd, "%u %u %u %u %u %u\n",
               stream_history_size, stream_table_size,
               stream_line_size, stream_max_stride,
               stream_cache_size>>10, stream_cache_assoc);

      // next print the format rows for the rest of the file
      // we have lines with different formats
      fprintf(fd, "# Img_ID, Img_BaseAddr, Img_NumInst, Img_Name\n");
      fprintf(fd, "# stride, NewStream, StreamReuse, NumBins, Dist1, Count1\n");
      fprintf(fd, "# 0, PC_offset, memop, InstTotFreq, CacheMisses, NoStream, RepeatAccess\n");

      CTPairArray histogram;  // = new StreamRD::CTPairArray();
      // go through each image and output the stats for each reference
      ImgContainer::const_iterator cit = imgData.first();
      while ((bool)cit)
      {
         int idx = cit.Index();
         
         MIAMI::LoadModule *myImg = allImgs[idx];
         assert (myImg);
         
         // For each image, we have information about the instructions,
         // but we have numeric indices.
         // Must convert indices back into program addresses.
         // A thread can finish at any time, but other threads may continue
         // to generate new indices; I need to compute the reverse map
         // every time a thread ends.
         // I implemented this logic in the LoadModule class. We compute the
         // reverse mapping on demand, only if it is stale.
            
         // for each image, print first its ID, base address, number of instructions and its name
         numDistinctInstructions += cit->instData.size();
         fprintf(fd, "%" PRIu32 " 0x%" PRIxaddr " %ld %s\n", 
                     idx, myImg->BaseAddr(), cit->instData.size(), myImg->Name().c_str());
         fflush(fd);
         
         addrtype iaddr;
         int memop;
         IIContainer::const_iterator icit = cit->instData.first();
         while ((bool)icit)
         {
            int iindex = icit.Index();    // the instruction index
            iaddr = myImg->getAddressForInstIndex(iindex, memop);

            const InstInfo *vit = (const InstInfo*)icit;
            count_type miss_total = 0;
            
            if (vit->siList)
            {
               InstInfo::StrideInfoList::iterator sit = vit->siList->first();
               while ((bool) sit)
               {
                  // start by printing the stride and the number of new streams for this stride
                  fprintf(fd, "%d %" PRIu64, sit->stride, sit->newStream);
                  miss_total += sit->newStream;
                  tot_newStreams += sit->newStream;
                  // print the instr address and memop only for the generic entry with stride of 0 
                  if (sit->reuseHist)
                  {
                     // Now, we have to get the values from the reuse distance histogram
                     histogram.clear();
                     sit->reuseHist->map(processBinToVector, &histogram);
                     // The complete histogram of the instruction must be in the histogram array
                     // Sort it and process it
                     std::sort(histogram.begin(), histogram.end(), CTPair::OrderAscByKey1());
                  
                     // compute total number of streams
                     count_type totStreams = 0;
                     CTPairArray::iterator it = histogram.begin();
                     for ( ; it!=histogram.end() ; ++it)
                     {
                        totStreams += it->second;
                     }
                     miss_total += totStreams;
                     tot_streamReuse += totStreams;
               
                     // now print the total number of streams and the histogram
                     fprintf (fd, " %" PRIu64 " %ld", totStreams, histogram.size());
                     for (it=histogram.begin() ; it!=histogram.end() ; ++it)
                     {
                        fprintf (fd, " %u %" PRIu64, it->first, it->second);
                     }
                     fprintf (fd, "\n");
                  } else
                     fprintf (fd, " 0 0\n");
                  ++ sit;
               }  // iterate over all found strides for this instruction
            }  // if we have any stream data
            // at the end, print one line with the stride-independent instruction info
            miss_total += (vit->noStream + vit->repeatAccess);
            count_type inst_total = vit->cacheHits + miss_total;
            
            fprintf (fd, "0 0x%" PRIxaddr " %d %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 "\n",
                  iaddr, memop, inst_total, miss_total, vit->noStream, vit->repeatAccess);
            fflush (fd);
            tot_accesses += inst_total;
            tot_misses += miss_total;

            fflush(fd);
            ++ icit;
         }
        
         ++ cit;
      }  // Iterate over images

      // update the global counts of relevant events
      g_libraryCalls += libraryCalls;
      g_tot_accesses += tot_accesses;
      g_tot_misses += tot_misses;
      g_tot_newStreams += tot_newStreams;
      g_tot_streamReuse += tot_streamReuse;
      // I cannot determine the number of distinct instructions across threads
//      g_numDistinctInstructions += numDistinctInstructions;
      
      // Print summary statistics for thread
      const char* prefix = "MIAMI_THREAD_SUMMARY:";
      fprintf(stderr, "%s T%d: Distinct references executed = %" PRIu32 "\n", prefix, tid, numDistinctInstructions);
      fprintf(stderr, "%s T%d: Number of library calls = %" PRIcount "\n", prefix, tid, libraryCalls);
      fprintf(stderr, "%s T%d: Number of memory accesses = %" PRIcount "\n", prefix, tid, tot_accesses);
      fprintf(stderr, "%s T%d: Number of cache misses = %" PRIcount "\n", prefix, tid, tot_misses);
      fprintf(stderr, "%s T%d: Number of streams created = %" PRIcount "\n", prefix, tid, tot_newStreams);
      fprintf(stderr, "%s T%d: Number of stream advances = %" PRIcount "\n", prefix, tid, tot_streamReuse);
  }


};  /* namespace StreamRD */
