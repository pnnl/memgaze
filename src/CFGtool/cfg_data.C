/*
 * This file is part of the MIAMI framework. For copyright information, see    
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: cfg_data.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements the data management and the tool specific logic for a CFG
 * profiler tool.
 */

#include "miami_types.h"
  
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <vector>
#include <algorithm>
#include <string>

#include "cfg_data.h"
#include "load_module.h"
#include "routine.h"

using MIAMI::addrtype;

namespace MIAMI_CFG
{
   MIAMI::LoadModule **allImgs;
   uint32_t maxImgs = 0;
   uint32_t loadedImgs = 0;
   int verbose_level = 0;
   bool save_bbls = false;
   bool print_stats = false;
   std::string debug_rtn;
   // initial number of counters supported by each thread
   // this global variable should be modified or used to allocate memory
   // only while holding the cfgLock lock
   int32_t thr_buf_size = INITIAL_BUF_SIZE;

   MiamiCfgStats g_stats;  // global stats; accumulate thread stats in here
   
   class ThreadLocalData
   {
   public:
      ThreadLocalData(int _tid) {
         tid = _tid;

         lastImgId = -1;
         myimg = 0;
         
         saved_buf_size = thr_buf_size;
         thr_counters = (count_type*)malloc(saved_buf_size * sizeof(count_type));
         if (! thr_counters)
            fprintf(stderr, "Error while allocating local buffer of %d elements for thread %d. Must Abort.\n",
                  saved_buf_size, tid);
         for (int i=0 ; i<saved_buf_size ; ++i)
            thr_counters[i] = 0;

         // Calls to ThreadStart are serialized by PIN; this is safe (w/ PIN)
         next = lastTLD;
         lastTLD = this;
      }
      ~ThreadLocalData() { 
         if (thr_counters)
            free(thr_counters);
      }
      
      inline int ProcessEvents(uint64_t numElems, struct BufRecord *ref);
      void OutputResultsToFile(FILE *fout);

      void DumpRoutineCFG(const char* rank_id);
      
      inline count_type* GetLocalBuffer() { return (thr_counters); }
      inline count_type* IncreaseLocalBuffer() 
      {
         // the new size is taken from the global variable thr_buf_size
         int32_t old_size = saved_buf_size;
         saved_buf_size = thr_buf_size;
         // I can also do local buffer increases when I process indirect branches, 
         // so check that we are actually increasing the buffer size.
         if (old_size < saved_buf_size)
         {
            thr_counters = (count_type*)realloc(thr_counters, saved_buf_size * sizeof(count_type));
            if (! thr_counters) {
               fprintf(stderr, "Error while increasing buffer size to %d elements for thread %d. Must Abort.\n",
                     saved_buf_size, tid);
               exit(-1);
            }
            for (int i=old_size ; i<saved_buf_size ; ++i)
               thr_counters[i] = 0;
         }
         return (thr_counters);
      }

      inline void RecordBufferIncrease(int32_t val) { stats.buffer_resizes += val; }

   private:
      int tid;   // thread ID; I create one object for each thread
      // All counters are stored in dense arays, one array per thread
      count_type *thr_counters;
      int32_t saved_buf_size;
      
      // keep also a bunch of counters for the different types of events
      MiamiCfgStats stats;

      MIAMI::LoadModule *myimg;
      int32_t lastImgId;

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

   count_type* GetThreadCountersBuffer(void *tdata)
   {
      ThreadLocalData *tld = static_cast<ThreadLocalData*>(tdata);
      return (tld->GetLocalBuffer());
   }
   
   count_type* IncreaseThreadCountersBuffer(void *tdata)
   {
      ThreadLocalData *tld = static_cast<ThreadLocalData*>(tdata);
      return (tld->IncreaseLocalBuffer());
   }
   
   void RecordBufferIncrease(void *tdata, int32_t val)
   {
      ThreadLocalData *tld = static_cast<ThreadLocalData*>(tdata);
      tld->RecordBufferIncrease(val);
   }
   
   void FinalizeThreadData(void *tdata, const char *fname1, const char* rank_id)
   {
      ThreadLocalData *tld = static_cast<ThreadLocalData*>(tdata);
      
      FILE *fout = 0;
      /* Dump cache statistics to first file */
      fout = fopen (fname1, "wb");
      if (!fout)
      {
         fprintf (stderr, "MIAMI::cfgtool::FinalizeThreadData: ERROR, cannot open output file %s. Profile data will not be saved.\n", fname1);
      } else
      {
         tld->OutputResultsToFile(fout);
         fclose(fout);
      }

      if (debug_rtn.length()>0)
      {
         tld->DumpRoutineCFG(rank_id);
      }
      
      delete (tld);
   }

   void PrintGlobalStatistics()
   {
      // Print summary statistics for program
      const char* prefix = "MIAMI_SIMULATION_SUMMARY";
      fprintf(stderr, "==============================================================================\n");
      fprintf(stderr, "%s: Number of buffer resizes = %" PRIcount "\n", prefix, g_stats.buffer_resizes);
      fprintf(stderr, "%s: Largest counter index = %d\n", prefix, MIAMI::LoadModule::GetHighWaterMarkIndex());
      fprintf(stderr, "%s: Counters buffer size = %d\n", prefix, thr_buf_size);
      fprintf(stderr, "%s: Number of total increments = %" PRIcount "\n", prefix, g_stats.tot_increments);
      fprintf(stderr, "%s: Number of indirect increments = %" PRIcount "\n", prefix, g_stats.ind_increments);
//      fprintf(stderr, "%s:  - distinct routine entries = %" PRIcount "\n", prefix, g_stats.routine_entries);
//      fprintf(stderr, "%s:  - distinct trace counters = %" PRIcount "\n", prefix, g_stats.trace_counters);
//      fprintf(stderr, "%s:  - distinct edge counters = %" PRIcount "\n", prefix, g_stats.edge_counters);
      fprintf(stderr, "==============================================================================\n");
      fflush(stderr);
   }

   int  
   ProcessBufferedElements(uint64_t numElems, struct BufRecord *ref, void *tdata)
   {
      ThreadLocalData *tld = static_cast<ThreadLocalData*>(tdata);
      return (tld->ProcessEvents(numElems, ref));
   }

   inline int
   ThreadLocalData::ProcessEvents(uint64_t numElems, struct BufRecord *ref)
   {
      unsigned int i;
      stats.ind_increments += numElems;
      
      i = 0;
      for (i=0 ; i<numElems ; ++i, ++ref)
      {
         int32_t imgId = ref->imgId;
         if (imgId != lastImgId)
         {
            lastImgId = imgId;
            myimg = allImgs[imgId];
         }
         int32_t& edge_index = myimg->getEdgeIndexForInstAndTarget(ref->iidx,
                                  ref->target);
         if (edge_index < 0)  // first time we see this combination
         {
            int32_t iidx = ref->iidx;
            // I need to do the reverse mapping and get the PC (block end address)
            // out of the index stored in the buffer
            addrtype end_addr = myimg->GetPCForIndex(iidx);
            if (end_addr == 0)  // we did not find a record for the index???
            {
               fprintf(stderr, "ERROR: ThreadLocalData::ProcessEvents: Image %d, cannot find a mapped PC for index %d. Not good!\n",
                           imgId, iidx);
               continue;
            }
            
            MIAMI::Routine* robj = myimg->HasRoutineAtPC(end_addr - 1);
            if (robj == 0)  // this can happen when the library is partially stripped
            {
               robj = myimg->RoutineAtIndirectPc(end_addr - 1);
               if (robj == 0)  // this should never happen
               {
                  fprintf(stderr, "ERROR: ThreadLocalData::ProcessEvents: Image %d, index=%d, PC=0x%" PRIxaddr 
                              " cannot find any routine for pc-1. How can it be??\n",
                              imgId, iidx, end_addr);
                  continue;
               }
            }
            MIAMI::CFG::Edge* ee = static_cast<MIAMI::CFG::Edge*>(robj->AddPotentialEdge(end_addr, 
                      ref->target, MIAMI::PrivateCFG::MIAMI_INDIRECT_BRANCH_EDGE));
            int32_t index = myimg->GetCFGEdgeIndex(ee);
            
            // check if we have to increase the local buffer
            if (index >= saved_buf_size)
            {
               int32_t old_size = saved_buf_size;
               while (index >= saved_buf_size) {
                  saved_buf_size <<= 1;
                  stats.buffer_resizes += 1;
               }
               if (MIAMI_CFG::verbose_level > 3)
               {
                  fprintf(stderr, "Thread %d must grow its own buffer from %d to %d while processing indirect branches.\n",
                              tid, old_size, saved_buf_size);
               }
                  
               thr_counters = (count_type*)realloc(thr_counters, saved_buf_size * sizeof(count_type));
               if (! thr_counters) {
                  fprintf(stderr, "Error while increasing buffer size to %d elements for thread %d. Must Abort.\n",
                        saved_buf_size, tid);
                  exit(-1);
               }
               for (int j=old_size ; j<saved_buf_size ; ++j)
                  thr_counters[j] = 0;
            }
            edge_index = index;
         }
         thr_counters[edge_index] += 1;
      }
      
      return (0);
   }  // ProcessEvents

   void 
   ThreadLocalData::OutputResultsToFile(FILE *fcfg)
   {
      uint32_t i, nImages = 0;
      for (i=0 ; i<maxImgs ; ++i)
         if (allImgs[i])
         {
            MIAMI::LoadModule *myImg = allImgs[i];
            nImages++;
            myImg->setFileOffset(ftell(fcfg));  // save offset where we start writing data for this image
            myImg->SaveCFGsToFile(fcfg, thr_counters, stats);
         }
         
      // at the end of the file, save an index table with the file offset for each image
      // I will save the number of images and then for each image save 
      // the image name (in length prefix format) + file offset where it starts
      // Finally, the last 8 bytes store the file offset where the table begins
      uint64_t tableOffset = ftell(fcfg);
      fwrite(&nImages, 4, 1, fcfg);
      for (i=0 ; i<maxImgs ; ++i)
         if (allImgs[i])
         {
            MIAMI::LoadModule *myImg = allImgs[i];
            uint32_t len = myImg->Name().length();
            fwrite(&len, 4, 1, fcfg);
            fwrite(myImg->Name().c_str(), 1, len, fcfg);
            uint64_t fof = myImg->FileOffset();
            fwrite(&fof, 8, 1, fcfg);
         }
      // finally, save the offset where the table starts
      fwrite(&tableOffset, 8, 1, fcfg);

      if (print_stats)
      {
         // update the global counts of relevant events
         g_stats.buffer_resizes += stats.buffer_resizes;
         g_stats.tot_increments += stats.tot_increments;
         g_stats.ind_increments += stats.ind_increments;
         // I cannot determine the number of distinct counters across threads
//         g_stats.routine_entries += stats.routine_entries;
//         g_stats.trace_counters += stats.trace_counters;
//         g_stats.edge_counters += stats.edge_counters;
         
         // Print summary statistics for thread
         const char* prefix = "MIAMI_THREAD_SUMMARY:";
         fprintf(stderr, "%s T%d: Number of buffer resizes = %" PRIcount "\n", prefix, tid, stats.buffer_resizes);
         fprintf(stderr, "%s T%d: Number of indirect increments = %" PRIcount "\n", prefix, tid, stats.ind_increments);
         fprintf(stderr, "%s T%d: Number of total increments = %" PRIcount "\n", prefix, tid, stats.tot_increments);
         fprintf(stderr, "%s T%d:  - distinct routine entries = %" PRIcount "\n", prefix, tid, stats.routine_entries);
         fprintf(stderr, "%s T%d:  - distinct trace counters = %" PRIcount "\n", prefix, tid, stats.trace_counters);
         fprintf(stderr, "%s T%d:  - distinct edge counters = %" PRIcount "\n", prefix, tid, stats.edge_counters);
         fflush(stderr);
      }
   }

   void 
   ThreadLocalData::DumpRoutineCFG(const char* rank_id)
   {
      uint32_t i;
      for (i=0 ; i<maxImgs ; ++i)
         if (allImgs[i])
         {
            MIAMI::LoadModule *myImg = allImgs[i];
            myImg->dumpRoutineCFG(debug_rtn, thr_counters, rank_id);
         }
   }

};  /* namespace MIAMI_CFG */
