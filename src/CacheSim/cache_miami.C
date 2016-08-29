/*
 * This file is part of the MIAMI framework. For copyright information, see    
 * the LICENSE file in the MIAMI root folder.  
 */
/*
 * File: cache_miami.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements the data management and the tool specific logic of a cache 
 * simulator tool.
 */

#include "miami_types.h"
  
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <vector>
#include <algorithm>

#include "dense_container.h"
#include "cache_sim.h"
#include "cache_miami.h"
#include "load_module.h"
#include "hpcfile_hpcrun.h"
#include "io.h"

using MIAMI::addrtype;

namespace MIAMIU 
{
   extern const char *accessTypeName[MIAMIU::NUM_ACCESS_TYPES];
}

namespace MIAMI_CACHE_SIM
{
   typedef uint64_t count_type;
   #define PRIcount PRIu64
   
   MIAMI::LoadModule **allImgs;
   uint32_t maxImgs = 0;
   uint32_t loadedImgs = 0;
   int verbose_level = 0;

   int line_shift = 0;
   
   bool do_conflicts = true;
   bool track_refs = false, long_dump = false;
   uint32_t cache_size=0, 
            line_size=0, 
            cache_assoc=0;
   
   count_type g_libraryCalls = 0;
   count_type g_tot_accesses = 0;
   count_type g_tot_misses = 0;
   count_type g_tot_write_back = 0;
   
   class InstInfo
   {
   public:
      InstInfo () {
         written_back_lines = 0;
         for (int i=0 ; i<MIAMIU::NUM_ACCESS_TYPES ; ++i)
            accessCount[i] = 0;
      }
      
      count_type accessCount[MIAMIU::NUM_ACCESS_TYPES];
      count_type written_back_lines;
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
         tot_write_back = 0;
         
         cache = new MIAMIU::CacheSim(cache_size,
                         line_size,
                         cache_assoc,
                         do_conflicts);
         
         // Calls to ThreadStart are serialized by PIN; this is safe (w/ PIN)
         next = lastTLD;
         lastTLD = this;
      }
      ~ThreadLocalData() { 
         if (cache)
            delete (cache);
      }
      
      inline int ProcessEvents(uint64_t numElems, struct MemRefRecord *ref);
      void OutputResultsToFile(FILE *fout);

      void DumpCacheStats(FILE *fout)
      {
         cache->dumpToFile (fout, long_dump);
         uint64_t counters[MIAMIU::NUM_ACCESS_TYPES];
         uint64_t lines_written_back;
         cache->getTotalCounts(counters, lines_written_back);
         
         for (int i=0 ; i<MIAMIU::NUM_ACCESS_TYPES ; ++i)
            tot_accesses += counters[i];
         tot_misses = counters[MIAMIU::CAPACITY_MISS] + counters[MIAMIU::CONFLICT_MISS];
         tot_write_back = lines_written_back;

         // update the global counts of relevant events
         g_libraryCalls += libraryCalls;
         g_tot_accesses += tot_accesses;
         g_tot_misses += tot_misses;
         g_tot_write_back += tot_write_back;
         // I cannot determine the number of distinct instructions across threads
   //      g_numDistinctInstructions += numDistinctInstructions;
         
         // Print summary statistics for thread
         const char* prefix = "MIAMI_THREAD_SUMMARY:";
         fprintf(stderr, "%s T%d: Distinct references executed = %" PRIu32 "\n", prefix, tid, numDistinctInstructions);
         fprintf(stderr, "%s T%d: Number of library calls = %" PRIcount "\n", prefix, tid, libraryCalls);
         fprintf(stderr, "%s T%d: Number of memory accesses = %" PRIcount "\n", prefix, tid, tot_accesses);
         fprintf(stderr, "%s T%d: Number of cache misses = %" PRIcount "\n", prefix, tid, tot_misses);
         fprintf(stderr, "%s T%d: Number of lines written back = %" PRIcount "\n", prefix, tid, tot_write_back);
      }

   private:
      int tid;   // thread ID; I create one object for each thread
      
      // Below are the data structures needed during processing
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
      count_type tot_write_back;

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

   void FinalizeThreadData(void *tdata, const char *fname1, const char* fname2)
   {
      ThreadLocalData *tld = static_cast<ThreadLocalData*>(tdata);
      
      FILE *fout = 0;
      /* Dump cache statistics to first file */
      fout = fopen (fname1, "wb");
      if (!fout)
      {
         fprintf (stderr, "MIAMI::cachesim::FinalizeThreadData: ERROR, cannot open output file %s. Profile data will not be saved.\n", fname1);
      } else
      {
         tld->DumpCacheStats(fout);
         fclose(fout);
      }

      if (track_refs)
      {
         fout = fopen (fname2, "wb");
         if (!fout)
         {
            fprintf (stderr, "MIAMI::cachesim::FinalizeThreadData: ERROR, cannot open output file %s. Profile data will not be saved.\n", fname2);
         } else
         {
            tld->OutputResultsToFile(fout);
            fclose(fout);
         }
      }
      
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
      fprintf(stderr, "%s: Number of lines written back = %" PRIcount "\n", prefix, g_tot_write_back);
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
      
      if (track_refs)
      {
         for (i=0 ; i<numElems ; ++i, ++ref)
         {
            int32_t imgId = ref->u.info.imgid;
            if (imgId != lastImgId)
            {
               lastImgId = imgId;
               imgInfo = &(imgData[imgId]);
            }

            InstInfo* iiref = &(imgInfo->instData[ref->iidx]);
            
            // perform the cache simulation
            register addrtype addr = ref->ea >> line_shift;
            uint8_t wroteBack=0;
            const MIAMIU::CACHE_AccessType result = cache->reference(addr, ref->u.info.type, &wroteBack);
            iiref->accessCount[result] += 1;
            iiref->written_back_lines += wroteBack;
         }
      } else
      {
         for (i=0 ; i<numElems ; ++i, ++ref)
         {
            // first perform the cache simulation
            register addrtype addr = ref->ea >> line_shift;
            cache->reference(addr, ref->u.info.type);
         }
      }
      return (0);
   }  // ProcessEvents

   static void
   write_string (FILE *fs, const char *str)
   {
     /* <string_length> <string_without_terminator> */
     uint len = strlen (str);
     hpc_fwrite_le4 (&len, fs);
     fwrite (str, 1, len, fs);
   }

   void 
   ThreadLocalData::OutputResultsToFile(FILE *fd)
   {
      // write the header first
      fwrite (HPCRUNFILE_MAGIC_STR, 1, HPCRUNFILE_MAGIC_STR_LEN, fd);
      fwrite (HPCRUNFILE_VERSION, 1, HPCRUNFILE_VERSION_LEN, fd);
      fputc (HPCRUNFILE_ENDIAN, fd);
      
      // now write the number of images for which we have data
      hpc_fwrite_le4 (&(loadedImgs), fd);
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
            
         // for each image, print its name and base address
         /* <loadmodule_name>, <loadmodule_loadoffset> */
         write_string (fd, myImg->Name().c_str());
         uint64_t base_addr = myImg->BaseAddr();
         hpc_fwrite_le8 (&(base_addr), fd);
         
         /* <loadmodule_eventcount> */
         uint32_t numEv = MIAMIU::NUM_ACCESS_TYPES + 1;
         hpc_fwrite_le4 (&numEv, fd);
         
         /* Event data */
         /*   <event_x_name> <event_x_description> <event_x_period> */
         /*   <event_x_data> */
         uint64_t period = 1;
         uint64_t count = cit->instData.size();
         
         numDistinctInstructions += count;
         addrtype iaddr;
         int memop;
         
         for (int i=0 ; i<MIAMIU::NUM_ACCESS_TYPES ; ++i)
         {
            write_string (fd, MIAMIU::accessTypeName[i]);
            write_string (fd, MIAMIU::accessTypeName[i]);
            hpc_fwrite_le8 (&period, fd);
            hpc_fwrite_le8 (&count, fd);

            IIContainer::const_iterator icit = cit->instData.first();
            while ((bool)icit)
            {
               const InstInfo *vit = (const InstInfo*)icit;
               int iindex = icit.Index();    // the instruction index
               iaddr = myImg->getAddressForInstIndex(iindex, memop);
               uint64_t addr64 = (uint64_t)iaddr;  // convert to 64-bit even for 32-bit systems
               // hpcrun sample counts are 32-bit, hmmm
               uint32_t cnt32 = (uint32_t)(vit->accessCount[i]);
               
               hpc_fwrite_le4 (&cnt32, fd);
               hpc_fwrite_le8 (&addr64, fd);
               
               ++ icit;
            }
         }
         
         // write also the data for the number of lines written back
         write_string (fd, "LinesWrittenBack");
         write_string (fd, "LinesWrittenBack");
         hpc_fwrite_le8 (&period, fd);
         hpc_fwrite_le8 (&count, fd);
         
         IIContainer::const_iterator icit = cit->instData.first();
         while ((bool)icit)
         {
            const InstInfo *vit = (const InstInfo*)icit;
            int iindex = icit.Index();    // the instruction index
            iaddr = myImg->getAddressForInstIndex(iindex, memop);
            uint64_t addr64 = (uint64_t)iaddr;  // convert to 64-bit even for 32-bit systems
            // hpcrun sample counts are 32-bit, hmmm
            uint32_t cnt32 = (uint32_t)(vit->written_back_lines);
            
            hpc_fwrite_le4 (&cnt32, fd);
            hpc_fwrite_le8 (&addr64, fd);
            
            ++ icit;
         }

         ++ cit;
      }  // Iterate over images

  }


};  /* namespace StreamRD */
