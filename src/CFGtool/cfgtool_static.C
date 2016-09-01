/*
 * This file is part of the MIAMI framework. For copyright information, see    
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: cfgtool_static.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements the control management logic for a CFG profiler PIN tool.
 * This tool uses static analysis to fully explore the CFG of a routine the 
 * first time we execute it. It also uses a dynamic approach to fill in any
 * missing pieces (unresolved targets of indirect branches) during execution,
 * and to maintain backwards compatibility with the purely dynamic approach.
 */

#include <pin.H>

#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <iostream>
#include <fstream>
#include <vector>

#include "cfg_data.h"
#include "routine.h"
#include "load_module.h"
#include "debug_miami.h"
#include "miami_utils.h"
#include "instruction_decoding.h"

#define KILO 1024
#define MEGA (KILO*KILO)
#define GIGA (KILO*MEGA)

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<UINT32> KnobBufferSize (KNOB_MODE_WRITEONCE,    "pintool",
    "b", STR(INITIAL_BUF_SIZE), "initial size of the thread local counter buffers");
KNOB<string> KnobRoutineName (KNOB_MODE_WRITEONCE,    "pintool",
    "r", "", "specify routine name for which to dump CFG in dot format, default: none");
KNOB<string> KnobOutputFile (KNOB_MODE_WRITEONCE,    "pintool",
    "o", "", "specify name prefix of file where to save discovered CFGs; default: execname");
KNOB<BOOL>   KnobNoPid (KNOB_MODE_WRITEONCE,    "pintool",
    "no_pid", "0", "do not add process PID to output file name");
KNOB<BOOL> KnobPrintStatistics (KNOB_MODE_WRITEONCE,    "pintool",
    "stats", "", "print profile summary statistics at program end.");
KNOB<UINT32>   KnobVerbosityLevel (KNOB_MODE_WRITEONCE,    "pintool",
    "debug", "0", "verbosity of debugging messages: 0 - off, 1 - initialization, 2 - add static analysis messages, 3 - add instrumentation events, 4 - add analysis time messages");
    
#include "on_demand_pin.h"
#include "use_mpi_rank.h"

/* ===================================================================== */

INT32 Usage()
{
    cerr << "This is the MIAMI control flow graph (CFG) profiler." << endl
         << "This tool profiles the CFGs of all executed routines." << endl
         << endl;

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}
/* ===================================================================== */

#define STRINGIZE(a)  #a

struct opInfo
{
    const char * name;                          /* Instruction name (we could get it from XED, but we nearly have it anyway) */
    UINT32 opcode;                              /* Opcode enumeration from XED */
    UINT32 reads;                               /* Number of reads per iteration */
    UINT32 writes;                              /* Number of writes per iteration */
    UINT32 size;                                /* Size of the memory access(es) at each iteration */
};

// Expand the names and properties of an instruction for all possible widths.
#define EXPAND_OPCODE(op, r, w)                         \
    { STRINGIZE(op##B), XED_ICLASS_##op##B, r, w, 1 },  \
    { STRINGIZE(op##W), XED_ICLASS_##op##W, r, w, 2 },  \
    { STRINGIZE(op##D), XED_ICLASS_##op##D, r, w, 4 },  \
    { STRINGIZE(op##Q), XED_ICLASS_##op##Q, r, w, 8 }

// Instructions which can be REP prefixed (we ignore I/O operations!)  We
// encode knowledge of the number of reads and writes each op performs
// here. We could determine this dynamically from INS_IsMemoryRead,
// INS_HasMemoryRead2, INS_IsMemoryWrite, but since we're special
// casing these instructions anyway, we may as well just use our
// knowledge. (Code for doing it the general way is in instrumentTrace,
// where we don't know which instructions we're dealing with).
//
// Order here matters, we test specifically for CMPS and SCAS based
// on their position in this table...
static const opInfo opcodes[] = {
    EXPAND_OPCODE(CMPS,2,0),                    /* two reads, no writes */
    EXPAND_OPCODE(SCAS,1,0),                    /* one read,  no writes */
    EXPAND_OPCODE(MOVS,1,1),                    /* one read,  one write */
    EXPAND_OPCODE(STOS,0,1),                    /* no reads,  one write */
    EXPAND_OPCODE(LODS,1,0),                    /* one read,  no writes */
};

#define NumOps (5*4)                            /* Five instructions times four lengths */

// Does the instrution have a REPZ/REPNZ prefix, or is the length solely determined by
// the value of the count register?
//
// If KnobSlow has been asserted we pretend that we have to work the slow way with
// all instructions so that we can measure the benefit of being smarter...
static BOOL takesConditionalRep(UINT32 opIdx)
{
    return opIdx < (2*4);                       /* CMPS and SCAS are the first two sets of instructions */
}

// Convert an opcode into an index in our tables.
static UINT32 opcodeIndex(UINT32 opcode)
{
    for (UINT32 i=0; i<NumOps; i++)
        if (opcodes[i].opcode == opcode)
            return i;

    ASSERT (FALSE,"Missing instruction " + decstr(opcode) + "\n");
    return 0;
}
/* ===================================================================== */

#define NUM_BUF_PAGES 8

using MIAMI::addrtype;

namespace MIAMI_CFG
{
   typedef std::vector<int32_t> IndexVector;
   
   extern MIAMI::LoadModule **allImgs;
   extern uint32_t maxImgs;
   extern uint32_t loadedImgs;
   
   BUFFER_ID bufId;
   TLS_KEY   thr_key;
   PIN_LOCK  cfgLock;
   PIN_LOCK  bufferLock;
   REG       buf_start;
   
   // define a second buffer limit; This one differs from the actual buffer size only
   // during the process of increasing buffer sizes
   // this variable should be modified only while holding the bufferLock lock
   static int32_t target_buf_size = INITIAL_BUF_SIZE;  
}
/* ===================================================================== */

static ADDRINT isZeroCount (UINT32 repCount, BOOL firstIter)
{
    return (firstIter && repCount==0);
}
/* ===================================================================== */

static ADDRINT isNonZeroCount (UINT32 repCount, BOOL firstIter)
{
    return (firstIter && repCount!=0);
}
/* ===================================================================== */

static ADDRINT isExecuting (BOOL executing, BOOL firstIter)
{
    return (executing && !firstIter);
}
/* ===================================================================== */

static ADDRINT isNotExecuting (BOOL executing)
{
    return (!executing);
}
/* ===================================================================== */

static VOID doCount(UINT32 idx, MIAMI_CFG::count_type *counters)
{ 
   counters[idx] += 1;
}
/* ===================================================================== */

// Analysis functions for execution counts.
// Analysis routine, FirstRep and Executing tell us the properties of the execution.
static VOID doRepCount (UINT32 idx, MIAMI_CFG::count_type *counters, 
          UINT32 repCount, UINT32 firstRep)
{
    counters[idx] += (repCount - firstRep);
}
/* ===================================================================== */

static VOID growBuffers (THREADID tid, int32_t maxIndex, ADDRINT restart_pc,
          CONTEXT *crtctxt)
{
   bool masterThread = false;
   int32_t nDoubling = 0;
   PIN_GetLock(&MIAMI_CFG::bufferLock, tid+1);
   // if the target buf size is larger than the received maxIndex, or
   // if the target and the current buf sizes differe, it means we are in 
   // the process we updating thread buffers. Another thread must be working
   // on this and is waiting for us to finish our trace.
   // We exit with a cache invalidate and ExecuteAt so we can re-instrument 
   // the trace.
   if (maxIndex >= MIAMI_CFG::target_buf_size && 
              MIAMI_CFG::target_buf_size==MIAMI_CFG::thr_buf_size)
   {
      masterThread = true;
      PIN_GetLock(&MIAMI_CFG::cfgLock, tid+1);
      int32_t numCounters = MIAMI::LoadModule::GetHighWaterMarkIndex(); // get the current value
      while (numCounters >= MIAMI_CFG::target_buf_size)
      {
         // double the size of the buffer
         MIAMI_CFG::target_buf_size <<= 1;
         nDoubling += 1;
      }
      
      if (MIAMI_CFG::verbose_level > 3)
         fprintf(stderr, "Thread %d is the master thread growing buffers from %d to %d. Passed/current maxIndex=%d/%d. Restart PC=0x%lx\n",
              tid, MIAMI_CFG::thr_buf_size, MIAMI_CFG::target_buf_size, 
              maxIndex, numCounters, restart_pc);
      
      // when I release this lock, other threads will be able to potentially create
      // many new indices; they will be smaller than the tentative buffer size, but 
      // the buffers are not actually grown at this point. 
      // The actual buffers and buffer size have not been updated yet.
      PIN_ReleaseLock(&MIAMI_CFG::cfgLock);
   }
   PIN_ReleaseLock(&MIAMI_CFG::bufferLock);
   if (masterThread)   // Now, I must block all threads and update their state
   {
      bool stopped_ok = PIN_StopApplicationThreads(tid);
      int32_t num_stopped = PIN_GetStoppedThreadCount();
      if (MIAMI_CFG::verbose_level > 3)
         fprintf(stderr, "Thread %d attempted to stop all threads, success=%d, num stopped threads=%d\n",
              tid, stopped_ok, num_stopped);
      if (! stopped_ok)  // oops, something went bad
      {
         if (MIAMI_CFG::verbose_level > 3)
            fprintf(stderr, "Thread %d failed to stop all the other threads. Is any other thread attempting the same thing???\n",
                   tid);
         // return;   // is this right?? Should I call ExecuteAt instead? FIXME
         // Just fall-thru to the default behavior: cache invalidate + ExecuteAt
      } else   // we successfully stopped all other threads
      {
         // Acquire the cfgLock here??? Everybody should be stopped right now. 
         // Only internal tool threads might still run, but we do not use any.
         // How about callbacks? The one for starting threads is especially important.
         PIN_GetLock(&MIAMI_CFG::cfgLock, tid+1);
         
         // update the buffers' size first
         MIAMI_CFG::thr_buf_size = MIAMI_CFG::target_buf_size;
         
         // iterate over each thread (including myself) and change their contexts
         for (int32_t i=0 ; i<=num_stopped ; ++i)
         {
            THREADID rid = 0;
            CONTEXT *ctxt = 0;
            if (i<num_stopped)  // one of the other threads, now stopped
            {
               rid = PIN_GetStoppedThreadId(i);
               // get a modifiable context
               ctxt = PIN_GetStoppedThreadWriteableContext(rid);
            } else
            {
               rid = tid;
               ctxt = crtctxt;
            }
            if (MIAMI_CFG::verbose_level > 3)
               fprintf(stderr, " - %d/%d: thread %d is handling (stopped) thread with ID %d",
                     i, num_stopped, tid, rid);
            // First resize its buffer
            void *tdata = PIN_GetThreadData(MIAMI_CFG::thr_key, rid);
            MIAMI_CFG::count_type *newbuf = MIAMI_CFG::IncreaseThreadCountersBuffer(tdata);
            PIN_SetContextReg(ctxt, MIAMI_CFG::buf_start, (ADDRINT)newbuf);
            if (MIAMI_CFG::verbose_level > 3)
               fprintf(stderr, ", new buffer starts at %p\n", newbuf);
            if (i == num_stopped)
               MIAMI_CFG::RecordBufferIncrease(tdata, nDoubling);  // just keeping stats
            
            // We are processing indirect jumps differently now.
            // No need to check the scratch register, which is unused 
            // and should be removed.
         }  // for each (stopped) thread
         
         PIN_ReleaseLock(&MIAMI_CFG::cfgLock);
         // do not forget to resume all threads
         PIN_ResumeApplicationThreads(tid);
      }  // we successfully stopped the threads
   }  // if master thread
   else
   {
      if (MIAMI_CFG::verbose_level > 3)
         fprintf(stderr, "Thread %d LOST the master race to grow buffers to %d. Passed/current maxIndex=%d/%d. Restart PC=0x%lx. Will yield CPU briefly.\n",
              tid, MIAMI_CFG::target_buf_size, maxIndex, 
              MIAMI::LoadModule::GetHighWaterMarkIndex(), restart_pc);
      PIN_Yield();
   }
      
   // This analysis function can be called only at the very start of a trace before the first
   // instruction executes, and it should be the very first analysis routine called for that trace.
   // At the end, everybody should flush the code cache associated with their trace
   // and then call ExecuteAt to reinstrument / reexecute it using the new larger buffers.
   // Invalidate the code cache so that we do not call this analysis routine next time 
   // we execute the trace
   assert(restart_pc == PIN_GetContextReg(crtctxt, REG_INST_PTR));
   CODECACHE_InvalidateTraceAtProgramAddress(restart_pc);
   PIN_ExecuteAt(crtctxt);
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
   if (id>=MIAMI_CFG::maxImgs || MIAMI_CFG::allImgs[id]==0)
   {
      fprintf (stdout, "Found instruction in image which has not been loaded.\n");
      return;
   }
   myimg = MIAMI_CFG::allImgs [id];
   ADDRINT entry = TRACE_Address(trace);  // entry address into the routine
   ADDRINT traceend = entry + TRACE_Size(trace);

   ADDRINT rStart = RTN_Address(rtn);
   const ADDRINT raddr = rStart - myimg->BaseAddr();

   // another bug (or bad design) in PIN. PIN limits routine sizes to 200,000 bytes.
   // For larger routines, it reports their sizes correctly, but then it creates
   // an overlapped routine for whatever is beyond that size limit.
   // To work around this, I keep track of routine spans in my tool.
   // Check if current trace start is part of an already created MIAMI routine first.
   MIAMI::Routine* robj = myimg->HasRoutineAtPC(entry);
   
   if (! robj)  // we do not have a routine for this PC, create one
      // for some libraries, it can happen that local symbols are stripped.
      // Thus, ranges of addresses do not belong do any known routine.
      // PIN assigns such traces to the previous symbol. Must catch this case.
      robj = myimg->HasRoutineAtPC(rStart);

   PIN_GetLock(&MIAMI_CFG::cfgLock, 0);
   if (! robj)
   {
      // search by using routine offset inside the image; image can be loaded at diff address
      MIAMI::Routine* &rref = myimg->GetRoutineAt(raddr);
      assert (!rref);
      
      rref = new MIAMI::Routine(myimg, rStart, RTN_Size(rtn), RTN_Name(rtn), raddr);
      // decode instructions from the start of the trace, and try to discover 
      // the control flow inside the routine
      rref->discover_CFG(entry);  // statically discover the CFG starting from entry
      if (rStart!=entry)          // if entry address is different than routine start
         rref->discover_CFG(rStart);  // discover CFG starting from routine start as well
      // separate CFG discovery from counter placement; I might recover the CFG 
      // in other situations where I do not care about counters, but we want to
      // perform different analysis
      rref->MarkCountersPlacement();
      
      // Dump the static CFG if this is the tracked routine
      if (!KnobRoutineName.Value().compare(rref->Name()))
         rref->CfgToDot();
      robj = rref;
      myimg->AddRoutine(robj);
   }
   
   // Visit every basic block  in the trace
   int bidx = 0;  // keep track of the BBL index
   for (BBL bbl=TRACE_BblHead(trace); BBL_Valid(bbl); bbl=BBL_Next(bbl), ++bidx)
   {
      ADDRINT start_addr = BBL_Address(bbl);
      ADDRINT end_addr = start_addr + BBL_Size(bbl);
      INS lastIns = BBL_InsTail(bbl);

      MIAMI::CFG::NodeType btype = MIAMI::CFG::MIAMI_CODE_BLOCK;
      if (BBL_NumIns(bbl)==1 && INS_Stutters(lastIns))   // a REP instruction
      {
         btype = MIAMI::CFG::MIAMI_REP_BLOCK;
         assert(INS_Size(lastIns) > 1);
      }
      robj->AddBlock(start_addr, end_addr, btype);
      
      if (bidx == 0)  // do these tests here, so we at least have the block for the trace start
      {
         if (robj->RecordTraceRange(entry, traceend) && 
                // add also a counter for traces when no incoming edges were present 
                // because we removed routine entries (pass strict = false)
                robj->MayFollowCallSite(entry, false))
         {
            // new trace start, must insert a counter here
            robj->addTraceCounter(entry);
         }
         addrtype del_addr=0;
         // remove any recorded trace starts between entry+1 and end of current trace
         int nremoved = robj->DeleteAnyTraceStartsInRange(entry+1, traceend, del_addr); // exclusive end
         // invalidate the code cache to remove unnecessary trace instrumentation
         if (nremoved==1)  
            CODECACHE_InvalidateTraceAtProgramAddress(del_addr);
         else if (nremoved)
            CODECACHE_InvalidateRange(entry+1, traceend-1);  // inclusive end
      }
      
      // test if the last instruction of a BBL is a branch/call and creates the potential
      // for edges. Fall-thru edges as well as direct jumps.
      if (!INS_Valid(lastIns))
         continue;
      // test the fallthru on the last instruction. If tested on the bbl, it returns false if
      // the bbl is last in the trace
      // if this is the last bbl of the trace and the last instruction is not a branch, 
      // do not add a fall-thru edge here. In fact, I should not even add the block at all
      // at this point, as it is incomplete.
      MIAMI::CFG::Edge *ee = NULL;
      if (INS_HasFallThrough(lastIns))
      {
         ee = static_cast<MIAMI::CFG::Edge*>(robj->AddPotentialEdge(end_addr, end_addr, 
                             MIAMI::PrivateCFG::MIAMI_FALLTHRU_EDGE));
      }

      // check if we had an entry inside this block
      ADDRINT entry_addr;
      if (bidx==0 && robj->HasTraceStartAt(start_addr))
      {
         int32_t index = myimg->GetTraceStartIndex(start_addr);
         BBL_InsertCall(bbl, IPOINT_ANYWHERE, (AFUNPTR)doCount,
                       IARG_CALL_ORDER, CALL_ORDER_DEFAULT,
                       IARG_UINT32, index,
                       IARG_REG_VALUE, MIAMI_CFG::buf_start,
                       IARG_END);
      } 
      // if we found a trace start at start_addr, we shouldn't add a new
      // counter for an entry at the same address.
      // I think this optimization gives us very little, because I do not insert
      // trace starts at the routine start address, which covers most entries.
      // But, can we have multiple entries in a given BBL?? I think it can happen.
      // Ignore for now.
      if (robj->HasEntryInRange(start_addr, end_addr, entry_addr))
      {
         int32_t index = myimg->GetRoutineEntryIndex(entry_addr);
         BBL_InsertCall(bbl, IPOINT_ANYWHERE, (AFUNPTR)doCount,
                       IARG_CALL_ORDER, CALL_ORDER_DEFAULT,
                       IARG_UINT32, index,
                       IARG_REG_VALUE, MIAMI_CFG::buf_start,
                       IARG_END);
      }

      if (INS_Stutters(lastIns))   // a REP instruction, add a potential edge to itself
      {
         // Alternatively, use INS_HasRealRep(ins), this is architecture specific
         UINT32 opIdx = opcodeIndex(INS_Opcode(lastIns));
         // I must create a loop back edge around the rep instruction, but also
         // a bypass edge that skips the rep instruction (counted if rep count is 0)
         // First, must split the original REP node into two parts.
         // A REP prefix node where the bypass edge will start, and a regular code
         // node for the loop back edge
         addrtype repstart = INS_Address(lastIns);
         if (BBL_NumIns(bbl)>1)  // our original block included more stuff
            // split first into a REP block for the entire REP prefixed instruction
            robj->AddBlockNoEntry(repstart, end_addr, MIAMI::CFG::MIAMI_REP_BLOCK);
         // Then, split the instruction intself into a separate block
//         robj->AddBlockNoEntry(repstart+1, end_addr, MIAMI::CFG::MIAMI_CODE_BLOCK);
         
         ee = static_cast<MIAMI::CFG::Edge*>(robj->AddPotentialEdge(end_addr, repstart, 
                     MIAMI::CFG::MIAMI_DIRECT_BRANCH_EDGE));
         MIAMI::CFG::Edge *ee2 = static_cast<MIAMI::CFG::Edge*>(robj->AddPotentialEdge(repstart, 
                     end_addr, MIAMI::CFG::MIAMI_BYPASS_EDGE));
         ee2->setCounterEdge();

         int32_t index = myimg->GetCFGEdgeIndex(ee);
         int32_t index2 = myimg->GetCFGEdgeIndex(ee2);
         // When we get to execute the rep instruction, it will be separated into
         // its own trace/bbl. Check if current BBL has only 1 instruction. That means
         // that the rep instruction has been aleady broken into its own BBL.
         if (takesConditionalRep(opIdx))
         {
             // We have no smart way to lessen the number of
             // instrumentation calls because we can't determine when
             // the conditional instruction will finish.  So we just
             // let the instruction execute and have our
             // instrumentation be called on each iteration.  This is
             // the simplest way of handling REP prefixed instructions, where
             // each iteration appears as a separate instruction, and
             // is independently instrumented.
             //
             // Even if the REP instruction is executing, we do not increment it on
             // its first iteration (a count of 1 will flow from the incoming edge).
             INS_InsertIfCall(lastIns, IPOINT_BEFORE, (AFUNPTR)isExecuting, 
                         IARG_EXECUTING, IARG_FIRST_REP_ITERATION,
                         IARG_END);
             INS_InsertThenCall(lastIns, IPOINT_BEFORE, (AFUNPTR)doRepCount,
                         IARG_UINT32, index,
                         IARG_REG_VALUE, MIAMI_CFG::buf_start,
                         IARG_UINT32, 1,
                         IARG_FIRST_REP_ITERATION,
                         IARG_END);
             INS_InsertIfCall(lastIns, IPOINT_BEFORE, (AFUNPTR)isNotExecuting, 
                                IARG_EXECUTING, IARG_END);
             INS_InsertThenCall(lastIns, IPOINT_BEFORE, (AFUNPTR)doCount,
                         IARG_UINT32, index2,
                         IARG_REG_VALUE, MIAMI_CFG::buf_start,
                         IARG_END);
         }
         else
         {
             // The number of iterations is determined solely by the count register value,
             // therefore we can log all we need at the start of each REP "loop", and skip the
             // instrumentation on all the other iterations of the REP prefixed operation. Simply use
             // IF/THEN instrumentation which tests IARG_FIRST_REP_ITERATION.
             //
             INS_InsertIfCall(lastIns, IPOINT_BEFORE, (AFUNPTR)isNonZeroCount, 
                                IARG_REG_VALUE, INS_RepCountRegister(lastIns),
                                IARG_FIRST_REP_ITERATION, IARG_END);
             INS_InsertThenCall(lastIns, IPOINT_BEFORE, (AFUNPTR)doRepCount,
                                IARG_UINT32, index,
                                IARG_REG_VALUE, MIAMI_CFG::buf_start,
                                IARG_REG_VALUE, INS_RepCountRegister(lastIns),
                                IARG_FIRST_REP_ITERATION,
                                IARG_END);
             INS_InsertIfCall(lastIns, IPOINT_BEFORE, (AFUNPTR)isZeroCount, 
                                IARG_REG_VALUE, INS_RepCountRegister(lastIns),
                                IARG_FIRST_REP_ITERATION, IARG_END);
             INS_InsertThenCall(lastIns, IPOINT_BEFORE, (AFUNPTR)doCount,
                                IARG_UINT32, index2,
                                IARG_REG_VALUE, MIAMI_CFG::buf_start,
                                IARG_END);
         }
      } else
      {
         ADDRINT _target = 0;
         if (INS_IsDirectBranchOrCall(lastIns))  // we can get the target statically
            _target = INS_DirectBranchOrCallTargetAddress(lastIns);

         bool jump_returns = false, is_call = false;
         MIAMI::PrivateCFG::EdgeType etype = MIAMI::PrivateCFG::MIAMI_DIRECT_BRANCH_EDGE;
         if (INS_IsProcedureCall(lastIns))
         {
#if DEBUG_CFG_DISCOVERY
            LOG_CFG(4,
               fprintf(stderr, "Routine %s, before PC 0x%" PRIxaddr " is function call, target 0x%" PRIxaddr "\n",
                      robj->Name().c_str(), end_addr, _target);
            )
#endif
            jump_returns = true; is_call = true;
            etype = MIAMI::PrivateCFG::MIAMI_FALLTHRU_EDGE;
         }
         
         if (_target || is_call)
         {
            if (is_call) // || _target<robj->Start() || _target>=robj->End())
            {
               // inter-procedural
               // create a call surrogate block
               MIAMI::PrivateCFG::Edge *pe;
               robj->AddCallSurrogateBlock(INS_Address(lastIns), end_addr, _target,
                          etype, jump_returns, &pe);
               ee = static_cast<MIAMI::CFG::Edge*>(pe);
            } else  // inside the routine
            {
               ee = static_cast<MIAMI::CFG::Edge*>(robj->AddPotentialEdge(end_addr, 
                          _target,
                          MIAMI::PrivateCFG::MIAMI_DIRECT_BRANCH_EDGE));
            }
         } 
         else if (INS_IsIndirectBranchOrCall(lastIns))  // insert code to get the target
         {
            if (INS_IsRet(lastIns))
               robj->AddReturnEdge(end_addr);
            else
            {
               // if the end_addr - 1 is outside the routine boundary, explicitly map
               // the routine pointer to this PC to use later when we process the buffer
               if (end_addr >= robj->End())
                  myimg->RoutineAtIndirectPc(end_addr - 1) = robj;
               // Map the block end adress to a 32-bit index that we store in the buffer
               int32_t index = myimg->GetIndexForPC(end_addr);
               INS_InsertFillBufferPredicated (lastIns, IPOINT_TAKEN_BRANCH, MIAMI_CFG::bufId,
                       IARG_UINT32, id, offsetof(struct MIAMI_CFG::BufRecord, imgId),
                       IARG_UINT32, index, offsetof(struct MIAMI_CFG::BufRecord, iidx),
                       IARG_BRANCH_TARGET_ADDR, offsetof(struct MIAMI_CFG::BufRecord, target),
                       IARG_END);
            }
         }
         if (ee && ee->isCounterEdge())
         {
            int32_t index = myimg->GetCFGEdgeIndex(ee);
            INS_InsertCall(lastIns, IPOINT_TAKEN_BRANCH, (AFUNPTR)doCount,
                       IARG_CALL_ORDER, CALL_ORDER_DEFAULT,
                       IARG_UINT32, index,
                       IARG_REG_VALUE, MIAMI_CFG::buf_start,
                       IARG_END);
         }
      }
   }
   int32_t maxIndex = MIAMI::LoadModule::GetHighWaterMarkIndex();
   if (maxIndex >= MIAMI_CFG::thr_buf_size)  // we are exceeding the preallocated buffer size
   {
      // Must insert code to increase the buffer size for all threads.
      // First thread that enters this trace will have to increase the buffers for all threads
      // However, multiple threads may start the trace at the same time.
      // I need to synchronize them. First thread to grab the lock must
      // update the target buffer size to signal the other threads that they must not do
      // anything, then release the lock, stop threads, increase buffers, flush code cache,
      // and restart the trace execution.
      // All threads, after aquiring the lock must check if the target buffer size is really
      // smaller than the passed maxIndex. If not, it means another thread got there first and
      // is in the process of setting things up, including changing registers for current thread.
      // I must quit my trace quickly so all threads can be stopped. Use ExecuteAt on the 
      // first trace instruction. We should not have executed anything from this trace yet.
      // This analysis call should be the first thing executed in this trace. Good Luck.
      if (MIAMI_CFG::verbose_level > 2)
         fprintf(stderr, "AnalyzeTrace: instrumenting trace at %lx, found maxIndex %d greater or equal than current buffer size %d. Insert call to growBuffers\n",
                  entry, maxIndex, MIAMI_CFG::thr_buf_size);
      TRACE_InsertCall(trace, IPOINT_BEFORE, (AFUNPTR)growBuffers,
                    IARG_CALL_ORDER, CALL_ORDER_FIRST,
                    IARG_THREAD_ID,
                    IARG_UINT32, maxIndex,
                    IARG_ADDRINT, entry,
                    IARG_CONTEXT, IARG_END);
   }
   PIN_ReleaseLock(&MIAMI_CFG::cfgLock);
}

/* ===================================================================== */

VOID ImageLoad(IMG img, VOID *v)
{
   uint32_t id = IMG_Id (img);
   MIAMI::MIAMI_OnDemandImgLoad(img);
   MIAMI::MIAMI_MpiRankImgLoad(img);

   if (id>=MIAMI_CFG::maxImgs)  // I need to extend my array
   {
      uint32_t oldSize = MIAMI_CFG::maxImgs;
      while (MIAMI_CFG::maxImgs <= id)
         MIAMI_CFG::maxImgs <<= 1;
      MIAMI_CFG::allImgs = (MIAMI::LoadModule**) realloc (MIAMI_CFG::allImgs, MIAMI_CFG::maxImgs*sizeof (MIAMI::LoadModule*));
      for (uint32_t i=oldSize ; i<MIAMI_CFG::maxImgs ; ++i)
         MIAMI_CFG::allImgs[i] = 0;
   }
   MIAMI::LoadModule* & newimg = MIAMI_CFG::allImgs [id];
   if (newimg)   // should be NULL I think
      assert (!"Loading image with id seen before");

   MIAMI::addrtype load_offset = IMG_LoadOffset(img);
   newimg = new MIAMI::LoadModule (id, load_offset, IMG_LowAddress(img)-load_offset, IMG_Name(img)); 
   ++ MIAMI_CFG::loadedImgs;
}
/* ===================================================================== */

VOID Fini (int code, VOID * v)
{
   unsigned int i;
   MIAMI::MIAMI_OnDemandFini();
   if (MIAMI_CFG::print_stats)
      MIAMI_CFG::PrintGlobalStatistics();
   
   // deallocate the info stored for each image
   for (i=0 ; i<MIAMI_CFG::maxImgs ; ++i)
      if (MIAMI_CFG::allImgs[i])
         delete (MIAMI_CFG::allImgs[i]);
   free (MIAMI_CFG::allImgs);
}
/* ===================================================================== */

VOID ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    // A thread will need to look up its local data, so save pointer in TLS
    // Acquire the cfgLock because we allocate memory based on the value
    // of the thr_buf_size variable
    PIN_GetLock(&MIAMI_CFG::cfgLock, tid+1);
    void *p_tls = MIAMI_CFG::CreateThreadLocalData(tid);
    PIN_SetThreadData(MIAMI_CFG::thr_key, p_tls, tid);
    PIN_SetContextReg(ctxt, MIAMI_CFG::buf_start, 
            (ADDRINT)MIAMI_CFG::GetThreadCountersBuffer(p_tls));
    PIN_ReleaseLock(&MIAMI_CFG::cfgLock);
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

   // open the output file for this thread
   if (! strlen (KnobOutputFile.Value().c_str()))
   {
      // Find the name of the image with id 1
      MIAMI::LoadModule *img1 = MIAMI_CFG::allImgs [1];
      if (! img1)
         assert (! "Fini: We have no data about the image with id 1");
      sprintf (fname1, "%.95s%s%s.cfg", 
              MIAMIU::StripPath(img1->Name().c_str()), rankbuf, pidbuf);
   } else
   {
      sprintf (fname1, "%.230s%s%s.cfg", KnobOutputFile.Value().c_str(), rankbuf, pidbuf);
   }

   PIN_GetLock(&MIAMI_CFG::cfgLock, tid+1);
   MIAMI_CFG::FinalizeThreadData(PIN_GetThreadData(MIAMI_CFG::thr_key, tid), fname1, rankbuf);
   PIN_ReleaseLock(&MIAMI_CFG::cfgLock);

   PIN_SetThreadData(MIAMI_CFG::thr_key, 0, tid);
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
#if DEBUG_CFG_DISCOVERY
   LOG_CFG(4,
      fprintf(stderr, "In BufferFull numElems=%" PRIu64 "\n", numElements);
      fflush(stderr);
   )
#endif
   struct MIAMI_CFG::BufRecord * ref=(struct MIAMI_CFG::BufRecord*)buf;
   // acquire the CFG lock because we are modifying the CFGs.
   PIN_GetLock(&MIAMI_CFG::cfgLock, tid+1);
   MIAMI_CFG::ProcessBufferedElements(numElements, ref,
                  PIN_GetThreadData(MIAMI_CFG::thr_key, tid));
   PIN_ReleaseLock(&MIAMI_CFG::cfgLock);
   return (buf);
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
   MIAMI::MIAMI_OnDemandInit();

   /* initialize the architecture dependent instruction decoding */
   MIAMI::instruction_decoding_init(NULL);

#if 0
   // print LD_LIBRARY_PATH
   char *ldp = getenv("LD_LIBRARY_PATH");
   if (ldp)
      fprintf(stderr, "LD_LIBRARY_PATH=%s\n", ldp);
#endif
    
   MIAMI_CFG::verbose_level = KnobVerbosityLevel.Value();
//   MIAMI_CFG::save_bbls = KnobSaveBBLsToFile.Value();
   MIAMI_CFG::debug_rtn = KnobRoutineName.Value();
   MIAMI_CFG::print_stats = KnobPrintStatistics.Value();
   MIAMI_CFG::thr_buf_size = KnobBufferSize.Value();
   MIAMI_CFG::target_buf_size = KnobBufferSize.Value();
              
   MIAMI_CFG::maxImgs = 16;
   MIAMI_CFG::allImgs = (MIAMI::LoadModule**)malloc(MIAMI_CFG::maxImgs * sizeof (MIAMI::LoadModule*));
   for (uint32_t i=0 ; i<MIAMI_CFG::maxImgs ; ++i)
      MIAMI_CFG::allImgs[i] = 0;

   // reserve a scratch register
   MIAMI_CFG::buf_start = PIN_ClaimToolRegister();
   if (! REG_valid(MIAMI_CFG::buf_start))
   {
      cerr << "Cannot allocate scratch register for local buffer. Cannot continue without it.\n";
      exit (1);
   }

   // Initialize the branch target buffer
   MIAMI_CFG::bufId = PIN_DefineTraceBuffer(sizeof(struct MIAMI_CFG::BufRecord), NUM_BUF_PAGES,
                                  BufferFull, 0);

   if (MIAMI_CFG::bufId == BUFFER_ID_INVALID)
   {
       cerr << "Error: could not allocate branch buffer" << endl;
       return 1;
   }
   
   // Initialize thread-specific data not handled by buffering api.
   MIAMI_CFG::thr_key = PIN_CreateThreadDataKey(0);
   
   // Initialize the CFG and buffer locks
   PIN_InitLock(&MIAMI_CFG::cfgLock);
   PIN_InitLock(&MIAMI_CFG::bufferLock);
   
   // add thread callbacks
   PIN_AddThreadStartFunction(ThreadStart, 0);
   PIN_AddThreadFiniFunction(ThreadFini, 0);

   IMG_AddInstrumentFunction(ImageLoad, 0);
   TRACE_AddInstrumentFunction(AnalyzeTrace, 0);
    
   PIN_AddFiniFunction(Fini, 0);
   
   // Never returns
   PIN_StartProgram();
    
   return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
