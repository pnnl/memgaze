/*
 * This file is part of the MIAMI framework. For copyright information, see    
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: cachetool.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * The control manager of a cache simulation PIN tool.
 */

#include <pin.H>

#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <iostream>
#include <fstream>

#include "cache_miami.h"
#include "miami_utils.h"
#include "load_module.h"

#define KILO 1024
#define MEGA (KILO*KILO)
#define GIGA (KILO*MEGA)

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile (KNOB_MODE_WRITEONCE,    "pintool",
    "o", "", "specify cache sim output file name prefix, default - execname-size.line.assoc");
KNOB<BOOL> KnobLongSummary (KNOB_MODE_WRITEONCE,    "pintool",
    "s", "0", "detailed summary - print a summary for each cache set (default - print only a summary for the entire cache)");
KNOB<BOOL> KnobTrackReferences (KNOB_MODE_WRITEONCE,    "pintool",
    "r", "0", "track and report information about each reference");
KNOB<UINT32> KnobCacheSize (KNOB_MODE_WRITEONCE, "pintool",
    "c","64", "cache size in kilobytes");
KNOB<UINT32> KnobLineSize (KNOB_MODE_WRITEONCE, "pintool",
    "b","64", "cache block size in bytes");
KNOB<UINT32> KnobAssociativity (KNOB_MODE_WRITEONCE, "pintool",
    "a","4", "cache associativity (1 for direct mapped, 0 for fully-associative)");
KNOB<BOOL> KnobNoConflictMisses (KNOB_MODE_WRITEONCE,    "pintool",
    "n", "0", "do not simulate conflict misses (faster simulation)");
KNOB<BOOL>   KnobNoPid (KNOB_MODE_WRITEONCE,    "pintool",
    "no_pid", "0", "do not append process PID to output file name");

#include "on_demand_pin.h"
#include "use_mpi_rank.h"

/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This is the MIAMI cache simulator.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}
/* ===================================================================== */


#define NUM_BUF_PAGES 8

using MIAMI::addrtype;

namespace MIAMI_CACHE_SIM
{
   extern MIAMI::LoadModule **allImgs;
   extern uint32_t maxImgs;
   extern uint32_t loadedImgs;
   
   BUFFER_ID bufId;
   TLS_KEY   thr_key;
}
//uint64_t numSimulatedInst = 0;  // this should be maintained per thread

/* ===================================================================== */

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
   if (id>=MIAMI_CACHE_SIM::maxImgs || MIAMI_CACHE_SIM::allImgs[id]==0)
   {
      fprintf (stdout, "Found instruction in image that has not been loaded.\n");
      return;
   }
   myimg = MIAMI_CACHE_SIM::allImgs [id];

   MIAMI_CACHE_SIM::MemRefRecord::memref_uint32_u val;
   val.info.imgid = id;

   // Visit every basic block in the trace
   for (BBL bbl=TRACE_BblHead(trace) ; BBL_Valid(bbl); bbl=BBL_Next(bbl))
   {
      for (INS ins=BBL_InsHead(bbl) ; INS_Valid(ins) ; ins=INS_Next(ins))
      {
         UINT32 memOperands = INS_MemoryOperandCount(ins);
         if (memOperands>0)
         {
            ADDRINT iaddr = INS_Address (ins);

            // Iterate over each memory operand of the instruction.
            for (UINT32 memOp=0 ; memOp<memOperands ; ++memOp)
            {
               if (INS_MemoryOperandIsRead(ins, memOp))
               {
                  int32_t index = myimg->getInstIndex (iaddr, memOp);
                  val.info.type = 0;  // we only have loads and stores; load=0, store=1
               
                  INS_InsertFillBufferPredicated (ins, IPOINT_BEFORE, MIAMI_CACHE_SIM::bufId,
                         IARG_MEMORYOP_EA, memOp, offsetof(struct MIAMI_CACHE_SIM::MemRefRecord, ea),
                         IARG_UINT32, index, offsetof(struct MIAMI_CACHE_SIM::MemRefRecord, iidx),
                         IARG_UINT32, val.field32, offsetof(struct MIAMI_CACHE_SIM::MemRefRecord, u.field32),
                         IARG_END);
               }  /* if the operand is Read */
            }  /* loop over memory operands */

            // Repeat the loop over memory operands for stores, The execute after all the loads
            for (UINT32 memOp=0 ; memOp<memOperands ; ++memOp)
            {
               if (INS_MemoryOperandIsWritten(ins, memOp))
               {
                  int32_t index = myimg->getInstIndex (iaddr, memOp);
                  val.info.type = 1;  // we only have loads and stores; load=0, store=1
               
                  INS_InsertFillBufferPredicated (ins, IPOINT_BEFORE, MIAMI_CACHE_SIM::bufId,
                         IARG_MEMORYOP_EA, memOp, offsetof(struct MIAMI_CACHE_SIM::MemRefRecord, ea),
                         IARG_UINT32, index, offsetof(struct MIAMI_CACHE_SIM::MemRefRecord, iidx),
                         IARG_UINT32, val.field32, offsetof(struct MIAMI_CACHE_SIM::MemRefRecord, u.field32),
                         IARG_END);
               }  /* if operand is writen */
            }  /* second loop over memory operands */
            
         }  // if memOperands > 0
      }  // for each INS in block
   }  // for each BBL
}
/* ===================================================================== */

VOID ImageLoad(IMG img, VOID *v)
{
   uint32_t id = IMG_Id (img);
   MIAMI::MIAMI_OnDemandImgLoad(img);
   MIAMI::MIAMI_MpiRankImgLoad(img);

   if (id>=MIAMI_CACHE_SIM::maxImgs)  // I need to extend my array
   {
      uint32_t oldSize = MIAMI_CACHE_SIM::maxImgs;
      while (MIAMI_CACHE_SIM::maxImgs <= id)
         MIAMI_CACHE_SIM::maxImgs <<= 1;
      MIAMI_CACHE_SIM::allImgs = (MIAMI::LoadModule**) realloc (MIAMI_CACHE_SIM::allImgs, MIAMI_CACHE_SIM::maxImgs*sizeof (MIAMI::LoadModule*));
      for (uint32_t i=oldSize ; i<MIAMI_CACHE_SIM::maxImgs ; ++i)
         MIAMI_CACHE_SIM::allImgs[i] = 0;
   }
   MIAMI::LoadModule* & newimg = MIAMI_CACHE_SIM::allImgs [id];
   if (newimg)   // should be NULL I think
      assert (!"Loading image with id seen before");

   MIAMI::addrtype load_offset = IMG_LoadOffset(img);
   newimg = new MIAMI::LoadModule (id, load_offset, IMG_LowAddress(img)-load_offset, IMG_Name(img)); 
   ++ MIAMI_CACHE_SIM::loadedImgs;
}
/* ===================================================================== */

VOID Fini (int code, VOID * v)
{
   unsigned int i;
   MIAMI::MIAMI_OnDemandFini();
   MIAMI_CACHE_SIM::PrintGlobalStatistics();
   
   // deallocate the info stored for each image
   for (i=0 ; i<MIAMI_CACHE_SIM::maxImgs ; ++i)
      if (MIAMI_CACHE_SIM::allImgs[i])
         delete (MIAMI_CACHE_SIM::allImgs[i]);
   free (MIAMI_CACHE_SIM::allImgs);
}
/* ===================================================================== */

VOID ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    // A thread will need to look up its local data, so save pointer in TLS
    PIN_SetThreadData(MIAMI_CACHE_SIM::thr_key, MIAMI_CACHE_SIM::CreateThreadLocalData(tid), tid);
}
/* ===================================================================== */

VOID ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
   char fname1[256], fname2[256], rankbuf[32], pidbuf[32];
   
   if (! KnobNoMpiRank.Value())
      sprintf(rankbuf, "-%05d-%04d", MIAMI::miami_process_rank, tid);
   else
      sprintf(rankbuf, "-%04d", tid);
      
    pid_t procPid = PIN_GetPid();
    if (! KnobNoPid.Value())
       sprintf(pidbuf, "-%05d", procPid);
    else
       pidbuf[0] = '\0';

   // open the output file for this thread
   if (! strlen (KnobOutputFile.Value().c_str()))
   {
      // Find the name of the image with id 1
      MIAMI::LoadModule *img1 = MIAMI_CACHE_SIM::allImgs [1];
      if (! img1)
         assert (! "Fini: We have no data about the image with id 1");
      sprintf (fname1, "%.25s-%uK.%u.%u%s%s.csim", 
              MIAMIU::StripPath(img1->Name().c_str()),
              KnobCacheSize.Value(), KnobLineSize.Value(), 
              KnobAssociativity.Value(), rankbuf, pidbuf);
      sprintf (fname2, "%.25s-%uK.%u.%u%s%s.pxml", 
              MIAMIU::StripPath(img1->Name().c_str()),
              KnobCacheSize.Value(), KnobLineSize.Value(), 
              KnobAssociativity.Value(), rankbuf, pidbuf);
   } else
   {
      sprintf (fname1, "%.230s%s%s.csim", KnobOutputFile.Value().c_str(), rankbuf, pidbuf);
      sprintf (fname2, "%.230s%s%s.pxml", KnobOutputFile.Value().c_str(), rankbuf, pidbuf);
   }

   MIAMI_CACHE_SIM::FinalizeThreadData(PIN_GetThreadData(MIAMI_CACHE_SIM::thr_key, tid), fname1, fname2);

   PIN_SetThreadData(MIAMI_CACHE_SIM::thr_key, 0, tid);
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
#if DEBUG_CACHE_SIM
   fprintf(stderr, "In BufferFull numElems=%" PRIu64 "\n", numElements);
   fflush(stderr);
#endif
   struct MIAMI_CACHE_SIM::MemRefRecord * ref=(struct MIAMI_CACHE_SIM::MemRefRecord*)buf;
   MIAMI_CACHE_SIM::ProcessBufferedElements(numElements, ref,
                  PIN_GetThreadData(MIAMI_CACHE_SIM::thr_key, tid));
   return (buf);
}

/* ===================================================================== */

int 
main (int argc, char *argv[])
{
   PIN_InitSymbols();

   if (PIN_Init (argc,argv))
   {
       return Usage();
   }
   MIAMI::MIAMI_OnDemandInit();

   MIAMI_CACHE_SIM::verbose_level = 0; // KnobVerbosityLevel.Value();
   MIAMI_CACHE_SIM::line_shift = MIAMIU::FloorLog2(KnobLineSize.Value());
   if (MIAMI_CACHE_SIM::verbose_level>0)
   {
      fprintf(stderr, "Pintool LineShift=%d\n", MIAMI_CACHE_SIM::line_shift);
   }

   MIAMI_CACHE_SIM::cache_size = KnobCacheSize.Value() * KILO;
   MIAMI_CACHE_SIM::line_size = KnobLineSize.Value();
   MIAMI_CACHE_SIM::cache_assoc = KnobAssociativity.Value();
   MIAMI_CACHE_SIM::do_conflicts = !KnobNoConflictMisses.Value();
   MIAMI_CACHE_SIM::track_refs = KnobTrackReferences.Value();
   MIAMI_CACHE_SIM::long_dump = KnobLongSummary.Value();
              
   MIAMI_CACHE_SIM::maxImgs = 16;
   MIAMI_CACHE_SIM::allImgs = (MIAMI::LoadModule**)malloc(MIAMI_CACHE_SIM::maxImgs * sizeof (MIAMI::LoadModule*));
   for (uint32_t i=0 ; i<MIAMI_CACHE_SIM::maxImgs ; ++i)
      MIAMI_CACHE_SIM::allImgs[i] = 0;

   // Initialize the memory reference buffer
   MIAMI_CACHE_SIM::bufId = PIN_DefineTraceBuffer(sizeof(struct MIAMI_CACHE_SIM::MemRefRecord), NUM_BUF_PAGES,
                                  BufferFull, 0);

   if (MIAMI_CACHE_SIM::bufId == BUFFER_ID_INVALID)
   {
       cerr << "Error: could not allocate initial buffer" << endl;
       return 1;
   }
   
   // Initialize thread-specific data not handled by buffering api.
   MIAMI_CACHE_SIM::thr_key = PIN_CreateThreadDataKey(0);
   
   // add thread callbacks
   PIN_AddThreadStartFunction(ThreadStart, 0);
   PIN_AddThreadFiniFunction(ThreadFini, 0);

   IMG_AddInstrumentFunction (ImageLoad, 0);
   TRACE_AddInstrumentFunction (AnalyzeTrace, 0);
    
   PIN_AddFiniFunction (Fini, 0);
   
   // Never returns
   PIN_StartProgram();
    
   return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
