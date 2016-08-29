/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: mark_back_edges.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Identifies loop back edges in a CFG based on Tarjan information.
 * Identifies also loop entry and loop exit edges.
 */

#include "uipair.h"
#include <stdlib.h>
#include <unistd.h>
#include <fnmatch.h>
#include <assert.h>

#include "tarjans/MiamiRIFG.h"
#include "tarjans/TarjanIntervals.h"
#include "miami_types.h"
#include "PrivateCFG.h"

#define DEBUG_LOOP_EDGE 0

using namespace MIAMI;
#if DEBUG_LOOP_EDGE
using namespace std;
#endif

// mark also loop exit edges with the outermost loop they are exiting from.
// this info is needed sometimes and it is hard to get the id of the outermost
// exited loop from eel when an edge exits multiple levels of a loop nest.
void
PrivateCFG::mark_loop_back_edges (RIFGNodeId root, TarjanIntervals *tarj, MiamiRIFG *ecfg,
           AddrEdgeMMap *entryEdges, AddrEdgeMMap *callEntryEdges)
{
   RIFGNodeId kid;
#if DEBUG_LOOP_EDGE
   if (root == 0)
   {
      fflush (stdout);
      fflush (stderr);
      tarj->Dump(cerr);
      fflush (stdout);
   }
   cerr << "mark_loop_back_edges: processing interval of type " 
        << tarj->getNodeType (root) << " for node " << root << endl;
#endif

   Node *root_b = static_cast<Node*>(ecfg->GetRIFGNode (root));
   for (kid = tarj->TarjInners(root) ; kid != RIFG_NIL ; 
                kid = tarj->TarjNext(kid))
   {
      RIFGEdgeId inEdge;
      Node *b = static_cast<Node*>(ecfg->GetRIFGNode (kid));
      // if callEntryEdges is not NULL and this bb is call surrogate,
      // store the edge outgoing from bb as call entry edge.
      if (callEntryEdges && b->isCallSurrogate())
      {
         Edge *outE = NULL;
         int succSize = b->num_outgoing();
         if (succSize)  // if it has successors, it can have only one
         {
            outE = b->firstOutgoingOfType(PrivateCFG::MIAMI_FALLTHRU_EDGE);
//            assert (outE != 0);
         }
         if (outE)
         {
            addrtype loopStart = root_b->getStartAddress();
//            fprintf (stderr, "Record call entry edge for loop %08x, entry block %08x\n",
//                   loopStart, outE->tail()->start());
            callEntryEdges->insert (
                AddrEdgeMMap::value_type (loopStart, outE));
         }
      }
      
#if DEBUG_LOOP_EDGE
      cerr << "Processing incoming edges for node " << kid << " b " << b->getId() << " [" 
           << hex << b->getStartAddress() << "," << b->getEndAddress() << "]" << dec << endl;
#endif
      RIFGEdgeIterator* ei = ecfg->GetEdgeIterator(kid, ED_INCOMING);
      for (inEdge = ei->Current(); inEdge != RIFG_NIL; inEdge = (*ei)++)
      {
         Edge* e = static_cast<Edge*>(ecfg->GetRIFGEdge(inEdge));
         if (tarj->IsBackEdge(inEdge))
         {
#if DEBUG_LOOP_EDGE
            cerr << "mark_loop_back_edges: setting edge from b " << e->source()->getId() 
                 << " [" << hex << e->source()->getStartAddress() << "," 
                 << e->source()->getEndAddress() << dec << "] to b " 
                 << e->sink()->getId() << " [" << hex << e->sink()->getStartAddress()
                 << "," << e->sink()->getEndAddress() << "]" << dec << endl;
#endif
            e->setLoopBackEdge();
         }
         RIFGNodeId src = ecfg->GetEdgeSrc (inEdge);
         int lEnter = 0, lExit = 0;   // store number of levels crossed
         tarj->tarj_entries_exits (src, kid, lEnter, lExit);
#if DEBUG_LOOP_EDGE
         if (lEnter || lExit)
         {
            cerr << "Edge E " << e->getId() << " from  b " << e->source()->getId() 
                 << " to b " << e->sink()->getId() << " enters "
                 << lEnter << " loops and exits " << lExit << " loops." << endl;
         }
#endif
         if (lExit > 0) // it is a loop exit edge
         {
            // get the outermost loop exited using the tarjans
            RIFGNodeId lhead = tarj->tarj_loop_exited (src, kid);
            assert (lhead != RIFG_NIL);
            assert (tarj->numExitEdges (lhead) > 0);
            // do I get exit edges from irreducible intervals? 
            // assert for now, and if we do, then uncomment the loop below
            // I am interested only if I exit some natural loop.
            // Yes, irreducible intervals are "loops" with multiple entry 
            // nodes. They have exit edges as well.
            //assert (tarj->IntervalType(lhead) == RI_TARJ_INTERVAL);

            Node* btemp = static_cast<Node*>(ecfg->GetRIFGNode (lhead));
            e->setOuterLoopExited (btemp);
            e->setNumLevelsExited (lExit);
//            if (lExit>1)
//               r->set_has_edge_crossing_multiple_levels ();
         }
         if (lEnter > 0)  // it is a loop entry edge
         {
            // get the outermost loop entered using the tarjans
            RIFGNodeId lhead = tarj->tarj_loop_entered (src, kid);
            assert (lhead != RIFG_NIL);
            // do I get entry edges for irreducible intervals? I think not.
            // assert for now, and if we do, then solve it later.
            // I am interested only if I enter some natural loop.
            // Yes we get entry edges into irreducible intervals, duh
            //assert (tarj->IntervalType(lhead) == RI_TARJ_INTERVAL);

            Node* btemp = static_cast<Node*>(ecfg->GetRIFGNode (lhead));
            e->setOuterLoopEntered (btemp);
            e->setNumLevelsEntered (lEnter);
//            if (lEnter>1)
//               r->set_has_edge_crossing_multiple_levels ();
            // also mark the block as a entry block?
            b->setLoopEntryBlock ();

            // if entryEdges is not NULL, record this edge as an entry 
            // edge for our current loop.
            // gmarin, 05/10/2012: If the edge enters multiple loop levels,
            // add the edge as entry for all the levels
            if (entryEdges)
            {
               RIFGNodeId ikid = RIFG_NIL;
               if (tarj->NodeIsLoopHeader(kid))
                  ikid = kid;
               else
                  ikid = tarj->TarjOuter(kid);
               
               for(int k=1 ; k<=lEnter ; ++k)
               {
                  assert (ikid != RIFG_NIL);
                  Node* btemp = static_cast<Node*>(ecfg->GetRIFGNode (ikid));
                  addrtype loopStart = btemp->getStartAddress();
                  
#if DEBUG_LOOP_EDGE
                  fprintf (stderr, "Record entry edge for loop %08lx, entry block %08lx, levels entered %d/%d\n",
                        loopStart, b->getStartAddress(), k, lEnter);
#endif
                  entryEdges->insert (AddrEdgeMMap::value_type (loopStart, e));
                  if (k<lEnter)
                     ikid = tarj->TarjOuter(ikid);
               }
               // at the end I should get the outermost scope entered by the edge
               // which we found before
               assert (ikid == lhead);
            }
         }
      }
      delete ei;

      if (tarj->NodeIsLoopHeader(kid))
      {
         mark_loop_back_edges (kid, tarj, ecfg, entryEdges, callEntryEdges);
      }
   }
}

