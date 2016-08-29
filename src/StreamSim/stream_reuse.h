/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: stream_reuse.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements thread level data management and the tool specific logic for a 
 * streaming concurrency tool.
 */

#ifndef _MIAMI_STREAM_REUSE_H
#define _MIAMI_STREAM_REUSE_H

#include "miami_types.h"

namespace StreamRD
{
   extern bool noStreamSim;
   extern int line_shift;
   extern int verbose_level;
   
   extern uint32_t stream_table_size, stream_history_size, stream_max_stride;
   extern uint32_t stream_cache_size, stream_line_size, stream_cache_assoc;

   struct MemRefRecord
   {
      MIAMI::addrtype ea; /* can be 32-bit or 64-bit adress type */
      int32_t iidx;       /* 32-bits for the instruction index inside its image */
      int32_t imgId;
   };

   extern void* CreateThreadLocalData(int tid);
   extern void FinalizeThreadData(void *tdata, FILE *fout);
   extern int  ProcessBufferedElements(uint64_t numElems, struct MemRefRecord *reference, void *data);
   extern void PrintGlobalStatistics();

};  /* namespace StreamRD */

#endif

