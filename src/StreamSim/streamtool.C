/*
 * This file is part of the MIAMI framework. For copyright information, see    
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: streamtool.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements the control management logic for a streaming concurrency tool
 * built on top of PIN. This tool identifies data streaming behavior inside
 * each thread and collects a distribution of the number of active streams at
 * reference level.
 */

#include <pin.H>

#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "miami_types.h"
#include "stream_reuse.h"
#include "load_module.h"
#include "miami_utils.h"

#define KILO 1024
#define MEGA (KILO*KILO)
#define GIGA (KILO*MEGA)

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile (KNOB_MODE_WRITEONCE,    "pintool",
    "o", "", "specify simulation output file name prefix, default - execname-hist_size.table_size-line_size.max_stride");
KNOB<UINT32> KnobCacheSize (KNOB_MODE_WRITEONCE, "pintool",
    "C","6144", "cache size in kilobytes");
KNOB<UINT32> KnobLineSize (KNOB_MODE_WRITEONCE, "pintool",
    "L","64", "cache line size; also granularity for stream strides");
KNOB<UINT32> KnobAssociativity (KNOB_MODE_WRITEONCE, "pintool",
    "A","48", "cache associativity (1 for direct mapped, 0 for fully-associative)");

KNOB<UINT32> KnobMaxStrideSize (KNOB_MODE_WRITEONCE, "pintool",
    "M","3", "maximum allowed stream stride, in lines");
KNOB<UINT32> KnobHistorySize (KNOB_MODE_WRITEONCE, "pintool",
    "H","256", "history table size in entries. A value of 0 disables stream detection.");
KNOB<UINT32> KnobTableSize (KNOB_MODE_WRITEONCE, "pintool",
    "T","128", "stream table size in entries. A value of 0 disables stream detection.");

KNOB<BOOL>   KnobNoPid (KNOB_MODE_WRITEONCE,    "pintool",
    "no_pid", "0", "do not append process PID to output file name");
KNOB<UINT32>   KnobVerbosityLevel (KNOB_MODE_WRITEONCE,    "pintool",
    "debug", "0", "verbosity of debugging messages: 0 - off, 1 - initialization, 2 - add static analysis messages, 3 - add instrumentation events, 4 - add trace of dynamic events (very verbose), 5 - add event processing messages (extremely verbose)");

#include "on_demand_pin.h"
#include "use_mpi_rank.h"

/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This is the MIAMI stream concurrency simulator.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}
/* ===================================================================== */

#define NUM_BUF_PAGES 8

using MIAMI::addrtype;

namespace StreamRD
{
   extern MIAMI::LoadModule **allImgs;
   extern uint32_t maxImgs;
   extern uint32_t loadedImgs;
   
   BUFFER_ID bufId;
   TLS_KEY   thr_key;
}
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
   if (id>=StreamRD::maxImgs || StreamRD::allImgs[id]==0)
   {
      fprintf (stdout, "Found instruction in image that has not been loaded.\n");
      return;
   }
   myimg = StreamRD::allImgs [id];

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
               int32_t index = myimg->getInstIndex (iaddr, memOp);
               //bool isLoad = INS_MemoryOperandIsRead(ins, memOp);
               // I do not differentiate between reads and writes for the purpose
               // of detecting streams
               INS_InsertFillBufferPredicated (ins, IPOINT_BEFORE, StreamRD::bufId,
                      IARG_MEMORYOP_EA, memOp, offsetof(struct StreamRD::MemRefRecord, ea),
                      IARG_UINT32, index, offsetof(struct StreamRD::MemRefRecord, iidx),
                      IARG_UINT32, id, offsetof(struct StreamRD::MemRefRecord, imgId),
                      IARG_END);
            }  // for each memOp
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

   if (id>=StreamRD::maxImgs)  // I need to extend my array
   {
      uint32_t oldSize = StreamRD::maxImgs;
      while (StreamRD::maxImgs <= id)
         StreamRD::maxImgs <<= 1;
      StreamRD::allImgs = (MIAMI::LoadModule**) realloc (StreamRD::allImgs, StreamRD::maxImgs*sizeof (MIAMI::LoadModule*));
      for (uint32_t i=oldSize ; i<StreamRD::maxImgs ; ++i)
         StreamRD::allImgs[i] = 0;
   }
   MIAMI::LoadModule* & newimg = StreamRD::allImgs [id];
   if (newimg)   // should be NULL I think
      assert (!"Loading image with id seen before");

   MIAMI::addrtype load_offset = IMG_LoadOffset(img);
   newimg = new MIAMI::LoadModule (id, load_offset, IMG_LowAddress(img)-load_offset, IMG_Name(img)); 
   ++ StreamRD::loadedImgs;
}
/* ===================================================================== */

VOID Fini (int code, VOID * v)
{
   unsigned int i;
   MIAMI::MIAMI_OnDemandFini();
   StreamRD::PrintGlobalStatistics();
   
   // deallocate the info stored for each image
   for (i=0 ; i<StreamRD::maxImgs ; ++i)
      if (StreamRD::allImgs[i])
         delete (StreamRD::allImgs[i]);
   free (StreamRD::allImgs);
}
/* ===================================================================== */

VOID ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    // A thread will need to look up its local data, so save pointer in TLS
    PIN_SetThreadData(StreamRD::thr_key, StreamRD::CreateThreadLocalData(tid), tid);
}
/* ===================================================================== */

VOID ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
   char fname1[256], rankbuf[32], pidbuf[32];
   
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
      // Find the name of the image with id 1
      MIAMI::LoadModule *img1 = StreamRD::allImgs [1];
      if (! img1)
         assert (! "Fini: We have no data about the image with id 1");
      sprintf (fname1, "%.25s-%u.%u-%u.%u-%u.%u%s%s.srd", 
              MIAMIU::StripPath(img1->Name().c_str()),
              KnobHistorySize.Value(), KnobTableSize.Value(), 
              KnobLineSize.Value(), KnobMaxStrideSize.Value(), 
              KnobCacheSize.Value(), KnobAssociativity.Value(),
              rankbuf, pidbuf);
   } else
   {
      sprintf (fname1, "%.230s%s%s.srd", KnobOutputFile.Value().c_str(), rankbuf, pidbuf);
   }

   fout = fopen (fname1, "wb");
   if (!fout)
   {
      fprintf (stderr, "MIAMI::streamsim::ThreadFini: ERROR, cannot open output file %s. Profile data will not be saved.\n", fname1);
   }

   StreamRD::FinalizeThreadData(PIN_GetThreadData(StreamRD::thr_key, tid), fout);

   PIN_SetThreadData(StreamRD::thr_key, 0, tid);
   
   if (fout)
      fclose(fout);
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
#if DEBUG_STREAM_DETECTION
   fprintf(stderr, "In BufferFull numElems=%" PRIu64 "\n", numElements);
   fflush(stderr);
#endif
   struct StreamRD::MemRefRecord * ref=(struct StreamRD::MemRefRecord*)buf;
   StreamRD::ProcessBufferedElements(numElements, ref,
                  PIN_GetThreadData(StreamRD::thr_key, tid));
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
    
   StreamRD::verbose_level = KnobVerbosityLevel.Value();
   StreamRD::line_shift = MIAMIU::FloorLog2(KnobLineSize.Value());
   if (StreamRD::verbose_level>0)
   {
      fprintf(stderr, "Pintool LineShift=%d\n", StreamRD::line_shift);
   }

   if (KnobTableSize.Value()==0 || KnobHistorySize.Value()==0)
   {
      // disable stream detection
      StreamRD::noStreamSim = 1;
   } else
   {
      StreamRD::stream_table_size = KnobTableSize.Value();
      StreamRD::stream_history_size = KnobHistorySize.Value();
      StreamRD::stream_max_stride = KnobMaxStrideSize.Value();
   }
   StreamRD::stream_cache_size = KnobCacheSize.Value() * KILO;
   StreamRD::stream_line_size = KnobLineSize.Value();
   StreamRD::stream_cache_assoc = KnobAssociativity.Value();
              
   StreamRD::maxImgs = 16;
   StreamRD::allImgs = (MIAMI::LoadModule**) malloc (StreamRD::maxImgs * sizeof (MIAMI::LoadModule*));
   for (uint32_t i=0 ; i<StreamRD::maxImgs ; ++i)
      StreamRD::allImgs[i] = 0;

   // Initialize the memory reference buffer
   StreamRD::bufId = PIN_DefineTraceBuffer(sizeof(struct StreamRD::MemRefRecord), NUM_BUF_PAGES,
                                  BufferFull, 0);

   if(StreamRD::bufId == BUFFER_ID_INVALID)
   {
       cerr << "Error: could not allocate initial buffer" << endl;
       return 1;
   }
    
   // Initialize thread-specific data not handled by buffering api.
   StreamRD::thr_key = PIN_CreateThreadDataKey(0);
   
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
