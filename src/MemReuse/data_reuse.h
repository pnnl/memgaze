/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: data_reuse.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements thread level data management and the tool specific logic for a 
 * data reuse analysis tool.
 */

#ifndef _MIAMI_DATA_REUSE_H
#define _MIAMI_DATA_REUSE_H

#include "miami_types.h"

#define DEBUG_SCOPE_DETECTION 0

namespace MIAMI_MEM_REUSE
{
   extern MIAMI::addrtype addr_mask;
   extern MIAMI::addrtype addr_value;
   extern uint32_t addr_shift;

   extern bool profileErrors;
   extern int  verbose_level;
   extern bool compute_footprint;
   extern bool text_mode_output;

typedef enum {
   MEMREUSE_EVTYPE_INVALID = 0,
   MEMREUSE_EVTYPE_MEM_LOAD,
   MEMREUSE_EVTYPE_MEM_STORE,
   MEMREUSE_EVTYPE_ENTER_SCOPE,
   MEMREUSE_EVTYPE_ROUTINE_ENTRY,
   MEMREUSE_EVTYPE_ITER_CHANGE,
   MEMREUSE_EVTYPE_EXIT_SCOPE,
   MEMREUSE_EVTYPE_AFTER_CALL,
   MEMREUSE_EVTYPE_BR_BYPASS_CALL,  /* taken branch to the instruction after a call */
   MEMREUSE_EVTYPE_LAST
} MemReuseEvType;

typedef enum {
   MEMREUSE_SCOPE_INVALID = 0,
   MEMREUSE_SCOPE_ROUTINE,
   MEMREUSE_SCOPE_LOOP,
   MEMREUSE_SCOPE_LAST
} MemReuseScopeType;
   
/* We use buffering to process multiple memory accesses at once.
 * Define the structure of a buffer element.
 * We store different event types: memory ref events and scope events.
 * Memory ref events will store 
 *  - ea   : the effective address
 *  - ea2  : the scope index(??)
 *  - iidx : the instruction index inside its load module
 *  - imgid: the image ID, unique during a run
 *  - type : the event type, e.g. load, store
 *  - aux  : unused, padding for now
 *
 * Scope events will store:
 *  - ea   : store the current stack pointer if possible, or the scope start address
 *  - ea2  : the return IP (for routine entry only)
 *  - iidx : scope index inside its load module
 *  - imgid: the image ID, unique during a run
 *  - type : the event type, e.g., scope enter, exit, iteration change
 *  - aux  : might use it to indicate # of exited scopes, add an event for each entered level though
 *
 * I want to maintain a callstack of the program. Thus, on function calls, the 
 * scope ID identifies a unique path in the calling context tree. I must maintain
 * a tree of calling contexts, brrr.
 * Scope IDs for routines will identify nodes in the calling context tree, that 
 * spans all images. Thus, they will have an image ID of 0, while loops will have 
 * a unique numeric ID inside each image, so image ID will be non-zero.
 */

/* The PIN buffering API supports arguments of type ADDRINT and UINT32,
 * but not arguments of smaller sizes. I want to pack a short int and two 
 * byte sized fields into a UINT32 variable. For this, I will use an 
 * anonymous union with a 32-bit field and a packed structure of the needed
 * three small fields.
 */
struct MemRefRecord
{
   struct memref_uint32_pack {
      uint16_t imgid;  /* 16-bits for the image id, at most 65535 images */
      uint8_t type;    /* event type: load, store, enter scope, exit scope, iteration change, etc. */
      uint8_t aux;     /* auxiliary field to make the structure size a multiple of addrtype size */
   };
   
   MIAMI::addrtype ea; /* can be 32-bit or 64-bit address type */
   MIAMI::addrtype ea2;/* can be 32-bit or 64-bit address type */
   uint32_t iidx;      /* 32-bits for the instruction index inside its image */
   union memref_uint32_u {
      memref_uint32_pack info; /* access the pack's fields individually */
      uint32_t field32;        /* access all 32 bits of the structure in one go */
   } u;
};

/* Define a smaller record where we pre-compute information about the events
 * during the static analysis phase, to be used at run-time.
 */
struct ScopeEventRecord
{
   ScopeEventRecord(uint32_t scope, uint16_t img, uint16_t evt, uint16_t edget=0, uint16_t ax=0) :
       scopeId(scope), imgId(img), type(evt), edgeType(edget), aux(ax)
   {
   }
   
   uint32_t scopeId;  // scope ID
   uint16_t imgId;    // image ID
   uint16_t type;     // event type
   uint16_t edgeType; // if it is on the FALLTHRU or BRANCH_TAKEN edge
   uint16_t aux;      // number of scopes exited
};

extern void* CreateThreadLocalData(int tid);
extern void FinalizeThreadData(void *tdata, FILE *fout);
extern int  ProcessBufferedElements(uint64_t numElems, struct MemRefRecord *reference, void *data);
extern const char* EventTypeToString(MemReuseEvType ev);
extern void PrintGlobalStatistics();

}  /* namespace MIAMI_MEM_REUSE */

#endif
