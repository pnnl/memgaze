/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: routine.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data bookkeeping and analysis at the routine level for a CFG profiler.
 * Extends the PrivateRoutine implementation with functionality specific
 * to the CFG profiler.
 */

#ifndef MIAMI_CFGTOOL_ROUTINE_H
#define MIAMI_CFGTOOL_ROUTINE_H

#include "miami_types.h"
#include "CFG.h"
#include "private_routine.h"
#include "load_module.h"
#include <map>

namespace MIAMI
{

typedef std::pair<addrtype, addrtype> AddrPair;
// we use this map to record the ranges of address covered by previous traces
typedef std::map<addrtype, AddrPair> TraceMap;

class Routine : public PrivateRoutine
{

public:
   Routine(LoadModule *_img, addrtype _start, usize_t _size, const std::string& _name, addrtype _offset);
   virtual ~Routine();

   virtual int AddBlock(addrtype _start, addrtype _end, 
                  PrivateCFG::NodeType type = PrivateCFG::MIAMI_CODE_BLOCK,
                  int* inEdges=0);
   // add block, but do not create an entry if no incoming edge found
   int AddBlockNoEntry(addrtype _start, addrtype _end, 
                  PrivateCFG::NodeType type = PrivateCFG::MIAMI_CODE_BLOCK,
                  int* inEdges=0);
   
   // we are not using the mechanism with call site returns ...
//   virtual int AddCallSurrogateBlock(addrtype call_addr, addrtype _end, addrtype target,
//               PrivateCFG::EdgeType etype, bool control_returns, PrivateCFG::Edge** pe = 0);
   
   void printProfile(FILE *fout, int genCFGTest = 1) const;
   void printProfile(int genCFGTest = 1) const { printProfile(stdout, genCFGTest); }

   void saveToFile(FILE *fd, addrtype base_addr, LoadModule *myimg, 
               const MIAMI_CFG::count_type *thr_counters, 
               MIAMI_CFG::MiamiCfgStats &stats) const;
   void SetEntryAndTraceCounts(LoadModule *myimg, const MIAMI_CFG::count_type *thr_counters);
   
   // statically determine where we need to place counters; invoked after we
   // discover the CFG
   void MarkCountersPlacement();

   // Record traces seen for this routine. We only store new ranges that were not
   // covered by any previous trace. Returns true if the start address of the
   // trace has not been part of a previous trace, even if other parts of the
   // trace overlap with old traces. Return false if the trace start was part of
   // previous traces, even if part of the trace is new.
   bool RecordTraceRange(addrtype _start, addrtype _end);
   
private:
   TraceMap traceRanges;
   unsigned long counters[NUM_BBL_TYPES];
//   int64_t entered;
};

};

#endif
