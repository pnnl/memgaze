/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: CFG.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements the Control Flow Graph (CFG) logic for a routine. 
 * It extends the base PrivateCFG class implementation with analysis 
 * needed by the MIAMI modeling engine.
 */

#include <math.h>

// standard headers

# include <stdlib.h>
# include <string.h>
# include <assert.h>
# include <stdarg.h>

#include <iostream>
#include <fstream>
#include <iomanip>


using std::ostream;
using std::endl;
using std::cout;
using std::cerr;
using std::setw;

#include "miami_types.h"
#include "CFG.h"
#include "routine.h"
#include "debug_scheduler.h"
#include "Assertion.h"

#define HASHMAP_ENABLE_ERASE
#include "fast_hashmap.h"

using namespace MIAMI;

CFG::Node* defNodeP = NULL;
typedef MIAMIU::HashMap <int32_t, CFG::Node*, &defNodeP> IdNodeMap;

//--------------------------------------------------------------------------------------------------------------------

CFG::CFG (Routine* _r, const std::string& func_name)
      : PrivateCFG(_r, func_name)
{
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

#include "CFGDominator.h"
typedef CFGDominator <CFG::SuccessorNodesIterator, 
                   CFG::PredecessorNodesIterator> CFGPreDominator;
typedef CFGDominator <CFG::PredecessorNodesIterator,
                   CFG::SuccessorNodesIterator> CFGPostDominator;

//--------------------------------------------------------------------------------------------------------------------

void
CFG::computeNodeDegrees()
{
   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      nn->fanin = nn->fanout = 0;
      ++ nit;
   }
   
   EdgesIterator eit(*this);
   while ((bool)eit)
   {
      eit->source()->fanout += 1;
      eit->sink()->fanin += 1;
      ++ eit;
   }
}
//--------------------------------------------------------------------------------------------------------------------

void
CFG::draw_dominator_tree (BaseDominator *bdom, const char* prefix)
{
   char file_name[64];
   char file_name_ps[64];
   char title_draw[64];

   sprintf(file_name, "%s_%.40s.dot", prefix, routine_name.c_str());
   sprintf(file_name_ps, "%s_%.40s.ps", prefix, routine_name.c_str());
   sprintf(title_draw, "%s_%.40s", prefix, routine_name.c_str());
   
   std::ofstream fout(file_name, std::ios::out);
   assert(fout && fout.good());

   fout << "digraph \"" << title_draw << "\"{\n";
   fout << "size=\"7.5,10\";\n";
   fout << "center=\"true\";\n";
//   fout << "ratio=\"compress\";\n";
   fout << "ranksep=.3;\n";
   fout << "node[color=black,fontcolor=black,shape=ellipse,font=Helvetica,fontsize=14,height=.3];\n";
   fout << "edge[font=Helvetica,fontsize=14,dir=forward,style=\"setlinewidth(2),solid\",color=black,fontcolor=black];\n";
   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      if (nn->isCodeNode () )
      {
         nn->draw(fout);
         Node *idom = dynamic_cast<Node*>(bdom->iDominator (nn));
         if (idom)
            fout << "B" << idom->getId() << "->B" << nn->getId() << ";\n";
      }
      ++ nit;
   }
   fout << "}\n";

   fout.close();
   assert(fout.good());
}
//--------------------------------------------------------------------------------------------------------------------

#define DEBUG_PARSE_CFG 0

int 
CFG::loadFromFile(FILE *fd, MIAMI::EntryMap& entries, MIAMI::EntryMap& traces, addrtype _reloc)
{
#define CHECK_COND(cond, err, ...) if (cond) \
   {fprintf(stderr, "ERROR: " err "\n", __VA_ARGS__); goto load_error; }
#define CHECK_COND0(cond, err) if (cond) \
   {fprintf(stderr, "ERROR: " err "\n"); goto load_error; }
  
   size_t res;
   uint32_t nNodes;
   IdNodeMap nodes;
   uint32_t i = 0;
   PrivateCFG::NodeList::iterator lit;
   
   // read number of nodes first
   res = fread(&nNodes, 4, 1, fd);
   CHECK_COND0(res!=1, "reading number of nodes")
   if (nNodes == 0) return 0;
   
   res = fread(&cfgFlags, 4, 1, fd);
   CHECK_COND0(res!=1, "reading CFG flags")
   
   if (!entries.empty())
      setCfgFlags(CFG_HAS_ENTRY_POINTS);
   
   for (i=0 ; i<nNodes ; ++i)
   {
      addrtype _start, _end;
      char _type;
      uint32_t _id, _flags;
      res = fread(&_id, 4, 1, fd);
      CHECK_COND(res!=1, "reading ID of node %d", i)
      
      res = fread(&_type, 1, 1, fd);
      CHECK_COND(res!=1, "reading type of node %d", i)
      res = fread(&_flags, 4, 1, fd);
      CHECK_COND(res!=1, "reading flags of node %d", i)
      res = fread(&_start, sizeof(addrtype), 1, fd);
      CHECK_COND(res!=1, "reading start address of node %d", i)
      res = fread(&_end, sizeof(addrtype), 1, fd);
      CHECK_COND(res!=1, "reading end address of node %d", i)
      
      Node *nn = 0;
      if (_type == MIAMI_CALL_SITE)
      {
         nn = new Node(this, _start, _start, (NodeType)_type);
         nn->setTarget(_end);
         nn->flags = _flags;
      }
      else
      {
         nn = new Node(this, _start, _end, (NodeType)_type);
         nn->flags = _flags;
      }
      add(nn);
      
      if (_type != MIAMI_CALL_SITE)
      {
         MIAMI::EntryMap::iterator eit = entries.find(_start);
         if (eit != entries.end())  // it's an entry node
         {
            Edge* edge = new Edge(static_cast<Node*>(cfg_entry), nn, MIAMI_CFG_EDGE);
            add(edge);
            // add count
            nn->counter = eit->second;
            nn->flags |= NODE_PARTIAL_COUNT;
            nn->flags |= ROUTINE_ENTRY_NODE;
            topNodes.push_back(nn);
         }
         eit = traces.find(_start);
         if (eit != traces.end())  // it's a call return node
         {
            // add count; Why add it and not just copy it???
            // we add the counter value because, during profiling, 
            // for a given address I insert either a routine entry 
            // counter or a trace start one. Never both. 
            // The "entry" role is never lost. However, a node
            // can become trace start first time it is discovered, 
            // and later it can lose that role.
            // Hmm, if it loses that role, then I do not even save its
            // trace start count in the results file. 
            // I need to do this check before I save the cfg files.
//            nn->counter += eit->second;
            nn->counter = eit->second;
            // remove the partial flag if the node has it set
            // hmm, found issues. Better keep the Partial flag if
            // the node is also a potential routine entry.
//            nn->flags &= ~(NODE_PARTIAL_COUNT);
            // and set it as counter node
            nn->flags |= CFG_COUNTER_NODE;
         }
      }
      
      // I must also keep track of which old id corresponds to which node, 
      // so that I can create the edges correctly
      nodes[_id] = nn;
#if DEBUG_PARSE_CFG
      fprintf (stderr, "Parsed node (%u) [%p,%p] of type %u\n", _id,
          (void*)_start, (void*)_end, (unsigned int)_type);
#endif
   }
   
   i = 0;
   do
   {
      char type;
      uint32_t srcId, sinkId, _flags;
      res = fread(&type, 1, 1, fd);
      CHECK_COND(res!=1, "reading edge %d type", i)
      
      if (type == -1)  // special marker, exit loop
         break;

      res = fread(&_flags, 4, 1, fd);
      CHECK_COND(res!=1, "reading edge %d flags", i)
      res = fread(&srcId, 4, 1, fd);
      CHECK_COND(res!=1, "reading edge %d source ID", i)
      res = fread(&sinkId, 4, 1, fd);
      CHECK_COND(res!=1, "reading edge %d sink ID", i)
      
#if DEBUG_PARSE_CFG
      fprintf (stderr, "Parsed edge %u -> %u of type %u, with flags 0x%x\n", 
                  srcId, sinkId, (unsigned int)type, _flags);
#endif
      
      Node *src = nodes[srcId];
      if (src==NULL) {
         fprintf(stderr, "Edge %d has source node with old ID %d which we do not have.\n", i, srcId);
         assert(src!=NULL);
      }
      Node *sink;
      if (sinkId == (uint32_t)-1)
         sink = static_cast<Node*>(cfg_exit);
      else
      {
         sink = nodes[sinkId];
         if (sink==NULL) {
            fprintf(stderr, "Edge %d has sink node with old ID %d which we do not have.\n", i, sinkId);
            assert(sink!=NULL);
         }
      }
      Edge *ee = new Edge(src, sink, (EdgeType)type);
      ee->flags = _flags;
      add(ee);
      
      if (ee->isCounterEdge())
      {
         res = fread(&ee->counter, 8, 1, fd);
      }
      ++i;
   } while (1);
   
   // Compute top nodes and node levels after loading graph
   computeTopNodes();
   removeCfgFlags(CFG_GRAPH_IS_MODIFIED);
   ComputeNodeRanks();
   return (0);
   
load_error:
   fprintf(stderr, "Invalid CFG file format while reading graph for routine %s\n", 
       routine_name.c_str());
   return (-1);
}

void
CFG::Node::write_label(ostream& os, addrtype offset, int info, bool html_escape)
{
   PrivateCFG::Node::write_label(os, offset, info, html_escape);
   if (isCounterNode())
      os << "\\nM=" << counter;
   else if (hasComputedCount())
      os << "\\nC=" << counter;
   else if (hasPartialCount())
      os << "\\nP=" << counter;
   // write also the block rank
   os << ", R=" << rank;
}

void
CFG::Edge::write_label (ostream& os)
{
   os << "label=\"E" << id;
   if (isCounterEdge())
      os << "\\nM=" << counter;
   else if (hasGuessedCount())
      os << "\\nG=" << counter;
   else if (hasCount())
      os << "\\nC=" << counter;
   os << "\",";
}

class NodeSortInfo
{
public:
   CFG::Node *nn;
   int counterEdges;
   int totalEdges;
   
   NodeSortInfo()
   {
      nn = 0;
      counterEdges = totalEdges = 0;
   }
   NodeSortInfo(CFG::Node *_n, int cntE, int totE)
   {
      nn = _n;
      counterEdges = cntE;
      totalEdges = totE;
   }
};

class NodeSortCompare : public  std::binary_function <NodeSortInfo, NodeSortInfo, bool>
{
public:
   NodeSortCompare ()
   { }
   
   bool operator () (const NodeSortInfo& n1, const NodeSortInfo& n2) const
   {
      return (n1.totalEdges-n1.counterEdges < n2.totalEdges-n2.counterEdges);
   }
};

typedef std::multiset<NodeSortInfo, NodeSortCompare> NodeSortSet;

void
CFG::deleteUntakenCallSiteReturnEdges()
{
   IncomingEdgesIterator ieit(CfgExit());
   int delEdges = 0;
   while ((bool)ieit)
   {
      Edge *e = ieit;
      ++ ieit;
      if (e->type==MIAMI_RETURN_EDGE && *(e->Counter())==0 && e->source()->isCallSurrogate())
      {
         remove (e);
         delete (e);
         ++ delEdges;
      }
   }

#if DEBUG_CFG_COUNTS
   DEBUG_CFG(1, 
      fprintf (stderr, "Routine %s, removed %d untaken call site return edges\n", 
             routine_name.c_str(), delEdges);
   )
#endif
   // Compute top nodes and node levels after loading graph
   computeTopNodes();
   removeCfgFlags(CFG_GRAPH_IS_MODIFIED);
   ComputeNodeRanks();
}

int
CFG::computeBBAndEdgeCounts()
{
   // traverse all nodes
   // - for each node check how many incoming and outgoing edges have counts
   // - if node has counts on all edges on one side, we can compute the node frequency
   //   - add node to heap queue with key equal to number of unsolved edges (on the other side)
   // 
   // while heap queue not empty
   // - remove top node; unsolved egdes should be 1 or less (break if false)
   // - iterate over its incoming or outgoing edges and sum up their counts (if known)
   // - compute count for unsolved edge (node freq - sum other edges)
   // - check node at other end of edge 
   //   - if node did not have count and all edges on that side are solved,
   //     compute its frequency and add node to queue for edges on the other side
   //
   // if exited before computing all counts, something is wrong. I may not have enough edges
   // with counts in the initial CFG
   int nNodes = NumNodes();
   int *inEdges = new int[nNodes];
   int *outEdges = new int[nNodes];
   memset(inEdges, 0, nNodes*sizeof(int));
   memset(outEdges, 0, nNodes*sizeof(int));
   
   NodeSortSet inNodes, outNodes;
   // our nodes' addresses are already link time addresses
   addrtype base_addr = 0; // InRoutine()->InLoadModule()->BaseAddr();
   bool has_nonzero = false;
   
   EdgesIterator eit(*this);
   while ((bool)eit)
   {
      Edge *ee = (Edge*)eit;
      if (ee == NULL)
         cerr << "In computeBBAndEdgeCounts, EdgeIterator returned NULL edge, why?" << endl;
      else
      {
//         cerr << "In computeBBAndEdgeCounts, EdgeIterator returned valid edge with id" 
//              << ee->getId() << endl;
         if (ee->hasCount())  // mark it for both its source and sink nodes
         {
            outEdges[ee->source()->getId()] += 1;
            inEdges[ee->sink()->getId()] += 1;
            ee->setExecCount(ee->counter);
            ee->setSecondExecCount(ee->counter);  // is this ever used?
            if (ee->counter > 0)
               has_nonzero = true;
         }
      }
      ++ eit;
   }
   
   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      ObjId nid = nn->getId();
      // if this is a routine entry with a single incoming edge (the entry edge),
      // then we can upgrade the partial count to a full measured count
      if (nn->hasPartialCount() && nn->isRoutineEntry() && nn->num_incoming()==1)
      {
         nn->flags = nn->flags & ~(NODE_PARTIAL_COUNT);
         nn->flags |= CFG_COUNTER_NODE;
      }
      
      if (nn->hasCount())
      {
         if (nn->counter > 0)
            has_nonzero = true;
         
         int incDiff = nn->num_incoming() - inEdges[nid];
         // try to discover any additional entry nodes
         if (incDiff==0 && inEdges[nid]>0)  // make sure the counts match
         {
            uint64_t n_count = 0;
            IncomingEdgesIterator ieit(nn);
            while ((bool)ieit)
            {
               Edge *ie = ieit;
               BriefAssertion (ie->hasCount());
               n_count += *(ie->Counter());
               ++ ieit;
            }
            // the incoming count can be equal or less (if we did not recognize an entry edge).
            // Can it be greater? assert for now
            assert(n_count <= *(nn->Counter()));
            if (n_count < *(nn->Counter()))  // if less, add an entry edge, brrr
            {
                uint64_t i_count = *(nn->Counter()) - n_count;
                // this could be an entry block, so add an entry edge
                assert(! nn->isRoutineEntry());
                Node *ex = CfgEntry();
                Edge *re = static_cast<Edge*>(addEdge(ex, nn, MIAMI_CFG_EDGE));
                assert (re);
                removeCfgFlags(CFG_GRAPH_IS_MODIFIED);
                
                *(re->Counter()) = i_count;
                re->setExecCount(i_count);
                re->setSecondExecCount(i_count);  // is this ever used?
                re->flags |= EDGE_COUNT_COMPUTED;
                inEdges[nid] += 1;

                outEdges[ex->getId()] += 1;
                if (ex->hasCount())
                {
                   i_count += *(ex->Counter());
                   *(ex->Counter()) = i_count;
                   ex->setExecCount(i_count);
                   ex->setBlockCount(i_count);
                }

                nn->flags |= ROUTINE_ENTRY_NODE;
                topNodes.push_back(nn);
            }
         }
         
         if (incDiff == 1)  // push node in queue
            inNodes.insert(NodeSortInfo(nn, inEdges[nid], nn->num_incoming()));

         int outDiff = nn->num_outgoing() - outEdges[nid];
         if (outDiff == 1)  // push node in queue
            outNodes.insert(NodeSortInfo(nn, outEdges[nid], nn->num_outgoing()));
         nn->setExecCount(nn->counter);
         nn->setBlockCount(nn->counter);
      }
      
      if (!nn->hasCount() && outEdges[nid]==nn->num_outgoing() && outEdges[nid]>0)  // out edge counts computed
      {
         uint64_t n_count = 0;
         OutgoingEdgesIterator oeit(nn);
         while ((bool)oeit)
         {
            Edge *oe = oeit;
            BriefAssertion (oe->hasCount());
            n_count += *(oe->Counter());
            ++ oeit;
         }
         // set the counter for this node
         *(nn->Counter()) = n_count;
         nn->setExecCount(n_count);
         nn->setBlockCount(n_count);
         nn->flags |= NODE_COUNT_COMPUTED;
         int incDiff = nn->num_incoming() - inEdges[nid];
         if (incDiff == 1)  // push node in queue
            inNodes.insert(NodeSortInfo(nn, inEdges[nid], nn->num_incoming()));
      }

      if (!nn->hasCount() && inEdges[nid]==nn->num_incoming() && inEdges[nid]>0)  // inc edge counts computed
      {
         uint64_t n_count = 0;
         IncomingEdgesIterator ieit(nn);
         while ((bool)ieit)
         {
            Edge *ie = ieit;
            BriefAssertion (ie->hasCount());
            n_count += *(ie->Counter());
            ++ ieit;
         }
         // set the counter for this node
         *(nn->Counter()) = n_count;
         nn->setExecCount(n_count);
         nn->setBlockCount(n_count);
         nn->flags |= NODE_COUNT_COMPUTED;
         int outDiff = nn->num_outgoing() - outEdges[nid];
         if (outDiff == 1)  // push node in queue
            outNodes.insert(NodeSortInfo(nn, outEdges[nid], nn->num_outgoing()));
      }
      ++ nit;
   }
   
   // maintain lists of nodes and edges for which I had to guess the count,
   // or for which the count was derived after guessing some other counter.
   NodeList gCountNodes;
   EdgeList gCountEdges;
   Node *guessNode = 0;
   int num_failures = 0;
   Node *failNode = 0;
   Edge *failEdge = 0;
//   int64_t guessCounter = 0;
   
   // while inNodes and outNodes are not empty, I need to check them to see if we can 
   // compute counts for new edges and propagate the counts further
   int iter = 0;
   while (!inNodes.empty() || !outNodes.empty())
   {
       // print a debug message with the size of inNodes and outNodes at the start of the big loop
#if DEBUG_CFG_COUNTS
       DEBUG_CFG(1, 
          cerr << "InRoutine " << routine_name << ", compute counts step " << iter
               << " inNodes.size=" << inNodes.size() << " and outNodes.size=" 
               << outNodes.size() << endl;
       );
#endif
       
       NodeSortSet::iterator nsit = inNodes.begin();
       while (nsit!=inNodes.end())
       {
          ObjId nid = nsit->nn->getId();
          int diffE = nsit->nn->num_incoming() - inEdges[nid];
#if DEBUG_CFG_COUNTS
          DEBUG_CFG(6, 
             cerr << "ComputeCounts: from inNodes, processing node " << *(nsit->nn) 
                  << " with in-diffE=" << diffE << endl;
          );
#endif
          assert (diffE < 2);
          
          BriefAssertion (nsit->nn->hasCount());
          int64_t i_count = *(nsit->nn->Counter());
          IncomingEdgesIterator ieit(nsit->nn);
          Edge *theEdge = 0;
          while ((bool)ieit)
          {
             Edge *ie = ieit;
             if (ie->hasCount())
                i_count -= *(ie->Counter());
             else
             {
                // save a pointer to the edge without count
                // there must be only one
                BriefAssertion (theEdge == 0);
                theEdge = ie;
             }
             ++ ieit;
          }
          if (i_count<0) 
          {
             if (guessNode)  // we are in guess mode
             {
                // my initial guess is wrong
                num_failures += 1;
#if DEBUG_CFG_COUNTS
                DEBUG_CFG(2, 
                   fprintf(stderr, "In guess mode, negative edge count failure at E%d, must increase original guess node B%d count by %" PRId64 "\n",
                        (theEdge!=0?theEdge->getId():-1), guessNode->getId(), -i_count);
                )
#endif
                if ((theEdge && failEdge==theEdge) || (!theEdge && failNode==nsit->nn)) // same failure twice in a row
                {
                   draw_CFG("Repeat_NegativeCounters_In", base_addr);
                   assert ((theEdge && failEdge!=theEdge) || (!theEdge && failNode!=nsit->nn) ||
                           !"Repeat Edge or Node Failure");
                }
                assert (num_failures<100 || !"Too many failures");
                if (theEdge)
                {
                   failEdge = theEdge;
                   failNode = 0;
                }
                else
                {
                   failEdge = 0;
                   failNode = nsit->nn;
                }

                // I need to increase my starting guess counter by -i_count;
                *(guessNode->Counter()) += (-i_count);
                guessNode->setExecCount(guessNode->counter);
                guessNode->setBlockCount(guessNode->counter);
                // reset the COUNT_COMPUTED flag for all nodes and edges in the guess lists
                NodeList::iterator nlit = gCountNodes.begin();
                for ( ; nlit!=gCountNodes.end() ; ++nlit)
                {
                   Node *n1 = *nlit;
                   n1->flags &= ~(NODE_COUNT_COMPUTED);
                }
                EdgeList::iterator elit = gCountEdges.begin();
                for ( ; elit!=gCountEdges.end() ; ++elit)
                {
                   Edge *e1 = *elit;
                   e1->flags &= ~(EDGE_COUNT_COMPUTED);
                   outEdges[e1->source()->getId()] -= 1;
                   inEdges[e1->sink()->getId()] -= 1;
                }
                gCountNodes.clear();
                gCountEdges.clear();
                inNodes.clear();
                outNodes.clear();
                // Now add the guessNode back to inNodes and outNodes as needed
                int nid = guessNode->getId();
                if (outEdges[nid]+1 == guessNode->num_outgoing())
                   outNodes.insert(NodeSortInfo(guessNode, outEdges[nid], guessNode->num_outgoing()));
                if (inEdges[nid]+1 == guessNode->num_incoming())
                   inNodes.insert(NodeSortInfo(guessNode, inEdges[nid], guessNode->num_incoming()));
                break;
             } else
             {
                // not guess mode, it is an error
                // print the CFG right here
                fprintf(stderr, "Computed negative counter value %" PRId64 " for incoming edge %d from node B%d\n", 
                     i_count, (theEdge!=0?theEdge->getId():-1), nsit->nn->getId());
                draw_CFG("Negative_Counter_In", base_addr);
                BriefAssertion (i_count>=0 || !"Computed negative count for incoming edge.");
             }
          }
          BriefAssertion (i_count>=0 || !"Computed negative count for edge.");

          if (diffE == 0)
          {
             // this assert is spurious. The node must have count computed to even be in the queue.
             // I just need to remove the entry from the queue in this case, maybe print a debug msg.
#if DEBUG_CFG_COUNTS
             DEBUG_CFG(3, 
                cerr << "InRoutine " << routine_name << ", node " << *(nsit->nn)
                     << " found in inNodes queue, but all inc edges have counts!!" << endl;
             );
#endif
             // check that the counts on both sides match (Kirkoff's law)
             BriefAssertion (theEdge==0 || !"Found edge without count, but I was expecting all edges to have counts");
             if (i_count > 0)  // node count is greater than sum of its incoming edges
             {
                // this could be an entry block, so add an entry edge
                /*** HERE ***/
                assert(! nsit->nn->isRoutineEntry());
                Node *ex = CfgEntry();
                Edge *re = static_cast<Edge*>(addEdge(ex, nsit->nn, MIAMI_CFG_EDGE));
                assert (re);
                removeCfgFlags(CFG_GRAPH_IS_MODIFIED);
                
                *(re->Counter()) = i_count;
                re->setExecCount(i_count);
                re->setSecondExecCount(i_count);  // is this ever used?
                re->flags |= EDGE_COUNT_COMPUTED;
                inEdges[nid] += 1;
                if (guessNode)   // this should not be true
                   gCountEdges.push_back(re);

                outEdges[ex->getId()] += 1;
                if (ex->hasCount())
                {
                   i_count += *(ex->Counter());
                   *(ex->Counter()) = i_count;
                   ex->setExecCount(i_count);
                   ex->setBlockCount(i_count);
                   if (guessNode)
                      gCountNodes.push_back(ex);
                }

                nsit->nn->flags |= ROUTINE_ENTRY_NODE;
                topNodes.push_back(nsit->nn);
             }
          } else
          {
             assert (diffE == 1);
             BriefAssertion (theEdge || !"Found no edge without count");
             // if the source node is a call surrogate with count of 0, then
             // I should not attempt to propagate it ...
             // Wait on this change. I am debugging an issue in NWChem, and
             // I have branch edges that do not have the right count.
             // Must debug that issue first.
             *(theEdge->Counter()) = i_count;
             theEdge->setExecCount(i_count);
             theEdge->setSecondExecCount(i_count);  // is this ever used?
             theEdge->flags |= EDGE_COUNT_COMPUTED;
             inEdges[nid] += 1;
             if (guessNode)
                gCountEdges.push_back(theEdge);
                
             // now I must look at the edges's source node and incremenet its number
             // of outgoing edges with counts
             // do this only if the source node is not a callSurrogate node and the edge
             // sink is not a counter node (count measured instead of computed). 
             // I do not want to propagate counts across call surrogates. Each side should
             // propagate counts independently to detect calls that never return.
             // If the two sides have different counts, create a return edge to the 
             // cfg_exit node.
             Node *src = theEdge->source();
             ObjId srcId = src->getId();
             outEdges[srcId] += 1;
             if (!src->isCallSurrogate() || !nsit->nn->isCounterNode() || nsit->nn->hasPartialCount())
             {
                if (!src->hasCount() && outEdges[srcId] == src->num_outgoing())  // all counts computed
                {
                   int64_t n_count = 0;
                   OutgoingEdgesIterator oeit(src);
                   while ((bool)oeit)
                   {
                      Edge *oe = oeit;
                      BriefAssertion (oe->hasCount());
                      n_count += *(oe->Counter());
                      ++ oeit;
                   }
                   // set the counter for this node
                   if (src->hasPartialCount() && (int64_t)(*(src->Counter()))>n_count)
                   {
                      // if this is a partial count node and I computed a count smaller
                      // than what I measured, it means that somthing is rotten in Denmark
                      if (guessNode)  // we are in guess mode
                      {
                         // my initial guess is wrong
                         num_failures += 1;
#if DEBUG_CFG_COUNTS
                         DEBUG_CFG(2, 
                            fprintf(stderr, "In guess mode, partial node count failure at B%d, must increase original guess node B%d count by %" PRId64 "\n",
                                src->getId(), guessNode->getId(), *(src->Counter()) - n_count);
                         )
#endif
                         if (failNode == src) // same failure twice in a row
                         {
                            draw_CFG("Repeat_SmallerPartialCounter_In", base_addr);
                            assert (failNode!=src || !"Repeat Node Failure");
                         }
                         assert (num_failures<100 || !"Too many failures");
                         failEdge = 0;
                         failNode = src;
                         
                         // I need to increase my starting guess counter;
                         *(guessNode->Counter()) += (*(src->Counter()) - n_count);
                         guessNode->setExecCount(guessNode->counter);
                         guessNode->setBlockCount(guessNode->counter);
                         // reset the COUNT_COMPUTED flag for all nodes and edges in the guess lists
                         NodeList::iterator nlit = gCountNodes.begin();
                         for ( ; nlit!=gCountNodes.end() ; ++nlit)
                         {
                            Node *n1 = *nlit;
                            n1->flags &= ~(NODE_COUNT_COMPUTED);
                         }
                         EdgeList::iterator elit = gCountEdges.begin();
                         for ( ; elit!=gCountEdges.end() ; ++elit)
                         {
                            Edge *e1 = *elit;
                            e1->flags &= ~(EDGE_COUNT_COMPUTED);
                            outEdges[e1->source()->getId()] -= 1;
                            inEdges[e1->sink()->getId()] -= 1;
                         }
                         gCountNodes.clear();
                         gCountEdges.clear();
                         inNodes.clear();
                         outNodes.clear();
                         // Now add the guessNode back to inNodes and outNodes as needed
                         int nid = guessNode->getId();
                         if (outEdges[nid]+1 == guessNode->num_outgoing())
                            outNodes.insert(NodeSortInfo(guessNode, outEdges[nid], guessNode->num_outgoing()));
                         if (inEdges[nid]+1 == guessNode->num_incoming())
                            inNodes.insert(NodeSortInfo(guessNode, inEdges[nid], guessNode->num_incoming()));
                         break;
                      } else
                      {
                         // not guess mode, it is an error
                         fprintf(stderr, "Computed smaller counter value %" PRId64 " for partial count node %d with partial counter %" PRId64 "\n", 
                             n_count, src->getId(), *(src->Counter()));
                         draw_CFG("SmallerPartialCounter_In", base_addr);
                         BriefAssertion (i_count>=0 || !"Computed smaller count for partial count node.");
                      }
                   }
                   *(src->Counter()) = n_count;
                   src->setExecCount(n_count);
                   src->setBlockCount(n_count);
                   src->flags |= NODE_COUNT_COMPUTED;
                   if (guessNode)
                      gCountNodes.push_back(src);
                   int incDiff = src->num_incoming() - inEdges[srcId];
                   if (incDiff == 1)  // push node in queue
                      inNodes.insert(NodeSortInfo(src, inEdges[srcId], src->num_incoming()));
                } else
                   if (src->hasCount() && (outEdges[srcId]+1 == src->num_outgoing()))
                      outNodes.insert(NodeSortInfo(src, outEdges[srcId], src->num_outgoing()));
             }  // source node is not call surrogate
             else  // call surrogate, check if it has a count already to compare notes
             {
                if (src->hasCount() && outEdges[srcId] == src->num_outgoing())  // all counts computed
                {
                   int64_t n_count = 0;
                   OutgoingEdgesIterator oeit(src);
                   while ((bool)oeit)
                   {
                      Edge *oe = oeit;
                      BriefAssertion (oe->hasCount());
                      n_count += *(oe->Counter());
                      ++ oeit;
                   }
                   int64_t call_cnt = *(src->Counter());
                   if (call_cnt != n_count)
                   {
                      assert (call_cnt > n_count);
#if DEBUG_CFG_COUNTS
                      DEBUG_CFG(2, 
                         cerr << "X2: InRoutine " << routine_name << ", found call surrogate node " << *src
                              << " with count " << call_cnt << " different than the sum of its edges " 
                              << n_count << endl;
                      );
#endif
                      // I should create a return edge here
                      n_count = call_cnt - n_count;
                      if (n_count<0) 
                      {
                         // print the CFG right here
                         fprintf(stderr, "Computed negative counter value %" PRId64 " of not returned call at B%d.\n", 
                               n_count, src->getId());
                         // check if guess mode, and revert on error
                         /* TODO if the assert below fails */
                         draw_CFG("Negative_Counter_call", base_addr);
                         BriefAssertion (n_count>=0 || !"Computed negative count for not returned call.");
                      }
                      Node *ex = CfgExit();
                      Edge *re = static_cast<Edge*>(addUniqueEdge(src, ex, MIAMI_RETURN_EDGE));
                      assert (re);
                      removeCfgFlags(CFG_GRAPH_IS_MODIFIED);
                      
                      *(re->Counter()) = n_count;
                      re->setExecCount(n_count);
                      re->setSecondExecCount(n_count);  // is this ever used?
                      re->flags |= EDGE_COUNT_COMPUTED;
                      outEdges[srcId] += 1;
                      if (guessNode)
                         gCountEdges.push_back(re);
                      
                      inEdges[ex->getId()] += 1;
                      if (ex->hasCount())
                      {
                         n_count += *(ex->Counter());
                         *(ex->Counter()) = n_count;
                         ex->setExecCount(n_count);
                         ex->setBlockCount(n_count);
                         if (guessNode)
                            gCountNodes.push_back(ex);
                      } else  // check if this was the first edge to connect the exit node
                      if (ex->num_incoming()==inEdges[ex->getId()])
                      {
                         int64_t ex_count = 0;
                         IncomingEdgesIterator oxit(ex);
                         while ((bool)oxit)
                         {
                            Edge *oe = oxit;
                            BriefAssertion (oe->hasCount());
                            ex_count += *(oe->Counter());
                            ++ oxit;
                         }
                         // set the count for the exit block
                         *(ex->Counter()) = ex_count;
                         ex->setExecCount(ex_count);
                         ex->setBlockCount(ex_count);
                         ex->flags |= NODE_COUNT_COMPUTED;
                         if (guessNode)
                            gCountNodes.push_back(ex);
                      }
                   }
                }
             }
          }   // else, diffE == 1
          // I must delete the current entry from the set, save and incremenet iterator first
          NodeSortSet::iterator bsit = nsit;
          ++ nsit;
          inNodes.erase(bsit);
       }  // nsit iterates over inNodes
       
       // do the same for outNodes
       nsit = outNodes.begin();
       while (nsit!=outNodes.end())
       {
          ObjId nid = nsit->nn->getId();
          int diffE = nsit->nn->num_outgoing() - outEdges[nid];
#if DEBUG_CFG_COUNTS
          DEBUG_CFG(6, 
             cerr << "ComputeCounts: from outNodes, processing node " << *(nsit->nn) 
                  << " with out-diffE=" << diffE << endl;
          );
#endif
          assert (diffE < 2);
          BriefAssertion (nsit->nn->hasCount());
          int64_t i_count = *(nsit->nn->Counter());
       
          OutgoingEdgesIterator oeit(nsit->nn);
          Edge *theEdge = 0;
          while ((bool)oeit)
          {
             Edge *oe = oeit;
             if (oe->hasCount())
                i_count -= *(oe->Counter());
             else
             {
                // save a pointer to the edge without count
                // there must be only one
                BriefAssertion (theEdge == 0);
                theEdge = oe;
             }
             ++ oeit;
          }
          if (i_count<0) 
          {
             // print the CFG right here
#if DEBUG_CFG_COUNTS
             DEBUG_CFG(2, 
                fprintf(stderr, "Computed negative counter value %" PRId64 " for outgoing edge %d from node B%d\n", 
                    i_count, (theEdge?theEdge->getId():-1), nsit->nn->getId());
             )
#endif
             if (guessNode)  // we are in guess mode
             {
                // my initial guess is wrong
                num_failures += 1;
#if DEBUG_CFG_COUNTS
                DEBUG_CFG(2, 
                   fprintf(stderr, "In guess mode, negative edge count failure at E%d, must increase original guess node B%d count by %" PRId64 "\n",
                        (theEdge?theEdge->getId():-1), guessNode->getId(), -i_count);
                )
#endif
                if ((theEdge && failEdge==theEdge) || (!theEdge && failNode==nsit->nn)) // same failure twice in a row
                {
                   draw_CFG("Repeat_NegativeCounter_Out", base_addr);
                   assert ((theEdge && failEdge!=theEdge) || (!theEdge && failNode!=nsit->nn) ||
                           !"Repeat Edge or Node Failure");
                }
                assert (num_failures<100 || !"Too many failures");
                if (theEdge)
                {
                   failEdge = theEdge;
                   failNode = 0;
                }
                else
                {
                   failEdge = 0;
                   failNode = nsit->nn;
                }
                
                // I need to increase my starting guess counter by -i_count;
                *(guessNode->Counter()) += (-i_count);
                guessNode->setExecCount(guessNode->counter);
                guessNode->setBlockCount(guessNode->counter);
                // reset the COUNT_COMPUTED flag for all nodes and edges in the guess lists
                NodeList::iterator nlit = gCountNodes.begin();
                for ( ; nlit!=gCountNodes.end() ; ++nlit)
                {
                   Node *n1 = *nlit;
                   n1->flags &= ~(NODE_COUNT_COMPUTED);
                }
                EdgeList::iterator elit = gCountEdges.begin();
                for ( ; elit!=gCountEdges.end() ; ++elit)
                {
                   Edge *e1 = *elit;
                   e1->flags &= ~(EDGE_COUNT_COMPUTED);
                   outEdges[e1->source()->getId()] -= 1;
                   inEdges[e1->sink()->getId()] -= 1;
                }
                gCountNodes.clear();
                gCountEdges.clear();
                inNodes.clear();
                outNodes.clear();
                // Now add the guessNode back to inNodes and outNodes as needed
                int nid = guessNode->getId();
                if (outEdges[nid]+1 == guessNode->num_outgoing())
                   outNodes.insert(NodeSortInfo(guessNode, outEdges[nid], guessNode->num_outgoing()));
                if (inEdges[nid]+1 == guessNode->num_incoming())
                   inNodes.insert(NodeSortInfo(guessNode, inEdges[nid], guessNode->num_incoming()));
                break;
             } else
             {
                // not guess mode, it is an error
                // well, I am not sure anymore. Perhaps I should just create an entry edge
                // at this point. I've seen this assert fail in zeus and wrf part of SPEC2006
                // Create an entry edge at this point
                if (nsit->nn->isRoutineEntry())
                {
                   fprintf(stderr, "Non guess mode, negative edge count %" PRId64 " failure at E%d, investigate!!!\n",
                        i_count, (theEdge?theEdge->getId():-1) );
                   draw_CFG("Negative_Rout_Entry", base_addr);
                   assert(! nsit->nn->isRoutineEntry());
                }

                Node *ex = CfgEntry();
                Edge *re = static_cast<Edge*>(addEdge(ex, nsit->nn, MIAMI_CFG_EDGE));
                assert (re);
                removeCfgFlags(CFG_GRAPH_IS_MODIFIED);
                      
                i_count = -i_count;
                *(re->Counter()) = i_count;
                re->setExecCount(i_count);
                re->setSecondExecCount(i_count);  // is this ever used?
                re->flags |= EDGE_COUNT_COMPUTED;
                inEdges[nid] += 1;
                if (guessNode)   // this should not be true
                   gCountEdges.push_back(re);
                   
                // Our node, nn, must have counts computed already. We must increase its
                // count value, so that it adds up to its incoming edges
                assert (nsit->nn->hasCount());
                int64_t nn_count = i_count + *(nsit->nn->Counter());
                *(nsit->nn->Counter()) = nn_count;
                nsit->nn->setExecCount(nn_count);
                nsit->nn->setBlockCount(nn_count);
                if (guessNode)
                   gCountNodes.push_back(nsit->nn);
                      
                outEdges[ex->getId()] += 1;
                if (ex->hasCount())
                {
                   int64_t ex_count = i_count + *(ex->Counter());
                   *(ex->Counter()) = ex_count;
                   ex->setExecCount(ex_count);
                   ex->setBlockCount(ex_count);
                   if (guessNode)
                      gCountNodes.push_back(ex);
                }

                nsit->nn->flags |= ROUTINE_ENTRY_NODE;
                topNodes.push_back(nsit->nn);
                
                // now the count for theEdge can be set to 0
                // let the propagation code continue
                i_count = 0;
                
             }
          }
          
          
          if (diffE == 0)
          {
             // this assert is spurious. The node must have count computed to even be in the queue.
             // I just need to remove the entry from the queue in this case, maybe print a debug msg.
#if DEBUG_CFG_COUNTS
             DEBUG_CFG(3, 
                cerr << "InRoutine " << routine_name << ", node " << *(nsit->nn)
                     << " found in outNodes queue, but all out edges have counts!!" << endl;
             );
#endif
             if (nsit->nn->isCallSurrogate())   // compare counts
             {
                if (i_count > 0)
                {
#if DEBUG_CFG_COUNTS
                   DEBUG_CFG(2, 
                      cerr << "X1: InRoutine " << routine_name << ", found call surrogate node " << *(nsit->nn)
                           << " with exec count larger by " << i_count << " than the sum of its edges " << endl;
                   );
#endif
                   // I should create a return edge here
                   Node *ex = CfgExit();
                   Edge *re = static_cast<Edge*>(addUniqueEdge(nsit->nn, ex, MIAMI_RETURN_EDGE));
                   assert (re);
                   removeCfgFlags(CFG_GRAPH_IS_MODIFIED);
                   
                   *(re->Counter()) = i_count;
                   re->setExecCount(i_count);
                   re->setSecondExecCount(i_count);  // is this ever used?
                   re->flags |= EDGE_COUNT_COMPUTED;
                   outEdges[nid] += 1;
                   if (guessNode)
                      gCountEdges.push_back(re);
                      
                   inEdges[ex->getId()] += 1;
                   if (ex->hasCount())
                   {
                      i_count += *(ex->Counter());
                      *(ex->Counter()) = i_count;
                      ex->setExecCount(i_count);
                      ex->setBlockCount(i_count);
                      if (guessNode)
                         gCountNodes.push_back(ex);
                   } else  // check if this was the first edge to connect the exit node
                   if (ex->num_incoming()==inEdges[ex->getId()])
                   {
                      int64_t ex_count = 0;
                      IncomingEdgesIterator oxit(ex);
                      while ((bool)oxit)
                      {
                         Edge *oe = oxit;
                         BriefAssertion (oe->hasCount());
                         ex_count += *(oe->Counter());
                         ++ oxit;
                      }
                      // set the count for the exit block
                      *(ex->Counter()) = ex_count;
                      ex->setExecCount(ex_count);
                      ex->setBlockCount(ex_count);
                      ex->flags |= NODE_COUNT_COMPUTED;
                      if (guessNode)
                         gCountNodes.push_back(ex);
                   }
                }
             }
          } else   // diffE == 1
          {
             assert (diffE == 1);
             BriefAssertion (theEdge || !"Found no outgoing edge without count");
             // I do not propagate counts across call surrogate nodes
             // If the source node is a call surrogate and the sink node is a counter node,
             // then just ignore these counts; I still want to drop the entry from inNodes
             Node *sink = theEdge->sink();
             if (!nsit->nn->isCallSurrogate() || !sink->isCounterNode() || sink->hasPartialCount())
             {
                *(theEdge->Counter()) = i_count;
                theEdge->setExecCount(i_count);
                theEdge->setSecondExecCount(i_count);  // is this ever used?
                theEdge->flags |= EDGE_COUNT_COMPUTED;
                outEdges[nid] += 1;
                if (guessNode)
                   gCountEdges.push_back(theEdge);
                   
                // now I must look at the edges's sink node and incremenet its number
                // of incoming edges with counts
                ObjId sinkId = sink->getId();
                inEdges[sinkId] += 1;
                if (!sink->hasCount() && inEdges[sinkId]==sink->num_incoming())  // all counts computed
                {
                   int64_t n_count = 0;
                   IncomingEdgesIterator ieit(sink);
                   while ((bool)ieit)
                   {
                      Edge *ie = ieit;
                      BriefAssertion (ie->hasCount());
                      n_count += *(ie->Counter());
                      ++ ieit;
                   }
                   // set the counter for this node
                   // check first is this is a partial count node and we computed a smaller value
                   if (sink->hasPartialCount() && (int64_t)(*(sink->Counter()))>n_count)
                   {
                      // if this is a partial count node and I computed a count smaller
                      // than what I measured, it means that somthing is rotten in Denmark
                      if (guessNode)  // we are in guess mode
                      {
                         // my initial guess is wrong
                         num_failures += 1;
#if DEBUG_CFG_COUNTS
                         DEBUG_CFG(2, 
                            fprintf(stderr, "In guess mode, partial node count failure at B%d, must increase original guess node B%d count by %" PRId64 "\n",
                                sink->getId(), guessNode->getId(), *(sink->Counter()) - n_count);
                         )
#endif
                         if (failNode == sink) // same failure twice in a row
                         {
                            draw_CFG("Repeat_SmallerPartialCounter_Out", base_addr);
                            assert (failNode!=sink || !"Repeat Node Failure");
                         }
                         assert (num_failures<100 || !"Too many failures");
                         failEdge = 0;
                         failNode = sink;
                         
                         // I need to increase my starting guess counter;
                         *(guessNode->Counter()) += (*(sink->Counter()) - n_count);
                         guessNode->setExecCount(guessNode->counter);
                         guessNode->setBlockCount(guessNode->counter);
                         // reset the COUNT_COMPUTED flag for all nodes and edges in the guess lists
                         NodeList::iterator nlit = gCountNodes.begin();
                         for ( ; nlit!=gCountNodes.end() ; ++nlit)
                         {
                            Node *n1 = *nlit;
                            n1->flags &= ~(NODE_COUNT_COMPUTED);
                         }
                         EdgeList::iterator elit = gCountEdges.begin();
                         for ( ; elit!=gCountEdges.end() ; ++elit)
                         {
                            Edge *e1 = *elit;
                            e1->flags &= ~(EDGE_COUNT_COMPUTED);
                            outEdges[e1->source()->getId()] -= 1;
                            inEdges[e1->sink()->getId()] -= 1;
                         }
                         gCountNodes.clear();
                         gCountEdges.clear();
                         inNodes.clear();
                         outNodes.clear();
                         // Now add the guessNode back to inNodes and outNodes as needed
                         int nid = guessNode->getId();
                         if (outEdges[nid]+1 == guessNode->num_outgoing())
                            outNodes.insert(NodeSortInfo(guessNode, outEdges[nid], guessNode->num_outgoing()));
                         if (inEdges[nid]+1 == guessNode->num_incoming())
                            inNodes.insert(NodeSortInfo(guessNode, inEdges[nid], guessNode->num_incoming()));
                         break;
                      } else
                      {
                         // not guess mode, it is an error
                         fprintf(stderr, "Computed smaller counter value %" PRId64 " for partial count node %d with partial counter %" PRId64 "\n", 
                              n_count, sink->getId(), *(sink->Counter()));
                         draw_CFG("SmallerPartialCounter_Out", base_addr);
                         BriefAssertion (i_count>=0 || !"Computed smaller count for partial count node.");
                      }
                   }
                   
                   *(sink->Counter()) = n_count;
                   sink->setExecCount(n_count);
                   sink->setBlockCount(n_count);
                   sink->flags |= NODE_COUNT_COMPUTED;
                   if (guessNode)
                      gCountNodes.push_back(sink);
                   int outDiff = sink->num_outgoing() - outEdges[sinkId];
                   if (outDiff==1 || (outDiff==0 && sink->isCallSurrogate()))
                      // push node in queue
                      outNodes.insert(NodeSortInfo(sink, outEdges[sinkId], sink->num_outgoing()));
                } else
                   if (sink->hasCount() && (inEdges[sinkId]+1 == sink->num_incoming()))
                      inNodes.insert(NodeSortInfo(sink, inEdges[sinkId], sink->num_incoming()));
             }
          }   // else,  diffE == 1
          // I must delete the current entry from the set, save and incremenet iterator first
          NodeSortSet::iterator bsit = nsit;
          ++ nsit;
          outNodes.erase(bsit);
       }  // nsit iterates over outNodes
       
       ++ iter;
       
       // if the two queues are empty, check if all nodes and edges have counts
       if (inNodes.empty() && outNodes.empty())
       {
          int ncNodes = 0, ncEdges = 0;  // no-count nodes and edges
          int partNodes = 0;  // nodes with partial count
          int exitEdges = 0;  // edges going into the cfg_exit node
          guessNode = 0;
          NodesIterator nnit(*this);
          while((bool)nnit)
          {
             Node *nn = nnit;
             if (! nn->hasCount())
             {
                if (! nn->isCfgExit() || has_nonzero)
                   ncNodes += 1;
                if (nn->hasPartialCount()) {
                   partNodes += 1;
                   if (! guessNode)
                      guessNode = nn;
                }
             }
             ++ nnit;
          }
          EdgesIterator edit(*this);
          while((bool)edit)
          {
             Edge *ee = edit;
             if (! ee->hasCount())
             {
                ncEdges += 1;
                if (ee->sink()->isCfgExit())
                   exitEdges += 1;
             }
             ++ edit;
          }
          if (ncNodes || ncEdges)
          {
#if DEBUG_CFG_COUNTS
             DEBUG_CFG(1, 
                fprintf (stderr, "After computing counts, found %d nodes and %d edges without counts. Of these, %d nodes have partial counts and %d edges are exit edges.\n",
                    ncNodes, ncEdges, partNodes, exitEdges);
             )
#endif
             // either all nodes and edges have counts, or some of the no count edges are
             // exit edges, or some of the no count nodes haev partial counts
             if (!partNodes || !guessNode) 
             {
                fprintf (stderr, "Routine %s: after computing counts, found %d nodes and %d edges without counts. Of these, %d nodes have partial counts and %d edges are exit edges. guessNode=B%d\n",
                    routine_name.c_str(), ncNodes, ncEdges, partNodes, exitEdges, (guessNode?guessNode->getId():(-1)));
                draw_CFG("incompleteCounts", base_addr);
                assert (partNodes && guessNode);
             }
             // start guessing from the first partial count node
             // no need to add the initial node to the guess list because its pointer
             // is stored in guessNode
//             gCountNodes.push_back(guessNode);
             gCountNodes.clear();
             gCountEdges.clear();
             guessNode->flags |= CFG_COUNTER_NODE;
             guessNode->setExecCount(guessNode->counter);
             guessNode->setBlockCount(guessNode->counter);
             int nid = guessNode->getId();
             if (outEdges[nid]+1 == guessNode->num_outgoing())
                outNodes.insert(NodeSortInfo(guessNode, outEdges[nid], guessNode->num_outgoing()));
             if (inEdges[nid]+1 == guessNode->num_incoming())
                inNodes.insert(NodeSortInfo(guessNode, inEdges[nid], guessNode->num_incoming()));
             num_failures = 0;
             failNode = 0;
             failEdge = 0;
//             guessCounter = *(guessNode->Counter());
          }
       }  // inNodes and outNodes are empty
   }  // while inNodes or outNodes are not empty
   
   return (0);
}

