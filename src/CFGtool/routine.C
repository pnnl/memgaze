/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: routine.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data bookkeeping and analysis at the routine level for a CFG profiler.
 * Extends the PrivateRoutine implementation with functionality specific
 * to the CFG profiler.
 */

#include <assert.h>
#include <string.h>
#include "routine.h"

using namespace std;
using namespace MIAMI;

#define DEBUG_BBLs 0

#if DEBUG_BBLs
static const char *debugName = "strlen";
#endif

Routine::Routine(LoadModule *_img, addrtype _start, usize_t _size, 
           const std::string& _name, addrtype _offset)
     : PrivateRoutine(_img, _start, _size, _name, _offset)
{
   memset(counters, 0, NUM_BBL_TYPES*sizeof(unsigned long));
   g = new CFG(this, _name);
}

Routine::~Routine()
{
   if (g)
      delete g;
}

void
Routine::printProfile(FILE *fout, int genCFGTest) const
{
   if (fout)
   {
      int i;
      fprintf(fout, "Number of add BBL events:\n");
      unsigned long total = 0;
      for (i=0 ; i<NUM_BBL_TYPES ; ++i)
         if (counters[i]>0)
         {
            total += counters[i];
            fprintf(fout, "%lu - {%d: ", counters[i], i);
            if (i==CREATE_NEW_BBL)
               fprintf(fout, "construct new BBL}\n");
            else if (i==IDENTICAL_BBL)
               fprintf(fout, "identical BBL}\n");
            else
            {
               int var = i & BBL_ENTER_MASK;
               if (var==(ENTERS_OLD_BBL|EXTENDS_BEFORE_OLD_BBL))
                  fprintf(fout, "identical start");
               else if (var==ENTERS_OLD_BBL)
                  fprintf(fout, "starts inside old BBL");
               else if (var==EXTENDS_BEFORE_OLD_BBL)
                  fprintf(fout, "starts before old BBL");
               else
                  fprintf(fout, "WRONG start");
            
               var = i & BBL_EXIT_MASK;
               if (var==(EXITS_OLD_BBL|EXPANDS_OVER_OLD_BBL))
                  fprintf(fout, ", identical end");
               else if (var==EXITS_OLD_BBL)
                  fprintf(fout, ", ends inside old BBL");
               else if (var==EXPANDS_OVER_OLD_BBL)
                  fprintf(fout, ", ends after old BBL");
               else
                  fprintf(fout, ", WRONG end");
               if (i&COVER_MULTIPLE_BBLS)
                  fprintf(fout, ", covers multiple blocks");
               fprintf(fout, "}\n");
            }
         }
      fprintf(fout, "Total: %lu BBL events\n", total);
   }
   
   // draw the CFG graph
   if (!genCFGTest)
   {
      CfgToDot();
   }
}

#if 0
int 
Routine::AddCallSurrogateBlock(addrtype call_addr, addrtype _end, addrtype target,
               PrivateCFG::EdgeType etype, bool control_returns, PrivateCFG::Edge** pe)
{
   if (PrivateRoutine::AddCallSurrogateBlock(call_addr, _end, target, etype, control_returns, pe) &&
             control_returns)
   {
      // TODO: I also have to add a counter for the fall-through edge.
      // Call instructions do not have fall-through instrumentation points in PIN, 
      // so I have to implement this logic myself. Perhaps adding a counter to the 
      // block following the call instructions. It could be something similar to
      // routine entries where I keep track of addresses following function calls
      // and I collect counts for the traces starting at those addresses. Brrr.
      BBMap::iterator bit = _blocks.upper_bound(_end);
      uint64_t iniVal = 0;
      if (bit!=_blocks.end() && bit->second->getStartAddress()==_end)
      {
         // if we already have the fall-thru block, compute the counts from
         // all incoming edges into this block, before we take the fall-thru edge
         // for the first time. I will use this value to initialize the counter for 
         // the call-site return.
         // This works only if a new trace is generated next time we go into the
         // fall-thru block. However, since it was already JIT-ed before, I am not
         // sure if PIN will instrument it again. What to do?
         // I can invalidate the code cache ...
         CFG::Node *nn = static_cast<CFG::Node*>(bit->second);
         if (nn == NULL)
         {
            fprintf(stderr, "In Routine::AddCallSurrogateBlock, static cast of PrivateCFG::Node returns NULL for routine %s, "
                    "block at [0x%" PRIxaddr ",0x%" PRIxaddr "]\n",
                    name.c_str(), bit->second->getStartAddress(), bit->second->getEndAddress());
            assert (!"NULL CFG::Node static cast");
         }
         CFG::IncomingEdgesIterator ieit(nn);
         while ((bool)ieit)
         {
            CFG::Edge *ee = ieit;
            if (ee==NULL)
            {
               fprintf(stderr, "In Routine::AddCallSurrogateBlock, incoming iterator returns NULL for routine %s, "
                       "block at [0x%" PRIxaddr ",0x%" PRIxaddr "]\n",
                       name.c_str(), nn->getStartAddress(), nn->getEndAddress());
               assert (!"NULL CFG::Edge (due to static cast?)");
            }
            if (ee->isCounterEdge())
               iniVal += *(ieit->Counter());
            ++ ieit;
         }
      } 
      
      // and I am marking the fall-through address as a call-site return
      if (returns.find(_end)==returns.end())
      {
         // gmarin, 2014/01/30: This value have to be computed and stored per thread
         returns[_end] = iniVal;
#if DEBUG_CFG_DISCOVERY
         LOG_CFG(3,
            fprintf(stderr, "Routine %s, add call-site return at %p\n", name.c_str(), (void*)_end);
         )
#endif
      }
      return (1);
   } else
      return (0);
}
#endif
    
int
Routine::AddBlock(addrtype _start, addrtype _end, PrivateCFG::NodeType nodeType, int* inEdges)
{
   int incEdges = -1;
   int retval = PrivateRoutine::AddBlock(_start, _end, nodeType, &incEdges);
   
   // if incEdges == 0, then the _start address represents a routine entry;
   // mark it so.
   // disable the entries for now. I will just use trace starts, FIXME
   if (0 && incEdges==0 && entries.find(_start)==entries.end())
   {
      entries[_start] = 0;
#if DEBUG_CFG_DISCOVERY
      LOG_CFG(1,
         fprintf(stderr, "Routine %s, add entry at %p\n", name.c_str(), (void*)_start);
      )
#endif
   }
   
   // count how many BBLs of each type we encounter
   assert (retval>=0 && retval<NUM_BBL_TYPES);
   counters[retval] += 1;
#if DEBUG_BBLs
   if (name.compare(debugName)==0)
   {
      fprintf(stderr, "Routine %s, add block [%p,%p] => %d\n", name.c_str(),
          (void*)_start, (void*)_end, retval);
   }
#endif
   return (retval);
}

int
Routine::AddBlockNoEntry(addrtype _start, addrtype _end, PrivateCFG::NodeType nodeType, int* inEdges)
{
   int incEdges = -1;
   int retval = PrivateRoutine::AddBlock(_start, _end, nodeType, &incEdges);
   
   // count how many BBLs of each type we encounter
   assert (retval>=0 && retval<NUM_BBL_TYPES);
   counters[retval] += 1;
#if DEBUG_BBLs
   if (name.compare(debugName)==0)
   {
      fprintf(stderr, "Routine %s, add block no entry [%p,%p] => %d\n", name.c_str(),
          (void*)_start, (void*)_end, retval);
   }
#endif
   return (retval);
}


void
Routine::saveToFile(FILE *fd, addrtype base_addr, LoadModule *myimg, 
                 const MIAMI_CFG::count_type *thr_counters, 
                 MIAMI_CFG::MiamiCfgStats &stats) const
{
   // save start/end addresses and name in prefix format (len followed by name)
   fwrite(&start_offset, sizeof(addrtype), 1, fd);
   addrtype temp_start = start_addr - base_addr;
   fwrite(&temp_start, sizeof(addrtype), 1, fd);
   addrtype end_addr = temp_start + size;
   fwrite(&end_addr, sizeof(addrtype), 1, fd);
   // now save the routine name
   uint32_t len = name.length();
   fwrite(&len, 4, 1, fd);
   fwrite(name.c_str(), 1, len, fd);
   
   // save the entries/trace starts/CFG only if we have any non-zero counts
   bool is_zero = true;
   EntryMap::const_iterator eit = entries.begin();
   for ( ; eit!=entries.end() && is_zero ; ++eit)
   {
      uint64_t count = 0;
      int32_t index = myimg->HasRoutineEntryIndex(eit->first);
      if (index>0 && index<(int32_t)(MIAMI_CFG::thr_buf_size))
         count = thr_counters[index];
      if (count > 0)
         is_zero = false;
   }
   eit = traces.begin();
   for ( ; eit!=traces.end() && is_zero ; ++eit)
   {
      uint64_t count = 0;
      int32_t index = myimg->HasTraceStartIndex(eit->first);
      if (index>0 && index<(int32_t)(MIAMI_CFG::thr_buf_size))
         count = thr_counters[index];
      if (count > 0)
         is_zero = false;
   }
   if (is_zero)
   {
      // all entries and trace starts are 0; 
      // I only have to write 3 zeros in the CFG file
      uint32_t zero = 0;
      fwrite(&zero, 4, 1, fd);
      fwrite(&zero, 4, 1, fd);
      fwrite(&zero, 4, 1, fd);
      return;
   }
   
   // next, save the number of entries and the entries with their associated counts
   len = entries.size();
   fwrite(&len, 4, 1, fd);
   eit = entries.begin();
   for ( ; eit!=entries.end() ; ++eit)
   {
      addrtype temp_addr = eit->first - base_addr;
      uint64_t count = 0;
      int32_t index = myimg->HasRoutineEntryIndex(eit->first);
      if (index>0 && index<(int32_t)(MIAMI_CFG::thr_buf_size))
         count = thr_counters[index];
      if (count>0)
      {
         stats.routine_entries += 1;
         stats.tot_increments += count;
      }
      fwrite(&temp_addr, sizeof(addrtype), 1, fd);
      fwrite(&count, sizeof(uint64_t), 1, fd);
   }
   
   // save also the number of call-site returns and the return points with their associated counts
   // we do not keep track of return sites anymore. Save trace starts instead.
   len = traces.size();
   fwrite(&len, 4, 1, fd);
   eit = traces.begin();
   for ( ; eit!=traces.end() ; ++eit)
   {
      addrtype temp_addr = eit->first - base_addr;
      uint64_t count = 0;
      int32_t index = myimg->HasTraceStartIndex(eit->first);
      if (index>0 && index<(int32_t)(MIAMI_CFG::thr_buf_size))
         count = thr_counters[index];
      if (count>0)
      {
         stats.trace_counters += 1;
         stats.tot_increments += count;
      }
      fwrite(&temp_addr, sizeof(addrtype), 1, fd);
      fwrite(&count, sizeof(uint64_t), 1, fd);
   }
   
   // finally, save the cfg
   if (g)
   {
      CFG *cfg = static_cast<CFG*>(g);
      cfg->saveToFile(fd, base_addr, myimg, thr_counters, stats);
   }
   else
   {
      uint32_t zero = 0;
      fwrite(&zero, 4, 1, fd);
   }
}

void 
Routine::SetEntryAndTraceCounts(LoadModule *myimg, const MIAMI_CFG::count_type *thr_counters)
{
   EntryMap::iterator eit = entries.begin();
   for ( ; eit!=entries.end() ; ++eit)
   {
      uint64_t count = 0;
      int32_t index = myimg->HasRoutineEntryIndex(eit->first);
      if (index>0 && index<(int32_t)(MIAMI_CFG::thr_buf_size))
         count = thr_counters[index];
      eit->second = count;
   }
   
   eit = traces.begin();
   for ( ; eit!=traces.end() ; ++eit)
   {
      uint64_t count = 0;
      int32_t index = myimg->HasTraceStartIndex(eit->first);
      if (index>0 && index<(int32_t)(MIAMI_CFG::thr_buf_size))
         count = thr_counters[index];
      eit->second = count;
   }

   if (g)
   {
      CFG *cfg = static_cast<CFG*>(g);
      cfg->SetEdgeCounts(myimg, thr_counters);
   }
}

bool 
Routine::RecordTraceRange(addrtype _start, addrtype _end)
{
   bool new_start = true;
   TraceMap::iterator tits = traceRanges.upper_bound(_start), tite;

   if (tits == traceRanges.end() || _end<=tits->second.first)  // it's a completely new trace
   {
      traceRanges.insert(TraceMap::value_type(_end, AddrPair(_start, _end)));
   } else
   {
      // we dealt with the non-overlpped case until now.
      // Here, we either have parts of the trace that are new, or
      // the entire trace is included into old traces.
      addrtype bStart = tits->second.first;
      if (_start<bStart)
      {
         // _start is not part of a previous trace region.
         // we know that old trace's start address is < current _end based on a previous test, 
         // but its start address is > than current _start addr.
         // Split current trace in two. 
         traceRanges.insert(TraceMap::value_type(bStart, AddrPair(_start, bStart)));
      } else   // _start >= bstart
      {
         // The trace start address is not new.
         new_start = false;
         // I still have to check if other parts of the trace are new
      }
         
      // Check if we need to split the old trace
      {
         tite = tits;
         addrtype bStart = tite->second.first,
                  bEnd   = tite->second.second;
         addrtype prevEnd = bStart;
         
         while (tite!=traceRanges.end() && bEnd<_end)
         {
            prevEnd = bEnd;
            ++tite;
            if (tite != traceRanges.end()) 
            {
               bStart = tite->second.first;
               bEnd   = tite->second.second;
                  
               if (prevEnd < bStart)  // there is a gap between traces
               {
                  addrtype endval = bStart;
                  if (_end <= bStart) 
                  {
                     assert(prevEnd < _end);
                     endval = _end;
                  }
                  std::pair<TraceMap::iterator, bool> res = 
                          traceRanges.insert(TraceMap::value_type(endval, AddrPair(prevEnd, endval)));
                  
                  if (_end<=bStart)
                  {
                     tite = res.first;
                     bEnd = endval;
                     break;
                  }
               }
               else if (prevEnd>bStart)
                  assert ("Existing trace ranges overlap. It should not happen, right?");
            }
         }

         // now, we either reached the end of the map or we found a trace which extends
         // past the current _end (could be equal - it better be)
         if (tite == traceRanges.end())  // new block extends farther out
         {
            traceRanges.insert(TraceMap::value_type(_end, AddrPair(prevEnd, _end)));
         }
      }  // check for partial overlaps scope
   }   // not completely NEW trace
   
   return (new_start);
}

void 
Routine::MarkCountersPlacement()
{
   if (! g)   // cannot do anything if we don't have a CFG built
      return;
   // traverse all nodes and place counter on nodes following a call-surrogate block
   // and on nodes that have no incoming edges
   PrivateCFG::NodesIterator nit(*g);
   while ((bool)nit)
   {
      PrivateCFG::Node *nn = nit;
      PrivateCFG::Edge *fall_e = nn->firstIncomingOfType(PrivateCFG::MIAMI_FALLTHRU_EDGE);
      if (fall_e)  // we found one, now we know for sure what we have
      {
         PrivateCFG::Node *pred_b = fall_e->source();
         if (pred_b->isCallSurrogate())
            addTraceCounter(nn->getStartAddress());
      } else  // we do not have a fall-thru edge yet 
      {
         // check if it has any incoming edges
         bool no_edges = true;
         PrivateCFG::IncomingEdgesIterator ieit(nn);
         while ((bool)ieit && no_edges)
         {
            PrivateCFG::Edge *ee = ieit;
            if (ee->getType() != PrivateCFG::MIAMI_CFG_EDGE)
               no_edges = false;
            ++ ieit;
         }
         if (no_edges)
            addTraceCounter(nn->getStartAddress());
      }

      ++ nit;
   }
   
   // Traverse all edges, and place counters on direct, indirect and bypass edges.
   // Direct and indirect edges are marked when they are added to the graph.
   // Thus, only bypass edges must be marked here.
   PrivateCFG::EdgesIterator eit(*g);
   while ((bool)eit)
   {
      PrivateCFG::Edge *ee = eit;
      if (ee->getType() == PrivateCFG::MIAMI_BYPASS_EDGE)
         ee->setCounterEdge();
      ++ eit;
   }
}

