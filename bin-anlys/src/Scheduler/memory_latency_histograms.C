/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: memory_latency_histograms.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements the logic for parsing memory latency simulation results, and 
 * the mechanistic modeling approach to combine them with the instruction 
 * execution rate predictions from MIAMI, to compute total execution times.
 */

#include "MiamiDriver.h"
#include "miami_types.h"

using namespace MIAMI;

namespace MIAMI {

MemoryLatencyHistogram* defaultMLHP = 0;

int
MIAMI_Driver::parse_memory_latencies(const std::string& memory_file)
{
   FILE *fd;
   
   fd = fopen (memory_file.c_str(), "r");
   if (fd == NULL)
   {
      fprintf (stderr, "WARNING: Cannot open the file %s containing memory latency information for references.\n",
             memory_file.c_str());
      return (-1);
   }

   // This file contains hstograms of simulated memory latencies in the following format:
   // - one reference per line; for each reference list:
   //   - pc(hex format) num_bins(N) l1 c1 l2 c2 ... lN cN
   addrtype pc = 0;
   int N = 0;  // number of bins
   int res;
   mem_latency_t lat;
   access_count_t count;
   int gotErrors = 0;
   
   res = fscanf(fd, "%" PRIxaddr " %d", &pc, &N);
   while (!gotErrors && res==2 && !feof(fd))
   {
      if (N<0)
      {
         fprintf(stderr, "MIAMI_Driver::parse_machine_latencies: invalid number of bins for reference at pc %"
                   PRIxaddr ", num_bins = %d.\n", pc, N);
         ++ gotErrors;
         break;
      }
      
      if (N>0)
      {
         MemoryLatencyHistogram* &mhist = refLatencies[pc];
         // we should see each reference only once
         if (mhist != 0)
         {
            fprintf(stderr, "MIAMI_Driver::parse_machine_latencies: found repeat entry for pc 0x%" PRIxaddr "\n", pc);
            ++ gotErrors;
            break;
         }
         mhist = new MemoryLatencyHistogram(N);
      
         for (int i=0 ; i<N && !gotErrors ; ++i)
         {
            res = fscanf(fd, "%" PRImlat " %" PRIacnt, &lat, &count);
            if (res != 2)
            {
               fprintf(stderr, "MIAMI_Driver::parse_machine_latencies: trying to read latency and count for pc %"
                     PRIxaddr ", bin %d, but could read only %d elements.\n",
                     pc, i, res);
               ++ gotErrors;
               break;
            }
            (*mhist)[i].first = lat;
            (*mhist)[i].second = count;
         }
      }

      res = fscanf(fd, "%" PRIxaddr " %d", &pc, &N);
   }

   if (gotErrors)
   {
      fclose(fd);
      return (-1);
   } else
      return (0);
}


void
MIAMI_Driver::compute_memory_stalls_for_scope(ScopeImplementation *pscope, AddrSet& scopeMemRefs)
{
   Machine *tmach = targets.front();
   int win_size = tmach->getMachineFeature(MachineFeature_WINDOW_SIZE);
   
   if (! scopeMemRefs.empty() && win_size>=0 && refLatencies.size()>0)
   {
      // compute maximum computation overlap based on the scope's IPC.
      float scopeIPC = pscope->NumUops() / pscope->Latency();
      float maxMemOverlap = (float)win_size / scopeIPC;
      TimeAccount& stats = pscope->getExclusiveTimeStats();
      
      AddrSet::iterator ait = scopeMemRefs.begin();
      for ( ; ait!=scopeMemRefs.end() ; ++ait)
      {
         // find if we have any memory latency information associated with this ref
         int ridx;
         if ((ridx = refLatencies.indexOf(*ait)) >= 0)
         {
            const MemoryLatencyHistogram* mlhist = refLatencies.atIndex(ridx);
            assert (mlhist != 0);
            
            // iterate over the bins of type <latency, count> pairs
            MemoryLatencyHistogram::const_iterator mit = mlhist->begin();
            for ( ; mit!=mlhist->end() ; ++mit)
            {
               if (mit->first <= maxMemOverlap)  // add memory overlap metric
                  stats.addMemoryOverlapTime(mit->first * mit->second);
               else   // add both memory overlap and memory stall metrics
               {
                  stats.addMemoryOverlapTime(maxMemOverlap * mit->second);
                  stats.addMemoryStallTime((mit->first - maxMemOverlap) * mit->second);
               }
            }  // iterate over one histogram
         }  // if we have latency data for this reference
      }  // iterate over the refs in one scope
   }  // should we compute memory stall/overlap metrics?

}

}  /* namespace MIAMI */

