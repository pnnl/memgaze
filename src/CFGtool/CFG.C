/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: CFG.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements the Control Flow Graph (CFG) for a routine. It extends the 
 * common PrivateCFG class implementation with functionality specifically 
 * needed for a CFG profiler.
 */

#include "miami_types.h"
#include <math.h>

// standard headers

#ifdef NO_STD_CHEADERS
# include <stdlib.h>
# include <string.h>
# include <assert.h>
# include <stdarg.h>
#else
# include <cstdlib>
# include <cstring>
# include <cassert>
using namespace std; // For compatibility with non-std C headers
#endif

#include <iostream>
#include <fstream>
#include <iomanip>

#include "fast_hashmap.h"

using std::ostream;
using std::endl;
using std::cout;
using std::cerr;
using std::setw;

#include "CFG.h"
#include "routine.h"
#include "debug_miami.h"

using namespace MIAMI;

//--------------------------------------------------------------------------------------------------------------------

CFG::CFG (Routine* _r, const std::string& func_name)
      : PrivateCFG(_r, func_name)
{
  // create a node that will act as the cfg entry node
  cfg_entry = MakeNode(0, 0, MIAMI_CFG_BLOCK);
  cfg_entry->setNodeFlags(NODE_IS_CFG_ENTRY);
  PrivateCFG::add(cfg_entry);
  // and a node that will act as the cfg exit node
  cfg_exit = MakeNode(0, 0, MIAMI_CFG_BLOCK);
  cfg_exit->setNodeFlags(NODE_IS_CFG_EXIT);
  PrivateCFG::add(cfg_exit);
}

//--------------------------------------------------------------------------------------------------------------------
CFG::~CFG()
{
}

//--------------------------------------------------------------------------------------------------------------------
void 
CFG::add (DGraph::Edge* de)
{
   setCfgFlags(CFG_GRAPH_IS_MODIFIED);
   DGraph::add(de);
   Edge *e = static_cast<Edge*>(de);
   if (e->getType()==MIAMI_DIRECT_BRANCH_EDGE || e->getType()==MIAMI_INDIRECT_BRANCH_EDGE)
      e->setCounterEdge();
}
//--------------------------------------------------------------------------------------------------------------------

void 
CFG::saveToFile(FILE *fd, addrtype base_addr, LoadModule *myimg, 
                  const MIAMI_CFG::count_type *thr_counters, 
                  MIAMI_CFG::MiamiCfgStats &stats)
{
   // traverse prospectiveEdges first and check for any indirect branches that
   // have counts. If they are still in prospectiveEdges, it means they were
   // interprocedural jumps, so add a call surrogate node for each such edge
   // and then a return edge from the call surrogate to the cfg exit node.
   // Save the target address in the call surrogate node in case I use it later.
   // I can also have direct jumps to another routine, so do not limit the test
   // only to indirect jumps
   // gmarin, 04/13/2012: News flash: I can also have a fall-through edge which was
   // even taken, but not recorded. This happens in optimized libraries where you fall
   // through from one routine to another. The fall-through edge is added without a 
   // counter. We need to preserve any FALL_THROUGH edges that go to the cfg_exit node.
   // gmarin, 05/15/2012: I add explicit return edges from each call surrogate block.
   // Do not keep prospective fall-through edges that originate at call surrogate blocks. 
   // gmarin, 2013/10/16: Due to bad routine boundaries from PIN, sometimes I add
   // a fall-thru edge to the cfg exit node because that is the reported routine end, 
   // but then I get a trace that crosses that boundary, so I create a new fall-thru
   // edge. Do not keep fall-thru edges to cfg-exit if the source node has another
   // fall-thru edge.
   EdgeMap::iterator eit = prospectiveEdges.begin();
   for ( ; eit!=prospectiveEdges.end() ; )
   {
      EdgeMap::iterator tmp =  eit;
      Edge *e = static_cast<Edge*>(eit->second);
      if ( (e->type==MIAMI_INDIRECT_BRANCH_EDGE || e->type==MIAMI_DIRECT_BRANCH_EDGE) && 
           (eit->first<in_routine->Start() || eit->first>=in_routine->End()) )
      {
         // it is an indirect jump outside of current routine; create a call surogate node 
         // as sink
         addrtype eaddr = e->source()->getEndAddress();
         Node *n = MakeNode(eaddr, eaddr, MIAMI_CALL_SITE);
         setCfgFlags(CFG_GRAPH_IS_MODIFIED);
         DGraph::add(n);
         
         n->setTarget(eit->first);
         e->sink()->incoming_edges.remove(e);
         n->incoming_edges.push_back(e);
         e->ChangeSink(n);
         e->setProspectiveEdge(false);
      } else
      if (e->type==MIAMI_FALLTHRU_EDGE && e->source()->isCallSurrogate())
      {
         // I should not remove prospective edges when dealing with multi-threaded 
         // programs. Other threads are still running and may still use these
         // prospective edges.
         assert (e->isProspectiveEdge());
         // remove(e); 
         // delete (e);
      }
      ++eit;
      if (! e->isProspectiveEdge())
         prospectiveEdges.erase(tmp);
   }

   // write number of nodes first (not including cfg_entry and cfg_exit)
   uint32_t nNodes = node_set.size() - 2;
   fwrite(&nNodes, 4, 1, fd);
   if (nNodes == 0) return;
   fwrite(&cfgFlags, 4, 1, fd);
   
   NodesIterator nit(*this);
   uint32_t actualNodes = 0;
   while ((bool)nit)
   {
      Node *nn = nit;
      if (nn->isCodeNode())
      {
         fwrite(&nn->id, 4, 1, fd);
         char type = (char)nn->type;
         fwrite(&type, 1, 1, fd);
         fwrite(&nn->flags, 4, 1, fd);
         addrtype temp_addr = nn->start_addr - base_addr;
         fwrite(&temp_addr, sizeof(addrtype), 1, fd);
         if (type == MIAMI_CALL_SITE)
            temp_addr = nn->target - base_addr;
         else
            temp_addr = nn->end_addr - base_addr;
         fwrite(&temp_addr, sizeof(addrtype), 1, fd);
         ++ actualNodes;
      }
      ++ nit;
   }
   if (nNodes != actualNodes)
   {
      fprintf(stderr, "Routine %s, wrote out %u nodes instead of the expected %u. Check this out.\n",
           routine_name.c_str(), actualNodes, nNodes);
      assert(nNodes == actualNodes);
   }
   
   EdgesIterator egit(*this);
   while ((bool)egit)
   {
      Edge *ee = egit;
      // some edges may be NULL after dynamic_cast from iterator because they are
      // CFG edges of type PrivateCFG::Edge (only happens if Top Nodes are computed,
      // like when a CFG is drawn)
      if (ee && ee->type!=MIAMI_CFG_EDGE && 
              (!ee->isProspectiveEdge() || 
                    (!ee->source()->isCallSurrogate() && ee->type==MIAMI_FALLTHRU_EDGE)) &&
              (ee->sink()!=cfg_exit || ee->type==MIAMI_RETURN_EDGE || ee->type==MIAMI_FALLTHRU_EDGE)
         )
      {
         if (ee->sink()==cfg_exit && ee->type==MIAMI_FALLTHRU_EDGE)  
         {
            // check if we have other fall-thru edges from the same source node
            bool has_fall_thru = false;
            OutgoingEdgesIterator ogit(ee->source());
            while ((bool)ogit)
            {
               Edge *oe = ogit;
               if (oe->type==MIAMI_FALLTHRU_EDGE && oe->sink()!=cfg_exit) // different fall-thru
               {
                  has_fall_thru = true;
                  break;
               }
               ++ ogit;
            }
            if (has_fall_thru)
            {
               ++ egit;
               continue;
            }
         }
         // no need to save the id, we will not use it later
         //fwrite(&ee->id, 4, 1, fd);
         char type = (char)ee->type;
         fwrite(&type, 1, 1, fd);
         fwrite(&ee->flags, 4, 1, fd);
         fwrite(&ee->source()->id, 4, 1, fd);
         if (ee->sink() != cfg_exit)
            fwrite(&ee->sink()->id, 4, 1, fd);
         else
         {
            assert (ee->type==MIAMI_RETURN_EDGE || ee->type==MIAMI_FALLTHRU_EDGE);
            uint32_t id = -1;
            fwrite(&id, 4, 1, fd);
         }

         if (ee->isCounterEdge())
         {
            uint64_t count = 0;
            int32_t index = myimg->HasCFGEdgeIndex(ee);
            if (index>0 && index<(int32_t)(MIAMI_CFG::thr_buf_size))
               count = thr_counters[index];
            if (count>0)
            {
               stats.edge_counters += 1;
               stats.tot_increments += count;
            }
            fwrite(&count, sizeof(uint64_t), 1, fd);
         }
      }
      ++ egit;
   }
   // at the end I need to write a special value to mark the end of the edge list
   // I will just write a single byte set at 0xFF as this is an unused edge type
   char marker = -1;
   fwrite(&marker, 1, 1, fd);
}
//--------------------------------------------------------------------------------------------------------------------

void 
CFG::SetEdgeCounts(LoadModule *myimg, const MIAMI_CFG::count_type *thr_counters)
{
   EdgesIterator egit(*this);
   while ((bool)egit)
   {
      Edge *ee = egit;
      if (ee->isCounterEdge())
      {
         uint64_t value = 0;
         int32_t index = myimg->HasCFGEdgeIndex(ee);
         if (index>0 && index<(int32_t)(MIAMI_CFG::thr_buf_size))
            value = thr_counters[index];
         *(ee->Counter()) = value;
      }
      ++ egit;
   }
}

