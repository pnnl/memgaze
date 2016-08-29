/*
 * This file is part of the MIAMI framework. For copyright information, see    
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: cfg_data.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Defines the data types for a CFG profiler tool.
 */

#ifndef _CFG_DATA_MIAMI_H
#define _CFG_DATA_MIAMI_H

#include "miami_types.h"
#include <stdlib.h>

using MIAMI::addrtype;

namespace MIAMI_CFG
{
#define INITIAL_BUF_SIZE 32768

   typedef uint64_t count_type;
   #define PRIcount PRIu64
   
   extern int verbose_level;
   extern bool save_bbls;
   extern bool print_stats;
   extern std::string debug_rtn;
   extern int32_t thr_buf_size;

   struct MiamiCfgStats {
      MiamiCfgStats() {
         buffer_resizes = 0;
         tot_increments = 0;
         ind_increments = 0;
         routine_entries = 0;
         trace_counters = 0;
         edge_counters = 0;
      }
      
      count_type buffer_resizes;
      count_type tot_increments;
      count_type ind_increments;
      count_type routine_entries;
      count_type trace_counters;
      count_type edge_counters;
   };

   /* This is the structure type of a buffer element.
    */
   struct BufRecord
   {
      addrtype target;
      int32_t imgId;
      int32_t iidx;
   };

   extern void* CreateThreadLocalData(int tid);
   extern count_type* GetThreadCountersBuffer(void *tdata);
   extern count_type* IncreaseThreadCountersBuffer(void *tdata);
   extern void FinalizeThreadData(void *tdata, const char* fname1, const char* rank_id);
   extern int  ProcessBufferedElements(uint64_t numElems, struct BufRecord *buf_p, void *data);
   extern void PrintGlobalStatistics();
   extern void RecordBufferIncrease(void *tdata, int32_t val);

};  /* namespace MIAMI_CFG */

#endif

