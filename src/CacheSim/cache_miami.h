/*
 * This file is part of the MIAMI framework. For copyright information, see    
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: cache_miami.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data type declarations for a cache simulator tool.
 */


#ifndef _CACHE_MIAMI_H
#define _CACHE_MIAMI_H

#include "miami_types.h"

namespace MIAMI_CACHE_SIM
{
   extern int line_shift;
   extern int verbose_level;
   
   extern uint32_t cache_size, line_size, cache_assoc;
   extern bool do_conflicts;
   extern bool track_refs, long_dump;

   struct MemRefRecord
   {
      struct memref_uint32_pack {
         uint16_t imgid;  /* 16-bits for the image id, at most 65535 images */
         uint16_t type;   /* ref type: load or store. OInly 1-bit is used, 15 bits available. */
      };
      
      MIAMI::addrtype ea; /* can be 32-bit or 64-bit adress type */
      int32_t iidx;       /* 32-bits for the instruction index inside its image */
      union memref_uint32_u {
         memref_uint32_pack info; /* access the pack's fields individually */
         uint32_t field32;        /* access all 32 bits of the structure in one go */
      } u;
   };

   extern void* CreateThreadLocalData(int tid);
   extern void FinalizeThreadData(void *tdata, const char* fname1, const char* fname2);
   extern int  ProcessBufferedElements(uint64_t numElems, struct MemRefRecord *reference, void *data);
   extern void PrintGlobalStatistics();

};  /* namespace MIAMI_CACHE_SIM */

#endif

