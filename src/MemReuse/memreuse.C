/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: memreuse.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements the control management logic for a data reuse analysis tool
 * built on top of PIN. This tool identifies data reuse patterns inside
 * each thread and collects a histogram of memory reuse distances observed 
 * for each reuse pattern during execution.
 */

#include <pin.H>

#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <iostream>
#include <fstream>

#include "miami_utils.h"
#include "routine.h"
#include "load_module.h"
#include "debug_miami.h"
#include "instruction_decoding.h"
#include "data_reuse.h"

#define KILO 1024
#define MEGA (KILO*KILO)
#define GIGA (KILO*MEGA)

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile (KNOB_MODE_WRITEONCE,    "pintool",
    "o", "", "specify prefix of output file name, default - execname-:block_size:");
KNOB<UINT32> KnobLineSize (KNOB_MODE_WRITEONCE, "pintool",
    "B","64", "memory block size");
KNOB<string> KnobRoutineName (KNOB_MODE_WRITEONCE,    "pintool",
    "r", "", "specify routine name for which to dump CFG in dot format, default: none");
KNOB<BOOL>   KnobCarryScope (KNOB_MODE_WRITEONCE,    "pintool",
    "carry", "0", "compute the carrying scopes of reuse patterns");
KNOB<BOOL>   KnobFootprint (KNOB_MODE_WRITEONCE,    "pintool",
    "footprint", "0", "compute the footprint of scopes (turns on carry flag as well)");
KNOB<BOOL>   KnobScopeErrors (KNOB_MODE_WRITEONCE,    "pintool",
    "errors", "0", "collect and present a distribution of observed stack inconsistencies");
KNOB<UINT32>   KnobVerbosityLevel (KNOB_MODE_WRITEONCE,    "pintool",
    "debug", "0", "verbosity of debugging messages: 0 - off, 1 - initialization, 2 - add static analysis messages, 3 - add instrumentation events, 4 - add trace of dynamic events (very verbose), 5 - add event processing messages (extremely verbose)");
KNOB<BOOL>   KnobTextMode (KNOB_MODE_WRITEONCE,    "pintool",
    "text", "1", "output profile data in text mode");

KNOB<BOOL>   KnobNoPid (KNOB_MODE_WRITEONCE,    "pintool",
    "no_pid", "0", "do not append process PID to output file name");
KNOB<BOOL>   KnobUseScratchRegs (KNOB_MODE_WRITEONCE,    "pintool",
    "use_scratch", "0", "allocate scratch registers to hold the sampling mask and value");
KNOB<ADDRINT> KnobBlockMask (KNOB_MODE_WRITEONCE, "pintool",
    "block_mask","0", "mask applied to memory block index for address space sampling");
KNOB<ADDRINT> KnobBlockValue (KNOB_MODE_WRITEONCE, "pintool",
    "block_value","0", "use together with -block_mask to select memory blocks for address space sampling");
    
#include "on_demand_pin.h"
#include "use_mpi_rank.h"

/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This is the MIAMI data reuse simulator.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}
/* ===================================================================== */

#define NUM_BUF_PAGES 8

using MIAMI::addrtype;

namespace MIAMI_MEM_REUSE
{
   extern MIAMI::LoadModule **allImgs;
   extern uint32_t maxImgs;
   extern uint32_t loadedImgs;
   
   bool use_sampling = false;
   bool use_scratch_reg = false;
   bool compute_carry = false;
   
   BUFFER_ID bufId;
   TLS_KEY   thr_key;
   REG scratch_reg0;
   REG scratch_reg1;
};
/* ===================================================================== */

static ADDRINT PIN_FAST_ANALYSIS_CALL mustSampleRef(ADDRINT ea)
{
   return ((ea & MIAMI_MEM_REUSE::addr_mask) == MIAMI_MEM_REUSE::addr_value);
}

/* ===================================================================== */

static ADDRINT PIN_FAST_ANALYSIS_CALL mustSampleRef2(ADDRINT ea, ADDRINT mask, ADDRINT val)
{
   return ((ea & mask) == val);
}

/* ===================================================================== */

void
InsertScopeInstrumentation(INS lIns, MIAMI_MEM_REUSE::ScopeEventRecord *ser)
{
   MIAMI_MEM_REUSE::MemRefRecord::memref_uint32_u val;
   
   val.info.type = ser->type;
   val.info.imgid = ser->imgId;
   val.info.aux = ser->aux;
   IPOINT ipoint = IPOINT_AFTER;
   if (INS_Stutters(lIns))   // a REP instruction
   {
#if DEBUG_SCOPE_DETECTION
      if (MIAMI_MEM_REUSE::verbose_level > 2)
      {
         cerr << "Inserting event of type "
              << ser->type << ", imgId=" << ser->imgId << ", aux=" << ser->aux
              << ", edgeType=" << ser->edgeType << ", scopeId=" << ser->scopeId
              << " after REP instruction: " << INS_Disassemble(lIns) 
              << endl;
      }
#endif
   } else
   {
      if (ser->edgeType)
      {
         ipoint = IPOINT_TAKEN_BRANCH;
         if (!INS_IsBranchOrCall(lIns))
         {
            cerr << "End of BBL, instruction is not Barnch or Call: " 
                 << INS_Disassemble(lIns) << " while inserting event of type "
                 << ser->type << ", imgId=" << ser->imgId << ", aux=" << ser->aux
                 << ", edgeType=" << ser->edgeType << ", scopeId=" << ser->scopeId
                 << endl;
            assert(INS_IsBranchOrCall(lIns));
         }
      } else
      {
         if (!INS_HasFallThrough(lIns))
         {
            if (MIAMI_MEM_REUSE::verbose_level>1 || !INS_IsCall(lIns))
               cerr << "End of BBL, instruction does not have FallThru: " 
                    << INS_Disassemble(lIns) << " while inserting event of type "
                    << ser->type << ", imgId=" << ser->imgId << ", aux=" << ser->aux
                    << ", edgeType=" << ser->edgeType << ", scopeId=" << ser->scopeId
                    << endl;
            assert(INS_HasFallThrough(lIns) || INS_IsCall(lIns));
            // If the instruction is a CALL, I cannot insert instrumentation on this edge.
            return;
         }
      }
   }
#if DEBUG_SCOPE_DETECTION
   if (MIAMI_MEM_REUSE::verbose_level > 2)
   {
      fprintf(stderr, "Inserting instrumentation at ipoint %d, on instruction at 0x%" 
            PRIxaddr " (0x%" PRIxaddr "), event of type %d for scope %d, img %d, aux=%d\n",
            ipoint, INS_Address(lIns), INS_Address(lIns)+INS_Size(lIns), ser->type, 
            ser->scopeId, ser->imgId, ser->aux);
   }
#endif
   INS_InsertFillBuffer (lIns, ipoint, MIAMI_MEM_REUSE::bufId,
          IARG_REG_VALUE, REG_STACK_PTR, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, ea),
          IARG_UINT32, ser->scopeId, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, iidx),
          IARG_UINT32, val.field32, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, u.field32),
          IARG_END);
}

VOID AnalyzeTrace(TRACE trace, VOID *v)
{
   if (! MIAMI::instrumentation_is_active)
      return;
   
   MIAMI::LoadModule *myimg = 0;
   // Find which routine are we in
   RTN rtn = TRACE_Rtn (trace);
   if (! RTN_Valid (rtn))
   {
      return;
   }
   uint32_t id = IMG_Id (SEC_Img (RTN_Sec (rtn)));
   if (id>=MIAMI_MEM_REUSE::maxImgs || MIAMI_MEM_REUSE::allImgs[id]==0)
   {
      fprintf (stdout, "Found instruction in image which has not been loaded.\n");
      return;
   }
   myimg = MIAMI_MEM_REUSE::allImgs [id];

   MIAMI_MEM_REUSE::MemRefRecord::memref_uint32_u val;
   val.info.imgid = id;
   val.info.aux = 0;
   
   if (MIAMI_MEM_REUSE::compute_carry)
   {
      addrtype tStart = TRACE_Address(trace);  // entry address into the routine
      ADDRINT rStart = RTN_Address(rtn);
      const ADDRINT raddr = rStart - myimg->BaseAddr();

      // another bug (or bad design) in PIN. PIN limits routine sizes to 200,000 bytes.
      // For larger routines, it reports their sizes correctly, but then it creates
      // an overlapped routine for whatever is beyond that size limit.
      // To work around this, I keep track of routine spans in my tool.
      // Check if current trace start is part of an already created MIAMI routine first.
      MIAMI::Routine* robj = myimg->HasRoutineAtPC(tStart);
      
      if (! robj)  // we do not have a routine for this PC, create one
         // for some libraries, it can happen that local symbols are stripped.
         // Thus, ranges of addresses do not belong do any known routine.
         // PIN assigns such traces to the previous symbol. Must catch this case.
         robj = myimg->HasRoutineAtPC(rStart);

      if (! robj)
      {
         // search by using routine offset inside the image; image can be loaded at diff address
         MIAMI::Routine* &rref = myimg->GetRoutineAt(raddr);
         assert (!rref);
         
         rref = new MIAMI::Routine(myimg, rStart, RTN_Size(rtn), RTN_Name(rtn), raddr);
         // decode instructions from the start of the trace, and try to discover 
         // the control flow inside the routine
         rref->discover_CFG(tStart);  // statically discover the CFG starting from entry
         if (rStart!=tStart)          // if entry address is different than routine start
            rref->discover_CFG(rStart);  // discover CFG starting from routine start as well
         
         // Dump the static CFG if this is the tracked routine
         if (!KnobRoutineName.Value().compare(rref->Name()))
            rref->CfgToDot();
         robj = rref;
         myimg->AddRoutine(robj);

         // We must insert instrumentation before and after a scope, also on an
         // iteration change, and of course, on every memory micro-op.
         // We'll use the PIN buffer API to process multiple events at once.
         // Also, must use TLS to support multi-threading.
         //
         // I need a routine to build the loop nesting structure and determine
         // the places where we must insert the scope markers.
         // On a TRACE callback, I must check if this is a scope entry/exit/iter
         // change.
         robj->DetermineInstrumentationPoints();
         
         // I record the places where I have to insert instrumentation events, 
         // in separate data structures inside the load module.
         // I think I can delete the CFG at this point, to save memory
         robj->DeleteControlFlowGraph();
      }

      if (tStart == rStart)  // routine entry, add event
      {
         uint32_t scopeId = myimg->getScopeIndex(rStart, 0);
         val.info.type = MIAMI_MEM_REUSE::MEMREUSE_EVTYPE_ROUTINE_ENTRY;
         INS_InsertFillBuffer (BBL_InsHead(TRACE_BblHead(trace)), IPOINT_BEFORE, MIAMI_MEM_REUSE::bufId,
                IARG_REG_VALUE, REG_STACK_PTR, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, ea),
                IARG_RETURN_IP, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, ea2),
                IARG_UINT32, scopeId, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, iidx),
                IARG_UINT32, val.field32, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, u.field32),
                IARG_END);
      }
      
      // check if this trace follows a function call instruction. 
      // It cannot be a routine start at the same time, but then again,
      // this depends on the symbols found in the object file.
      // It is safer to check all the traces, including apparent routine starts
      unsigned int preHash = MIAMI::ScopeEventsHash::pre_hash(tStart);
      if (myimg->bblEvents.is_member(tStart, preHash))
      {
         MIAMI::ListOfEvents *evL = myimg->bblEvents.insert(tStart, preHash);
         MIAMI::ListOfEvents::iterator lit;
         if (evL==NULL || evL->size()!=1)
         {
            if (!evL)
               fprintf(stderr, "evL = %p\n", evL);
            if (evL && MIAMI_MEM_REUSE::verbose_level>0)
            {
               fprintf(stderr, "evL->size()=%ld\n", (long)(evL->size()));
               for(lit = evL->begin() ; lit!=evL->end() ; ++lit)
               {
                  cerr << "Found event of type "
                       << lit->type << ", imgId=" << lit->imgId << ", aux=" << lit->aux
                       << ", edgeType=" << lit->edgeType << ", scopeId=" << lit->scopeId
                       << " at trace start 0x" << hex << tStart << dec << endl;
               }
            }
            assert(evL != NULL && evL->size()>0);
         }
         for(lit = evL->begin() ; lit!=evL->end() ; ++lit)
         {
#if DEBUG_SCOPE_DETECTION
            if (MIAMI_MEM_REUSE::verbose_level > 2)
            {
               fprintf(stderr, "Inserting AFTER_CALL event at start of trace 0x%" PRIxaddr " (0x%" PRIxaddr ") in scopeId=%d, img=%d\n",
                  tStart, tStart - myimg->BaseAddr(), lit->scopeId, lit->imgId);
            }
#endif

            val.info.type = lit->type;
            val.info.imgid = lit->imgId;
            INS_InsertFillBuffer (BBL_InsHead(TRACE_BblHead(trace)), IPOINT_BEFORE, MIAMI_MEM_REUSE::bufId,
                   IARG_REG_VALUE, REG_STACK_PTR, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, ea),
                   IARG_UINT32, lit->scopeId, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, iidx),
                   IARG_UINT32, val.field32, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, u.field32),
                   IARG_END);
         }
      }
   }
   
   // Visit every basic block in the trace
   for (BBL bbl=TRACE_BblHead(trace) ; BBL_Valid(bbl); bbl=BBL_Next(bbl))
   {
#if 1
      for (INS ins=BBL_InsHead(bbl) ; INS_Valid(ins) ; ins=INS_Next(ins))
      {
         UINT32 memOperands = INS_MemoryOperandCount(ins);
         if (memOperands>0)
         {
            ADDRINT iaddr = INS_Address (ins); // - myimg->BaseAddr();
            
            // Iterate over each memory operand of the instruction.
            for (UINT32 memOp=0 ; memOp<memOperands ; ++memOp)
            {
               if (INS_MemoryOperandIsRead(ins, memOp))
               {
                  MIAMI::IntPair& ipair = myimg->getInstIndexAndScope (iaddr, memOp);
                  val.info.type = MIAMI_MEM_REUSE::MEMREUSE_EVTYPE_MEM_LOAD;
               
                  if (MIAMI_MEM_REUSE::use_sampling)
                  {
                     if (MIAMI_MEM_REUSE::use_scratch_reg)
                        INS_InsertIfPredicatedCall (ins, IPOINT_BEFORE, (AFUNPTR)mustSampleRef2,
                             IARG_FAST_ANALYSIS_CALL,
                             IARG_MEMORYOP_EA, memOp, 
                             IARG_REG_VALUE, MIAMI_MEM_REUSE::scratch_reg0, IARG_REG_VALUE, MIAMI_MEM_REUSE::scratch_reg1,
                             IARG_END);
                     else
                        INS_InsertIfPredicatedCall (ins, IPOINT_BEFORE, (AFUNPTR)mustSampleRef2,
                             IARG_FAST_ANALYSIS_CALL,
                             IARG_MEMORYOP_EA, memOp, 
                             IARG_ADDRINT, MIAMI_MEM_REUSE::addr_mask, IARG_ADDRINT, MIAMI_MEM_REUSE::addr_value,
                             IARG_END);
                     INS_InsertFillBufferThen (ins, IPOINT_BEFORE, MIAMI_MEM_REUSE::bufId,
                            IARG_MEMORYOP_EA, memOp, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, ea),
                            // Hmmm, I copy a UINT32 into a ADDRINT sized field. Error prone??
                            IARG_UINT32, ipair.scope, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, ea2),
                            IARG_UINT32, ipair.index, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, iidx),
                            IARG_UINT32, val.field32, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, u.field32),
                            IARG_END);
                  } else   // no sampling
                  {
                     INS_InsertFillBufferPredicated (ins, IPOINT_BEFORE, MIAMI_MEM_REUSE::bufId,
                            IARG_MEMORYOP_EA, memOp, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, ea),
                            // Hmmm, I copy a UINT32 into a ADDRINT sized field. Error prone??
                            IARG_UINT32, ipair.scope, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, ea2),
                            IARG_UINT32, ipair.index, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, iidx),
                            IARG_UINT32, val.field32, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, u.field32),
                            IARG_END);
                  }  /* no sampling */
               }  /* if the operand is Read */
            }  /* loop over memory operands */

            // Repeat the loop over memory operands for stores, The execute after all the loads
            for (UINT32 memOp=0 ; memOp<memOperands ; ++memOp)
            {
               if (INS_MemoryOperandIsWritten(ins, memOp))
               {
                  MIAMI::IntPair& ipair = myimg->getInstIndexAndScope (iaddr, memOp);
                  val.info.type = MIAMI_MEM_REUSE::MEMREUSE_EVTYPE_MEM_STORE;
               
                  if (MIAMI_MEM_REUSE::use_sampling)
                  {
                     if (MIAMI_MEM_REUSE::use_scratch_reg)
                        INS_InsertIfPredicatedCall (ins, IPOINT_BEFORE, (AFUNPTR)mustSampleRef2,
                             IARG_FAST_ANALYSIS_CALL,
                             IARG_MEMORYOP_EA, memOp, 
                             IARG_REG_VALUE, MIAMI_MEM_REUSE::scratch_reg0, IARG_REG_VALUE, MIAMI_MEM_REUSE::scratch_reg1,
                             IARG_END);
                     else
                        INS_InsertIfPredicatedCall (ins, IPOINT_BEFORE, (AFUNPTR)mustSampleRef2,
                             IARG_FAST_ANALYSIS_CALL,
                             IARG_MEMORYOP_EA, memOp, 
                             IARG_ADDRINT, MIAMI_MEM_REUSE::addr_mask, IARG_ADDRINT, MIAMI_MEM_REUSE::addr_value,
                             IARG_END);
                     INS_InsertFillBufferThen (ins, IPOINT_BEFORE, MIAMI_MEM_REUSE::bufId,
                            IARG_MEMORYOP_EA, memOp, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, ea),
                            // Hmmm, I copy a UINT32 into a ADDRINT sized field. Error prone??
                            IARG_UINT32, ipair.scope, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, ea2),
                            IARG_UINT32, ipair.index, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, iidx),
                            IARG_UINT32, val.field32, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, u.field32),
                            IARG_END);
                  } else   // no sampling
                  {
                     INS_InsertFillBufferPredicated (ins, IPOINT_BEFORE, MIAMI_MEM_REUSE::bufId,
                            IARG_MEMORYOP_EA, memOp, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, ea),
                            // Hmmm, I copy a UINT32 into a ADDRINT sized field. Error prone??
                            IARG_UINT32, ipair.scope, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, ea2),
                            IARG_UINT32, ipair.index, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, iidx),
                            IARG_UINT32, val.field32, offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, u.field32),
                            IARG_END);
                  }  /* no sampling */
               }  /* if operand is writen */
            }  /* second loop over memory operands */

         }  /* if this is a memory instruction */
      }  /* for every INS in this BBL */
#endif
      
      // check if we have to add any scope instrumentation on any of the edges
      // outgoing from this BBL
      if (MIAMI_MEM_REUSE::compute_carry)
      {
         addrtype sBbl = BBL_Address(bbl);
         addrtype eBbl = sBbl + BBL_Size(bbl);
         
         MIAMI::ScopeEventsMap::const_iterator lb = myimg->edgeEvents.upper_bound(sBbl);
         MIAMI::ScopeEventsMap::const_iterator ub = myimg->edgeEvents.upper_bound(eBbl);
         
         INS ins = BBL_InsHead(bbl);
         addrtype ins_addr = INS_Address(ins), ins_end = ins_addr+INS_Size(ins);
         for ( ; lb!=ub ; ++lb)
         {
            while (INS_Valid(ins) && ins_end<lb->first)
            {
               ins = INS_Next(ins);
               if (!INS_Valid(ins))
                  break;
               ins_addr = INS_Address(ins); 
               ins_end = ins_addr + INS_Size(ins);
            }
            if (lb->first == ins_end)
            {
               MIAMI::ListOfEvents *evL = lb->second;
               assert(evL!=NULL && !evL->empty());
               MIAMI::ListOfEvents::iterator lit = evL->begin();
               // I want to insert BR_BYPASS_CALL events last on an edge
               MIAMI_MEM_REUSE::ScopeEventRecord *ser = 0;
               for( ; lit!=evL->end() ; ++lit)
               {
                  // do not add ITER_CHANGE events for inner loops
                  if (lit->type==MIAMI_MEM_REUSE::MEMREUSE_EVTYPE_ITER_CHANGE &&
                        myimg->scopeIsInner[lit->scopeId])
                     continue;

                  if (lit->type == MIAMI_MEM_REUSE::MEMREUSE_EVTYPE_BR_BYPASS_CALL)
                  {
                     ser = &(*lit);
                     continue;
                  }
                  InsertScopeInstrumentation(ins, &(*lit));
               }
               if (ser)
                  InsertScopeInstrumentation(ins, ser);
            } else
            {
               fprintf(stderr, "ERROR: Could not find an INS handler whose end address is 0x%" PRIxaddr "\n",
                    lb->first);
            }
         }
         
      }  // if compute carry scope
   }  /* for every BBL in the TRACE */
}


/* ===================================================================== */

VOID ImageLoad (IMG img, VOID *v)
{
   uint32_t id = IMG_Id (img);
   MIAMI::MIAMI_OnDemandImgLoad(img);
   MIAMI::MIAMI_MpiRankImgLoad(img);
   
   if (id>=MIAMI_MEM_REUSE::maxImgs)  // I need to extend my array
   {
      uint32_t oldSize = MIAMI_MEM_REUSE::maxImgs;
      while (MIAMI_MEM_REUSE::maxImgs <= id)
         MIAMI_MEM_REUSE::maxImgs <<= 1;
      MIAMI_MEM_REUSE::allImgs = (MIAMI::LoadModule**) realloc (MIAMI_MEM_REUSE::allImgs, MIAMI_MEM_REUSE::maxImgs*sizeof (MIAMI::LoadModule*));
      for (uint32_t i=oldSize ; i<MIAMI_MEM_REUSE::maxImgs ; ++i)
         MIAMI_MEM_REUSE::allImgs[i] = 0;
   }
   MIAMI::LoadModule* & newimg = MIAMI_MEM_REUSE::allImgs [id];
   if (newimg)   // should be NULL I think
      assert (!"Loading image with id seen before");

   MIAMI::addrtype load_offset = IMG_LoadOffset(img);
   newimg = new MIAMI::LoadModule (id, load_offset, IMG_LowAddress(img)-load_offset, IMG_Name(img)); 
   ++ MIAMI_MEM_REUSE::loadedImgs;
}
/* ===================================================================== */


VOID Fini (int code, VOID * v)
{
   unsigned int i;
   MIAMI::MIAMI_OnDemandFini();
   MIAMI_MEM_REUSE::PrintGlobalStatistics();
   
   // deallocate the info stored for each image
   for (i=0 ; i<MIAMI_MEM_REUSE::maxImgs ; ++i)
      if (MIAMI_MEM_REUSE::allImgs[i])
         delete (MIAMI_MEM_REUSE::allImgs[i]);
   free (MIAMI_MEM_REUSE::allImgs);
}
/* ===================================================================== */


/*!
 * Called when a buffer fills up, or the thread exits, so we can process it or pass it off
 * as we see fit.
 * @param[in] id		buffer handle
 * @param[in] tid		id of owning thread
 * @param[in] ctxt		application context
 * @param[in] buf		actual pointer to buffer
 * @param[in] numElements	number of records
 * @param[in] v			callback value
 * @return  A pointer to the buffer to resume filling.
 */
VOID * BufferFull(BUFFER_ID id, THREADID tid, const CONTEXT *ctxt, VOID *buf,
                  UINT64 numElements, VOID *v)
{
    if (MIAMI_MEM_REUSE::verbose_level > 3)
    {
       cerr << "BufferFull, numElements=" << numElements << endl;
       unsigned int i;
       struct MIAMI_MEM_REUSE::MemRefRecord *ref2 = (struct MIAMI_MEM_REUSE::MemRefRecord*)buf;
       for (i=0 ; i<numElements ; ++i, ++ref2)
       {
          cerr << i << ") " << hex
               << "ea=0x" << ref2->ea << dec
               << ", iidx=" << ref2->iidx
               << ", imgid=" << ref2->u.info.imgid
               << ", type=" << (int)ref2->u.info.type
               << ", aux=" << (int)ref2->u.info.aux
               << endl;
       }
    }

    struct MIAMI_MEM_REUSE::MemRefRecord *ref = (struct MIAMI_MEM_REUSE::MemRefRecord*)buf;
    MIAMI_MEM_REUSE::ProcessBufferedElements(numElements, ref, 
               PIN_GetThreadData(MIAMI_MEM_REUSE::thr_key, tid));
    
    return buf;
}
/* ===================================================================== */

VOID ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    if (MIAMI_MEM_REUSE::use_scratch_reg)
    {
       PIN_SetContextReg(ctxt, MIAMI_MEM_REUSE::scratch_reg0, MIAMI_MEM_REUSE::addr_mask);
       PIN_SetContextReg(ctxt, MIAMI_MEM_REUSE::scratch_reg1, MIAMI_MEM_REUSE::addr_value);
    }
    // A thread will need to look up its local data, so save pointer in TLS
    PIN_SetThreadData(MIAMI_MEM_REUSE::thr_key, MIAMI_MEM_REUSE::CreateThreadLocalData(tid), tid);
}
/* ===================================================================== */

VOID ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
   char fname1[256], rankbuf[32], pidbuf[32];
   
   MIAMI::LoadModule *img1 = MIAMI_MEM_REUSE::allImgs [1];
   if (! KnobNoMpiRank.Value())
      sprintf(rankbuf, "-%05d-%04d", MIAMI::miami_process_rank, tid);
   else
      sprintf(rankbuf, "-%04d", tid);
      
    pid_t procPid = PIN_GetPid();
    if (! KnobNoPid.Value())
       sprintf(pidbuf, "-%05d", procPid);
    else
       pidbuf[0] = '\0';

   FILE *fout = NULL;
   // open the output file for this thread
   if (! strlen (KnobOutputFile.Value().c_str()))
   {
      sprintf (fname1, "%.95s-:%d:%s%s.mrd%c", MIAMIU::StripPath(img1->Name().c_str()), KnobLineSize.Value(), 
                    rankbuf, pidbuf,
                    (MIAMI_MEM_REUSE::text_mode_output?'t':'b'));
   } else
   {
      sprintf (fname1, "%.215s-:%d:%s%s.mrd%c", KnobOutputFile.Value().c_str(), KnobLineSize.Value(),
                    rankbuf, pidbuf,
                    (MIAMI_MEM_REUSE::text_mode_output?'t':'b'));
   }
   
   fout = fopen (fname1, "wb");
   if (!fout)
   {
      fprintf (stderr, "MIAMI::memreuse::ThreadFini: ERROR, cannot open output file %s. Profile data will not be saved.\n", fname1);
   }

   MIAMI_MEM_REUSE::FinalizeThreadData(PIN_GetThreadData(MIAMI_MEM_REUSE::thr_key, tid), fout);

   PIN_SetThreadData(MIAMI_MEM_REUSE::thr_key, 0, tid);
   
   if (fout)
      fclose(fout);
}
/* ===================================================================== */

int 
main (int argc, char *argv[])
{
//    PIN_InitSymbolsAlt(IFUNC_SYMBOLS);
    PIN_InitSymbols();

    if (PIN_Init (argc,argv))
    {
        return Usage();
    }
    MIAMI_MEM_REUSE::profileErrors = KnobScopeErrors.Value();
    MIAMI_MEM_REUSE::verbose_level = KnobVerbosityLevel.Value();
    MIAMI_MEM_REUSE::text_mode_output = KnobTextMode.Value();
    
    if (KnobLineSize.Value()<4 || !MIAMIU::IsPowerOf2(KnobLineSize.Value()))
    {
        fprintf(stderr, "Invalid block size of %d. Memory block size must be a power of 2 greater or equal to 4.\n", 
               KnobLineSize.Value());
        return (-2);
    } else
    {
       MIAMI_MEM_REUSE::addr_shift = MIAMIU::FloorLog2(KnobLineSize.Value());
    }
    if (KnobBlockMask.Value())  // there cannot be any sampling if the mask is zero
    {
       MIAMI_MEM_REUSE::use_sampling = 1;
       MIAMI_MEM_REUSE::addr_mask = KnobBlockMask.Value() << MIAMI_MEM_REUSE::addr_shift;
       MIAMI_MEM_REUSE::addr_value = KnobBlockValue.Value() << MIAMI_MEM_REUSE::addr_shift;
       if (MIAMI_MEM_REUSE::verbose_level > 0)
       {
          cout << "Using sampling with addr_mask=0x" << hex << MIAMI_MEM_REUSE::addr_mask
               << " and addr_value=0x" << MIAMI_MEM_REUSE::addr_value << dec << endl;
          cout << "addr_shift=" << MIAMI_MEM_REUSE::addr_shift << endl;
       }

       if (KnobUseScratchRegs.Value())
       {
          // We were asked to use scratch registers to store the address mask and value
          MIAMI_MEM_REUSE::scratch_reg0 = PIN_ClaimToolRegister();
          MIAMI_MEM_REUSE::scratch_reg1 = PIN_ClaimToolRegister();
          if (! (REG_valid(MIAMI_MEM_REUSE::scratch_reg0) && REG_valid(MIAMI_MEM_REUSE::scratch_reg1)) )
          {
             std::cout << "Cannot allocate scratch registers for sampling. Continuing without them.\n";
             MIAMI_MEM_REUSE::use_scratch_reg = 0;
          } else
          {
             if (MIAMI_MEM_REUSE::verbose_level > 0)
                std::cout << "Using scratch registers for sampling.\n";
             MIAMI_MEM_REUSE::use_scratch_reg = 1;
          }
       }
    }

    MIAMI::MIAMI_OnDemandInit();

    MIAMI_MEM_REUSE::maxImgs = 16;
    MIAMI_MEM_REUSE::allImgs = (MIAMI::LoadModule**) malloc (MIAMI_MEM_REUSE::maxImgs * sizeof (MIAMI::LoadModule*));
    for (uint32_t i=0 ; i<MIAMI_MEM_REUSE::maxImgs ; ++i)
       MIAMI_MEM_REUSE::allImgs[i] = 0;

    if (MIAMI_MEM_REUSE::verbose_level > 0)
    {
       cout << "Sizeof MemRefRecord: " << sizeof(struct MIAMI_MEM_REUSE::MemRefRecord) << endl
            << " - offset of ea: " << offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, ea) << endl
            << " - offset of iidx: " << offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, iidx) << endl
            << " - offset of field32: " << offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, u.field32) << endl
            << " - offset of info: " << offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, u.info) << endl
            << " - offset of info.type: " << offsetof(struct MIAMI_MEM_REUSE::MemRefRecord, u.info.type) << endl;

       cout << "Sizeof memref_uint32_pack: " << sizeof(struct MIAMI_MEM_REUSE::MemRefRecord::memref_uint32_pack) << endl
            << " - offset of imgid: " << offsetof(struct MIAMI_MEM_REUSE::MemRefRecord::memref_uint32_pack, imgid) << endl
            << " - offset of type: " << offsetof(struct MIAMI_MEM_REUSE::MemRefRecord::memref_uint32_pack, type) << endl
            << " - offset of aux: " << offsetof(struct MIAMI_MEM_REUSE::MemRefRecord::memref_uint32_pack, aux) << endl;
    }
    
    // Initialize the memory reference buffer
    MIAMI_MEM_REUSE::bufId = PIN_DefineTraceBuffer(sizeof(struct MIAMI_MEM_REUSE::MemRefRecord), NUM_BUF_PAGES,
                                  BufferFull, 0);

    if(MIAMI_MEM_REUSE::bufId == BUFFER_ID_INVALID)
    {
        cerr << "Error: could not allocate initial buffer" << endl;
        return 1;
    }
    
    MIAMI_MEM_REUSE::compute_footprint = KnobFootprint.Value();
    MIAMI_MEM_REUSE::compute_carry = (KnobCarryScope.Value() || MIAMI_MEM_REUSE::compute_footprint);

    // Initialize thread-specific data not handled by buffering api.
    MIAMI_MEM_REUSE::thr_key = PIN_CreateThreadDataKey(0);
   
    // add thread callbacks
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);

    IMG_AddInstrumentFunction (ImageLoad, 0);
    TRACE_AddInstrumentFunction (AnalyzeTrace, 0);
    
    PIN_AddFiniFunction (Fini, 0);

    /* initialize the architecture dependent instruction decoding */
    MIAMI::instruction_decoding_init(NULL);
    
    // Never returns
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
