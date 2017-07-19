/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: SchedDG.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements the logic for scheduling the nodes of a dependence
 * graph on the resources of an abstract machine model in a modulo fashion. 
 * The dependence graph edges correspond to dependencies between instructions
 * and must be satisfied by the resulting instruction schedule.
 * The scheduler identifies and reports the factors the constrain the 
 * schedule length.
 */

#include <math.h>
#include <instr_bins.H>

// standard headers

#ifdef NO_STD_CHEADERS
# include <stdlib.h>
# include <string.h>
# include <assert.h>
#else
# include <cstdlib>
# include <cstring>
# include <cassert>
using namespace std; // For compatibility with non-std C headers
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

using std::ostream;
using std::endl;
using std::cout;
using std::cerr;
using std::setw;

#include "dependency_type.h"
#include "SchedDG.h"
#include "debug_scheduler.h"
#include "FindCliques.h"
#include "DGDominator.h"

using namespace MIAMI;
using namespace MIAMIU;

namespace MIAMI
{
extern int haveErrors;
}

namespace MIAMI_DG 
{

#define ALTERNATE_EXTRA_LENGTH(x)  (int)(0.04*(resourceLimitedScore<2.0?resourceLimitedScore:2.0)*(x) + 0.5);

static const int SCHED_MAXIMUM_LOCAL_ATTEMPTS = 20;
static const int SCHED_MAXIMUM_LOCAL_BACKTRACK = 3;
static const int SCHED_MAXIMUM_GLOBAL_BACKTRACK = 10;

// define the exception bits; When I throw an exception, I pass a mask with
// the type of trycases I want to catch it
# define TC_NO_TRYCASE                0x0
# define TC_SHORT_BEFORE_LONG_CYCLE   0x1
# define TC_CONCENTRIC_TRYCASE        0x2
# define TC_NODE_WITH_MIXED_EDGES     0x4
# define TC_BARRIER_SLACK             0x8
# define TC_SCC_REACHED_FROM_ACYCLIC  0x10
# define TC_ALL_EXCEPTIONS            0xffffffff

//--------------------------------------------------------------------------------------------------------------------


const char *cycleColor[] = {"lawngreen", "purple", "tomato", 
          "chocolate", "gold", "aquamarine", "yellow" };

//--------------------------------------------------------------------------------------------------------------------
bool 
draw_regonly(SchedDG::Edge* e)
{
   if (e->getType()==ADDR_REGISTER_TYPE || e->getType()==GP_REGISTER_TYPE)
      return (true);
   else
      return (false);
}

bool 
draw_regmem(SchedDG::Edge* e)
{
   if ( e->getType()==ADDR_REGISTER_TYPE || e->getType()==GP_REGISTER_TYPE ||
        e->getType()==MEMORY_TYPE)
      return (true);
   else
      return (false);
}

bool 
draw_regmemsuper(SchedDG::Edge* e)
{
   switch (e->getType())
   {
      case ADDR_REGISTER_TYPE:
      case GP_REGISTER_TYPE:
      case MEMORY_TYPE:
      case SUPER_EDGE_TYPE:
         return (true);
      
      default:
         return (false);
   }
   return (false);
}

bool 
draw_all(SchedDG::Edge* e)
{
   return (true);
}

const char* regTypeToString(int tt)
{
   switch(tt)
   {
      case RegisterClass_REG_OP: return "r";
      case RegisterClass_MEM_OP: return "a";
      case RegisterClass_LEA_OP: return "l";
      case RegisterClass_STACK_REG: return "s";  // should I print the stack number at some point??
      case RegisterClass_TEMP_REG: return "t";
      
      // I should never see the following styles:
      case RegisterClass_INVALID:
      case RegisterClass_STACK_OPERATION:
      case RegisterClass_PSEUDO:
         cerr << "Error: I should not have a node register of type " << tt << endl;
         assert(!"Illegal register type");
         return (0);

      default:
         cerr << "Error: Unknown register type " << tt << endl;
         assert(!"Unknown register type");
         return (0);
   }
}

//--------------------------------------------------------------------------------------------------------------------
SchedDG::SchedDG(const char* func_name, PathID &_pathId, 
		 uint64_t _pathFreq, float _avgNumIters,
		 RFormulasMap& _refAddr, 
		 LoadModule *_img)
  : refFormulas(_refAddr), img(_img)
{
   pathId = _pathId;
   routine_name = func_name;
   // initialize the protected variables that are used by the derived class
   lastBranch = NULL;
   pathFreq = _pathFreq;
   if (pathFreq < 1)
      pathFreq = 1;
   avgNumIters = _avgNumIters;
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (1, 
      fprintf (stderr, "In SchedDG constructor for routine %s, pathId %s. AvgNumIters %f, freq %" PRIu64 "\n",
          routine_name, pathId.Name(), avgNumIters, pathFreq);
   )
#endif

  nextNodeId = 1;
  nextEdgeId = 1;
  topMarker = 1;
  numSchedEdges = 0;
  
//  Node::resetIds();
//  Edge::resetIds();
  nextCycleId = 1;
  limpModeFindCycle = false;
  limpModeScheduling = false;
  numActualCycles = 0.0;
//  compute_levels_needed = false;
  cfgFlags = 0;
  maxPathRootToLeaf = 0;
  longestCycle = 1;
  nextScheduleId = 1;
  maximumGraphLevel = 0;
  heavyOnResources = false;
  nextBarrierLevel = NULL;
  lastBarrierNode = NULL;
  nextBarrierNode = NULL;
  numStructures = 1;
  listOfNodes = NULL;
  childIdx = NULL;
  next_sibling = NULL;
  
  cycleDag = NULL;
  ds = NULL;
  numCycleSets = 0;
  longCyclesLeft = false;
  reloc_offset = 0;

  // create a node that will act as the cfg entry node
  cfg_top = new Node(this, 0L, 0, IB_dg_entry);
  cfg_top->_flags |= NODE_IS_DG_ENTRY;
  add(cfg_top);
  // and a node that will act as the cfg exit node
  cfg_bottom = new Node(this, 0L, 0, IB_dg_exit);
  cfg_bottom->_flags |= NODE_IS_DG_EXIT;
  add(cfg_bottom);
}

//--------------------------------------------------------------------------------------------------------------------
SchedDG::~SchedDG()
{
   int i;
//   fprintf (stderr, "SchedDG::DESTRUCTOR called\n");
   // if cycles were computed, then delete them
   if (HasAllFlags(DG_CYCLES_COMPUTED))
      removeAllCycles();
      
   if (childIdx)
      delete[] childIdx;
   if (next_sibling)
      delete[] next_sibling;
   if (listOfNodes)
   {
      for (i=0 ; i<numStructures ; ++i)
         if (listOfNodes[i])
            delete listOfNodes[i];
      delete[] listOfNodes;
   }

   if (nextBarrierLevel)
      delete[] nextBarrierLevel;
   if (lastBarrierNode)
      delete[] lastBarrierNode;
   if (nextBarrierNode)
      delete[] nextBarrierNode;
   
   // I have to delete all nodes and all edges
   EdgesIterator edit(*this);
   while ((bool)edit)
   {
      Edge *edge = edit;
      delete edge;
      ++ edit;
   }
   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *node = nit;
      delete node;
      ++ nit;
   }

   RemovedNodesIterator rnit(*this);
   while ((bool)rnit)
   {
      Node *node = rnit;
      delete node;
      ++ rnit;
   }

   node_set.clear();
   edge_set.clear();
   removed_node_set.clear();
   
   if (cycleDag)
      delete (cycleDag);
   if (ds)
      delete (ds);

   cfgFlags = 0;
}

//--------------------------------------------------------------------------------------------------------------------
void 
SchedDG::add (DGraph::Edge* de)
{
   Edge *e = static_cast<SchedDG::Edge*>(de);
   if (e->isSchedulingEdge())
   {
      setCfgFlags(DG_GRAPH_IS_MODIFIED);
      removeCfgFlags(DG_GRAPH_PRUNED);
   }
   DGraph::add(e);
}

//--------------------------------------------------------------------------------------------------------------------
void 
SchedDG::add (DGraph::Node* n)
{
   setCfgFlags(DG_GRAPH_IS_MODIFIED);
   DGraph::add(n);
}

//--------------------------------------------------------------------------------------------------------------------
void 
SchedDG::remove (DGraph::Edge* de)
{
   Edge *e = static_cast<SchedDG::Edge*>(de);
   if (e->isSchedulingEdge())
   {
      // if the graph has its cycles computed, print error and terminate
      // program. This is a logical error in the program because cycles are
      // fragile.
      if (HasAllFlags (DG_CYCLES_COMPUTED))
         assert(!"Cannot remove an edge after cycles have been computed. \
         Either remove all the cycles, or compute cycles only after all the \
         transformations on the graph are completed.");

      setCfgFlags(DG_GRAPH_IS_MODIFIED);
   }
   DGraph::remove(e);
}

//--------------------------------------------------------------------------------------------------------------------
void 
SchedDG::remove (DGraph::Node* n)
{
   // if the graph has its cycles computed, print error and terminate
   // program. This is a logical error in the program because cycles are
   // fragile.
   if (HasAllFlags( DG_CYCLES_COMPUTED))
      assert(!"Cannot remove an edge after cycles have been computed. \
      Either remove all the cycles, or compute cycles only after all the \
      transformations on the graph are completed.");

   setCfgFlags(DG_GRAPH_IS_MODIFIED);
   Node *nn = static_cast<Node*>(n);
   removed_node_set.insert (nn);
   DGraph::remove(n);
}
//--------------------------------------------------------------------------------------------------------------------

SchedDG::Edge* 
SchedDG::addDependency(Node* n1, Node* n2, int _ddirection, int _dtype, 
            int _ddistance, int _dlevel, int _overlapped, float _eProb )
{
   Edge* edge = new Edge(n1, n2, _ddirection, _dtype, _ddistance, 
          _dlevel, _overlapped, _eProb);
   add(edge);
   return (edge);
}
//--------------------------------------------------------------------------------------------------------------------

SchedDG::Edge*
SchedDG::addUniqueDependency(Node* n1, Node* n2, int _ddirection, 
            int _dtype, int _ddistance, int _dlevel, int _overlapped,
            float _eProb )
{
   OutgoingEdgesIterator oeit(n1);
   while ((bool)oeit)
   {
      if ( oeit->sink()==n2 && oeit->getType()==_dtype &&
           oeit->getDirection()==_ddirection && 
           oeit->getDistance()==_ddistance && oeit->getLevel()==_dlevel)
      {
         if (_dtype==CONTROL_TYPE && _eProb<oeit->getProbability())
            oeit->_prob = _eProb;
         return (oeit);
      }
      ++ oeit;
   }
   return (addDependency(n1, n2, _ddirection, _dtype, _ddistance, 
              _dlevel, _overlapped, _eProb));
}
//--------------------------------------------------------------------------------------------------------------------

#if 0
SchedDG::Edge*
SchedDG::updateSuperEdge (Node *src, Node *dest, EdgeList &el)
{
   // I should invoke findDependencyCycles only after latency was computed
   // Therefore, I should always have latency information for edges when I am
   // here. Add an assert in case any edge does not have latency info.
   bool hasLat = true;
   unsigned int newLat = 0;
   int iters = 0;  // count how many iters it crosses;
   int level = 0;
   Edge *newEdge;
   bool found = false;
   double numDiffPaths = 1.0;
   EdgeList::iterator elit = el.begin();
   for ( ; elit!=el.end() ; ++elit )
   {
      if ((*elit)->getDistance() > 0)
         iters += (*elit)->getDistance();
      if ((*elit)->HasLatency())
         newLat += (*elit)->getLatency();
      else
      {
         hasLat = false;
         assert (!"In updateSuperEdge, found edge without latency information.");
         break;
      }
      if ((*elit)->isSuperEdge())
      {
         assert ( (*elit)->numPaths );
         numDiffPaths *= (*elit)->numPaths;
      }
   }
   if (iters > 0)
      level = 1;
   
   // I am not going to accept super edges that cross multiple iterations.
   // Or I should check only not to have more than one backedge, but allow 
   // super edges with iters==2 if it has a carried dependency with 
   // distance 2? Ahh, too difficult. I will just reject super edges between
   // two barrier nodes if they have a number of iterations larger than another
   // super edge between same two nodes. 
   // 03/21/2007 mgabi: Unfortunately I found valid cases where I can have a 
   // carried memory dependency between nodes part of the same structure.
   // I should keep track of the paths with larger number of iterations, 
   // because I have to keep track of the slack for them.
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (8, 
      cerr << "Update super edge from " << src->id << " to " << dest->id
           << ": newLat=" << newLat << ", iters=" << iters << endl;
   )
#endif
   OutgoingEdgesIterator oeit(src);
   while ((bool)oeit)
   {
      if (oeit->sink()==dest && oeit->getType()==SUPER_EDGE_TYPE)
//                  && oeit->getDistance()==iters) // found one
      {
         // we must have latency per our assumptions. I will not test the 
         // case without latency anymore
         if (oeit->getDistance()>iters)
         {
            // update latency and distance information
            oeit->minLatency = newLat;
            oeit->realLatency = newLat;
            oeit->ddistance = iters;
            if (iters==0)
               oeit->dlevel = 0;
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (8, 
               cerr << "Older edge updated to fewer iters." << endl;
            )
#endif
         } else
         if (oeit->getDistance()==iters && oeit->getLatency()<newLat)
//         if (oeit->getLatency()<newLat)
         {
            // update latency information
            oeit->minLatency = newLat;
            oeit->realLatency = newLat;
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (8, 
               cerr << "Older edge updated to longer distance." << endl;
            )
#endif
         }
         found = true;
         newEdge = oeit;
         break;
      }
      ++ oeit;
   } 
   
   if (!found)
   {
      // did not find any super edge with the same distance between these nodes; 
      // create one
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (5, 
         if (iters > 1)
         {
            cerr << "##> Found multi-iteration super edge. Nodes:" << endl;
            Node *lastSENode = NULL;
            for (elit=el.begin() ; elit!=el.end() ; ++elit )
            {
               cerr << *((*elit)->source()) << " ++ ";
               lastSENode = (*elit)->sink();
            }
            assert (lastSENode != NULL);
            cerr << *lastSENode << endl;
         }
      )
#endif
      newEdge = addDependency(src, dest, OUTPUT_DEPENDENCY, SUPER_EDGE_TYPE,
                 iters, level, 1);
      newEdge->minLatency = newLat;
      newEdge->realLatency = newLat;
      // setting the HAS_BYPASS flag ensures the latency is not recomputed
      // when the source node is scheduled. For regular edges real latency is 
      // computed after the source node is scheduled because an instructions
      // can have several different length templates.
      newEdge->_flags |= (LATENCY_COMPUTED_EDGE | HAS_BYPASS_RULE_EDGE);
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (5, 
           cerr << "##> Added a new super edge: " << *newEdge << endl;
      )
#endif
   }
   newEdge->numPaths += numDiffPaths;
   
   // right here update the super edge info for each edge and node
   for (elit = el.begin() ; elit!=el.end() ; ++elit )
   {
      Edge *oldEdge = (*elit)->superE;
      assert (oldEdge==0 || oldEdge==newEdge);
      if (oldEdge==NULL)
#if 0
       || oldEdge->getDistance()>iters) ||
             (oldEdge->getDistance()==iters && 
                (oldEdge->source()->getId()<src->getId() ||
                 oldEdge->sink()->getId()>dest->getId())
             ) )
#endif
         (*elit)->superE = newEdge;

#if 0
      if ((*elit)->numSuperEdges == 0)  // first superE for this edge
      {
         (*elit)->numSuperEdges = 1;
         (*elit)->superE = newEdge;
      } else
      {
         Edge *oldEdge = (*elit)->superE;
         // For each edge I should store how many super-edges it has, a direct
         // pointer to the most restrictive one (the smallest distance?), and 
         // a STL map from distance to path.
         assert (oldEdge==newEdge || oldEdge->ddistance!=iters);
         if (oldEdge != newEdge)  // check which one is more restrictive
         {
            if (! (*elit)->superEdges)  // we have only one other superE
            {
               bool edgeFound = false;
               assert ((*elit)->numSuperEdges == 1);
               (*elit)->numSuperEdges = 2;
               (*elit)->superEdges = new UiEdgeMap ();
               (*elit)->superEdges.insert (UiEdgeMap::value_type
                         (oldEdge->ddistance, oldEdge));
               (*elit)->superEdges.insert (UiEdgeMap::value_type
                         (iters, newEdge));
            } else
            {
               UiEdgeMap::iterator emit = (*elit)->superEdges->find (iters);
               if (emit == (*elit)->superEdges->end())   // not found
                  (*elit)->superEdges.insert (UiEdgeMap::value_type
                            (iters, newEdge));
               else
               {
                  assert (emit->second == newEdge);
                  edgeFound = true;
               }
            }
            // finally, check if the new edge is the most restrictive edge
            // If the super-edge was already associated with this edge, then
            // I know it cannot be more restrictive
            if (!edgeFound && iters < oldEdge->ddistance)
               (*elit)->superE = newEdge;
         }
      }
#endif
      
      // now for nodes; do not set it for the two ends of the super-edge
      Node *nsink = (*elit)->sink();
      if (nsink == dest)  // this is the last edge+node; assert
         break;
      // do I need to keep track of all super-edges for a node as well?
      // I think not. Keep track of the most restrictive one only.
      // Check where we use the super edge associated with a node.
      oldEdge = nsink->superE;
      // with new changes, oldEdge should be NULL; is this true?
      assert (oldEdge==0 || oldEdge==newEdge);
      if (oldEdge==NULL)
#if 0
       || oldEdge->getDistance()>iters) ||
             (oldEdge->getDistance()==iters && 
                (oldEdge->source()->getId()<src->getId() ||
                 oldEdge->sink()->getId()>dest->getId())
             ) )
#endif
         nsink->superE = newEdge;
   }
   
   return (newEdge);
}
//--------------------------------------------------------------------------------------------------------------------
#endif

SchedDG::Edge::Edge (SchedDG::Node* n1, SchedDG::Node* n2, 
     int _ddirection, int _dtype, int _ddistance, int _dlevel, int _overlapped,
     float _eProb )
  : DGraph::Edge(n1, n2), ddirection(_ddirection), dtype(_dtype), 
       ddistance(_ddistance), dlevel(_dlevel), overlapped(_overlapped), 
       _prob(_eProb)
{
  id = n1->in_cfg()->nextEdgeId++;
  minLatency = 1;  // initialize all edges with a default latency
  realLatency = NO_LATENCY;
  /*
 * ozgurS
 * initialization of new variables
 */
  minMemLatency = 0;  // initialize all edges with a default latency
  realMemLatency = NO_LATENCY;
  minCPULatency = 1;  // initialize all edges with a default latency
  realCPULatency = NO_LATENCY;
  longestCycleMem = 0;
  longestCycleCPU = 0;

//ozgurE
  _flags = 0;
  resourceScore = 0;
  longestCycle = 0;
  sumAllCycles = 0;
  longestPath = 0;
  neighbors = 0;
  visibility_flag = 0;
  numPaths = 0.0;
  structureId = 0;
  longestStructPath = 0;
  usedSlack = 0;
  maxRemaining = 0;
  remainingEdges = 0;
  _visited = 0;
  _visited2 = 0;
#if 0
  numSuperEdges = 0;
  superEdges = NULL;
#endif
  superE = NULL;
  subEdges = NULL;
//  pathsIds = NULL;
}
//--------------------------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------------------------

void SchedDG::Edge::dump (ostream& os)
{
  os << "{ " << id << " " 
     << dependencyDirectionToString(ddirection) << " " 
     << dependencyTypeToString(dtype) << " " 
     << dependencyDistanceToString(ddistance) << " " 
     << "Level=" << dlevel << " }";
  os.flush();
}
//--------------------------------------------------------------------------------------------------------------------


void
SchedDG::dump (ostream& os)
{
  os << "===== SchedDG dump =====" << endl << "    PathId: " << hex << pathId << endl << "--------------------" << endl;
  // print the contents of all the nodes
  NodesIterator nodes_iter(*this);
  while ((bool)nodes_iter) {
    nodes_iter->longdump(this, os);
    os << endl;
    ++nodes_iter;
  }

  os << "====================" << endl;
}
//--------------------------------------------------------------------------------------------------------------------

void 
SchedDG::ComputeNodeLevels()
{
   computeTopNodes();
   // from each top node perform a DFS on loop independent edges and
   // compute a level for each node
   OutgoingEdgesIterator teit(cfg_top);
   unsigned int mm = new_marker();
   // compute also the maximum graph level
   maximumGraphLevel = 0;
   while ((bool)teit)
   {
      Node *nn = teit->sink();
      recursiveNodeLevels(nn, 1, mm);
      ++ teit;
   }
   // I would like to get the levels of barrier nodes and a way to, 
   // given a level, find out on what level is the next barrier node.
   // I also need a data structure to give me the last barrier node.
   if (nextBarrierLevel)
      delete[] nextBarrierLevel;
   nextBarrierLevel = new int[maximumGraphLevel+1];
   memset (nextBarrierLevel, 0, (maximumGraphLevel+1)*sizeof(int));

   if (lastBarrierNode)
      delete[] lastBarrierNode;
   lastBarrierNode = new Node*[maximumGraphLevel+1];
   memset (lastBarrierNode, 0, (maximumGraphLevel+1)*sizeof(Node*));
   
   if (nextBarrierNode)
      delete[] nextBarrierNode;
   nextBarrierNode = new Node*[maximumGraphLevel+1];
   memset (nextBarrierNode, 0, (maximumGraphLevel+1)*sizeof(Node*));
   
   if (!barrierNodes.empty())
   {
      NodeList::iterator nlit = barrierNodes.begin();
      for ( ; nlit!=barrierNodes.end() ; ++nlit )
      {
         Node *nn = *nlit;
         nextBarrierLevel[nn->_level] = nn->_level;
         lastBarrierNode[nn->_level] = nn;
         nextBarrierNode[nn->_level] = nn;
      }
      int maxBarrierLevel = 0, lastBarrierLevel = 0;
      Node *lastBarrier = NULL, *nextBarrier = NULL;
      int j = maximumGraphLevel;
      for ( ; j>0 ; --j )
         if (nextBarrierLevel[j])
         {
            maxBarrierLevel = j;
            lastBarrierLevel = j;
            assert (lastBarrierNode[j] != NULL);
            assert (nextBarrierNode[j] != NULL);
            lastBarrier = lastBarrierNode[j];
            nextBarrier = nextBarrierNode[j];
            break;
         }
      assert (j>0);
      for ( j=j-1 ; j>0 ; --j )
      {
         if (nextBarrierLevel[j])
            lastBarrierLevel = j;
         else
            nextBarrierLevel[j] = lastBarrierLevel;
         if (nextBarrierNode[j]==NULL)
            nextBarrierNode[j] = nextBarrier;
         else
            nextBarrier = nextBarrierNode[j];
      }
      for ( j=maxBarrierLevel+1 ; j<=maximumGraphLevel ; ++j )
      {
         nextBarrierLevel[j] = lastBarrierLevel;
         lastBarrierNode[j] = lastBarrier;
         nextBarrierNode[j] = nextBarrier;
      }
      for (j=1 ; j<=maxBarrierLevel ; ++j)
         if (lastBarrierNode[j]==NULL)
            lastBarrierNode[j] = lastBarrier;
         else
            lastBarrier = lastBarrierNode[j];
   }
   
   removeCfgFlags(DG_GRAPH_IS_MODIFIED);
}
//--------------------------------------------------------------------------------------------------------------------

void
SchedDG::recursiveNodeLevels(Node* node, int lev, unsigned int m)
{
   // if this is first time I see this node, assign a level to it
   if (! node->visited(m) )
   {
      node->_level = lev;
      node->visit(m);
//      cerr << "recursiveNodeLevels: first time visiting " << *node
//           << ", giving it level " << lev << endl;
   } else   // otherwise test if this is a deeper level
   {
      // if we find that the node is on a deeper level than we thought before
      // then continue recursively because its children might be on deeper 
      // levels too.
      // Otherwise stop recursion at this node, because it cannot change
      // anything downstream.
//      cerr << "recursiveNodeLevels: repeat visiting of " << *node
//           << ", with old level " << node->_level << " and new level "
//           << lev << endl;
      if (node->_level < lev)
         node->_level = lev;
      else
         return;
   }

   // we continue only if lev is the deepest level found for this node,
   // therefore, I must go recursive with lev+1
   int i = 0;
   OutgoingEdgesIterator oeit(node);
   while ((bool)oeit)
   {
      Edge *ee = oeit;
      if ( ee->getLevel()==0 && ! ee->IsRemoved() && 
             ee->isSchedulingEdge() )
      {
         ++ i;
         recursiveNodeLevels (ee->sink(), lev+1, m);
      }
      ++ oeit;
   }
   // maximum graph level can belong only to a leaf node; if I am here, 
   // it means that this node's level is lev
   if (i==0 && lev>maximumGraphLevel)
      maximumGraphLevel = lev;
}
//--------------------------------------------------------------------------------------------------------------------

bool
SchedDG::Edge::is_forward_edge(int entryLevel, int exitLevel)
{
   int srcLevel = source()->getLevel(),
       sinkLevel = sink()->getLevel();
   if (entryLevel < exitLevel)
      return (srcLevel < sinkLevel);
   else
      return (
              ((sinkLevel>entryLevel || srcLevel<exitLevel) && srcLevel<sinkLevel) ||
              (srcLevel>=entryLevel && sinkLevel<=exitLevel && srcLevel>sinkLevel)
             );
}

int
SchedDG::pruneDependencyGraph()
{
   // I have to compute levels for nodes
   if (HasAllFlags (DG_GRAPH_IS_MODIFIED))
      ComputeNodeLevels();
      
   // for each node
   NodesIterator nit(*this);
   int numNodes = HighWaterMarker();
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (5, 
      fprintf(stderr, "** Num nodes = %d\n", numNodes);
      fflush (stderr);
   )
#endif
   Edge** inEdges = new Edge*[numNodes];
   int res = 0;
   while((bool)nit)
   {
      memset(inEdges, 0, numNodes*sizeof(Edge*) );
      Node *nn = nit;
      // find the level of the farthest child
      int crtLevel = nn->_level;
      int lastChild = -1;
      int maxLatency = -1;
      bool is_load = nn->isInstructionNode() && nn->is_load_instruction();

      unsigned int childMark = new_marker();
      unsigned int visitedMark = new_marker();
      int numChild = 0;  // number of distinct children nodes that are at
                         // least two levels below current node
      int allChild = 0;  // number of distinct children nodes
      OutgoingEdgesIterator oem(nn);   // iterate all children
      while((bool)oem)
      {
         if (! oem->IsRemoved() && oem->getDistance()>=0 && 
                   oem->isSchedulingEdge())
         {
            Node* node = oem->sink();
            // test for self cycles here; we remove them if it is not a memory
            // dependency
            // mgabi 08/16/2005: I will not remove register self cycles anymore. 
            // Think about reduction sums. Eventually I could test how many
            // input registers the node has, and keep one node cycles with more 
            // than one input registers.
            // I test for multiple input registers, but found a case were
            // there are 2 input registers, but the cycle was due to incrementing
            // the index variable (or an address register), where the stride
            // was received as a parameter. I should test that both registers
            // are defined inside the loop. For this I should add some flag to
            // each register to tell if it was defined inside the loop or not.
            // TODO - FIXME
            if (node==nn && oem->getType()!=MEMORY_TYPE &&
                    !node->is_load_instruction())
            {
               // compute how many input edges of register type it has
               int input_reg_edges = 0;
               IncomingEdgesIterator ireit(nn);
               while ((bool)ireit)
               {
                  Edge *inEdge = ireit;
                  if (inEdge->getType()==GP_REGISTER_TYPE ||
                      inEdge->getType()==ADDR_REGISTER_TYPE)
                     ++ input_reg_edges;
                  ++ ireit;
               }
               if (node->srcRegs.size()<2 || input_reg_edges<2)
               {
                  // we found a self-cycle edge that can be removed
                  res += 1;
                  oem->MarkRemoved();
                  setCfgFlags(DG_GRAPH_IS_MODIFIED);
                  ++ oem;
                  continue;
               }
            }
            
            ObjId nodeId = node->getId();
            int nodeLev = node->_level;
            // I cannot remove edges that go to nodes on the immediate next 
            // level, therefore, to reduce time spent in this phase, I should
            // make sure they are not considered among the number of edges
            // that must be removed. I shouldn't even mark those nodes as
            // children because I just waste time in the next step checking
            // them. The problem is with the node on the last level. Next 
            // level is the first level. Must know which is the last level.
            if (inEdges[nodeId]==NULL)
            {
               ++ allChild;
               if (nodeLev>(crtLevel+1) || (nodeLev<=crtLevel && 
                      nodeLev+maximumGraphLevel-crtLevel > 1))
               {
                  node->visit(childMark);
                  ++ numChild;
               }
            }
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (0, 
               if (nodeId>=numNodes)
                  fprintf(stderr, "!*! PathId %s: Found nodeId=%d, >= than numNodes=%d\n",
                       pathId.Name(), nodeId, numNodes);
            )
#endif
            // test if this node has multiple dependencies to the same node
            // it can happen for a node to have a memory anti-dependency and 
            // a register true-dependency at the same time
            if (HasAllFlags(DG_MACHINE_DEFINED))  // has the edge latencies
            {
               // compute the maximum latency of all children
               if (maxLatency < (int)(oem->getLatency()))
                  maxLatency = (int)(oem->getLatency());
               if (inEdges[nodeId] != NULL)  // I've set a direct edge before
               {
                  // test which edge is less restrictive
                  Edge* dirE = inEdges[nodeId];
                  int dirIters = dirE->getDistance();
                  // do not remove edges with distance less than 0. These are special
                  // dependencies on one iteration only
                  // numIters must be >=0 because I do not traverse negative edges
   
                  // check that the direct edge cannot be more restrictive than the 
                  // chain of edges. Test both the number of iterations and the latency
                  // Also, do not remove register dependencies that start at 
                  // a load node, unless the more restrictive path starts also
                  // with the same type of dependency.
                  // Why did I decide not to remove register dependencies that
                  // start at a load. Oh, perhaps because these are the type of
                  // dependencies that determine stall cycles due to cache misses.
                  // Latency of loads is set to the lowest value (cache hit), 
                  // but in case of a cache miss on that load, I need to have
                  // that dependency to tell me how much can be overlapped 
                  // and how much latency will be exposed.
                  int dirLat = dirE->getLatency();
                  if (oem->getDistance()<=dirIters && oem->getLatency()>=dirLat)
                  {
                     // we found an edge that can be removed
                     if (! dirE->IsRemoved() && (!is_load || 
                           (dirE->getType()!=GP_REGISTER_TYPE && 
                            dirE->getType()!=ADDR_REGISTER_TYPE)) )
                     {
                        res += 1;
                        dirE->MarkRemoved();
                        setCfgFlags(DG_GRAPH_IS_MODIFIED);
                     }
                     inEdges[nodeId] = oem;
                  }
                  else
                     if (oem->getDistance()>=dirIters && oem->getLatency()<=dirLat
                         && (!is_load || 
                           (oem->getType()!=GP_REGISTER_TYPE && 
                            oem->getType()!=ADDR_REGISTER_TYPE)) )
                     {
                        // we can remove the current edge
                        res += 1;
                        oem->MarkRemoved();
                        setCfgFlags(DG_GRAPH_IS_MODIFIED);
                     }
               } else
                  inEdges[nodeId] = oem;
            } else
               inEdges[nodeId] = oem;
            
            if ( (lastChild>crtLevel && (lastChild<nodeLev || nodeLev<=crtLevel)) // forward dep
                 || (lastChild<nodeLev && nodeLev<=crtLevel) // back dep
                 || (lastChild == (-1)) )  // uninitialized
               lastChild = nodeLev;
         }
         ++ oem;
      }
      
      // In the recursive part of the algorithm, I can remove at most
      // numChild edges (edges that span more than one level). If there are 
      // no children on the immediate next level, then I can remove at most
      // numChild-1 edges (one edge must remain to keep me connected).
      if (allChild == numChild)
         numChild -= 1;
      if (numChild>0)
      {
         assert(lastChild > 0);
         // determine how many backedges are allowed
         int backEdges = 0;   // is the last child on a backedge?
         if (lastChild<=crtLevel)
            backEdges = 1;
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (6, 
            cerr << "PruneGraph: going recursive for node " << *nn 
                 << " with numChildren=" << numChild << ", lastChildLevel="
                 << lastChild << ", crtLevel=" << crtLevel 
                 << ", maxLatency=" << maxLatency << endl;
         )
#endif
         res += pruneGraphRecursively(nn, childMark, lastChild, maxLatency,
               visitedMark, /* num edges */ 0, /* num iters crossed */ 0,
               /* latency */ 0, /*barriers*/ 0, backEdges, inEdges, 
               numChild, false);
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (6, 
            cerr << "PruneGraph: total number of removed dependencies is " 
                 << res << endl;
         )
#endif
      }
      ++ nit;
   }
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (2, 
      cerr << "PruneGraph: removed " << res << " dependencies." << endl;
      fflush (stderr);
   )
#endif
   delete[] inEdges;
   setCfgFlags (DG_GRAPH_PRUNED);
   return (res);
}
//--------------------------------------------------------------------------------------------------------------------

int
SchedDG::pruneGraphRecursively(SchedDG::Node* node, unsigned int cMark,
         int lastChild, int maxLatency, unsigned int vMark,
         int numEdges, int numIters, int latency, int barriers, 
         int backEdges, Edge** inEdges, int toRemove, bool loadToReg)
{
   int res = 0;
   // test when we stop from recursion. Use lastChild info and how many 
   // backedges we can traverse
   int nId = node->getId();
   int nLev = node->_level;
   if ((nLev>lastChild && backEdges==0) || backEdges<0)
      return (res);

   if (node->visited(vMark))
      return (res);
   
   if (node->visited(cMark))   // found a direct child
   {
      // determine that we did not reach this node on a direct edge
      if (numEdges > 1)  // not a direct edge
      {
         Edge* dirE = inEdges[nId];
         int dirIters = dirE->getDistance();
         int dirLat = dirE->getLatency();
         // do not remove edges with distance less than 0. These are special
         // dependencies on one iteration only
         // numIters must be >=0 because I do not traverse negative edges

         // if this child crosses a barrier node and we have at least two
         // barrier nodes in this path, then I can remove it.
         // 03/09/2013, gmarin: why have more than 1 barrier nodes to remove 
         // dependencies that cross one? I should remove dependencies that 
         // cross a barrier node, even if only one barrier node in path.
         if (barriers) // && barrierNodes.size() > 1)
         {
            if (! dirE->IsRemoved())
            {
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (6, 
                  cerr << "pruneDependencyGraph: remove edge " << *dirE
                       << " which crosses a barrier node." << endl;
               )
#endif
               res += 1;
               dirE->MarkRemoved();
               setCfgFlags (DG_GRAPH_IS_MODIFIED);
            }
         } else
         // check that the direct edge cannot be more restrictive than the 
         // chain of edges. Test both the number of iterations and the latency
         if (numIters<=dirIters && latency>=dirLat)
         {
            // we found an edge that can be removed
            if (! dirE->IsRemoved() && 
                     (loadToReg || !dirE->source()->is_load_instruction() || 
                        (dirE->getType()!=GP_REGISTER_TYPE && 
                         dirE->getType()!=ADDR_REGISTER_TYPE)) )
            {
               res += 1;
               dirE->MarkRemoved();
               setCfgFlags(DG_GRAPH_IS_MODIFIED);
            }
         }
      }
      
   }
   
   if ((numEdges>3 || (maxLatency>=0 && latency>=maxLatency)) && 
           !node->visited(cMark))
      node->visit (vMark);
   
   // delete only when the barrier node is not the source node
   if (numEdges && node->isBarrierNode ())
      barriers += 1;
   OutgoingEdgesIterator oeit(node);
   while ((bool)oeit)
   {
      if (toRemove <= res)
         break;
      // do not consider control dependencies for chains unless latencies
      // were computed
      int dist = oeit->getDistance();
      unsigned int lat = oeit->getLatency();
      if (dist>=0 && !oeit->IsRemoved() && oeit->isSchedulingEdge() && 
           (oeit->getType()!=CONTROL_TYPE || 
               (HasAllFlags(DG_MACHINE_DEFINED))) )
      {
         Node* sink = oeit->sink();
         bool loadRegDepAllowed = false;
         if (numEdges==0 && node->is_load_instruction() && 
              (oeit->getType()==GP_REGISTER_TYPE || 
               oeit->getType()==ADDR_REGISTER_TYPE) )
            loadRegDepAllowed = true;
         
         // test for self cycles here; we remove them if it is not a memory
         // dependency
         // mgabi 08/16/2005: I will not remove register self cycles anymore. 
         // Think about reduction sums. Eventually I could test how many
         // input registers the node has, and keep one node cycles with more 
         // than one input registers.
         // I test for multiple input registers, but found a case were
         // there are 2 input registers, but the cycle was due to incrementing
         // the index variable (or an address register), where the stride
         // was received as a parameter. I should test that both registers
         // are defined inside the loop. For this I should add some flag to
         // each register to tell if it was defined inside the loop or not.
         // TODO - FIXME
         if (node==sink && oeit->getType()!=MEMORY_TYPE &&
                 !node->is_load_instruction() && 
                  node->srcRegs.size()<2 )
         {
            // we found a self-cycle edge that can be removed
            if (! oeit->IsRemoved() )
            {
               res += 1;
               oeit->MarkRemoved();
               setCfgFlags(DG_GRAPH_IS_MODIFIED);
            }
         }

         int back = 0;
         if (sink->_level <= nLev)  // this is a back edge
            back = 1;
         res +=
            pruneGraphRecursively(sink, 
                    cMark, lastChild, maxLatency, vMark, numEdges+1, 
                    numIters+dist, latency+lat, barriers, backEdges-back, 
                    inEdges, toRemove-res, (loadToReg || loadRegDepAllowed));
      }
      ++ oeit;
   }
   return (res);
}
//--------------------------------------------------------------------------------------------------------------------

// gmarin, 2013/09/03:
// I must also prune memory dependencies that cross a barrier node.
// I made a change in the pruneGraphRecursively routine that hopefully 
// takes care of this.
int
SchedDG::pruneMemoryDependencies (NodeArray *memNodes)
{
   // for each node
   NodeArray::iterator nit = memNodes->begin ();
   int numNodes = HighWaterMarker();

   Edge** inEdges = new Edge*[numNodes];
   int res = 0;

   for ( ; nit!=memNodes->end() ; ++nit)
   {
      memset(inEdges, 0, numNodes*sizeof(Edge*) );
      Node *nn = *nit;
      // find the level of the farthest child (node id is the level)
      int crtLevel = nn->getId ();
      int lastChild = -1;

      unsigned int childMark = new_marker();
      unsigned int visitedMark = new_marker();
      int numChild = 0;  // number of distinct children nodes that are at
                         // least two ids below current node
      int allChild = 0;  // number of distinct children nodes
      OutgoingEdgesIterator oem(nn);   // iterate all children
      while((bool)oem)
      {
         if (! oem->IsRemoved() && oem->getDistance()>=0 && 
                   oem->getType() == MEMORY_TYPE)
         {
            Node* node = oem->sink();
            
            ObjId nodeId = node->getId();
            int nodeLev = nodeId;
            // I cannot remove edges that go to the immediate next node,
            // therefore, to reduce time spent in this phase, I should
            // make sure they are not considered among the number of edges
            // that must be removed. I shouldn't even mark those nodes as
            // children because I just waste time in the next step checking
            // them. The problem is with the last node. Next node
            // is the first node. Must know which is the last node.
            if (inEdges[nodeId]==NULL)
            {
               ++ allChild;
               if (nodeLev>(crtLevel+1) || (nodeLev<=crtLevel && 
                      nodeLev+numNodes-crtLevel > 2))
               {
                  node->visit (childMark);
                  ++ numChild;
               }
            }
#if DEBUG_GRAPH_CONSTRUCTION
            DEBUG_GRAPH (0, 
               if (nodeId>=numNodes)
                  fprintf(stderr, "!*! PathId %s: Found nodeId=%d, >= than numNodes=%d\n",
                       pathId.Name(), nodeId, numNodes);
            )
#endif

            if (inEdges[nodeId] != NULL)  // I've set a direct edge before
            {
               // test which edge is less restrictive
               Edge* dirE = inEdges[nodeId];
               int dirIters = dirE->getDistance();
               // do not remove edges with distance less than 0. These are 
               // special dependencies on one iteration only.
               // numIters must be >=0 because I do not traverse negative edges
   
               // check that the direct edge cannot be more restrictive than 
               // the chain of edges. Test the number of iterations.
               if (oem->getDistance()<dirIters)
               {
                  // we found an edge that can be removed
                  if (! dirE->IsRemoved())
                  {
                     res += 1;
                     dirE->MarkRemoved();
//                     setCfgFlags(DG_GRAPH_IS_MODIFIED);
                  }
                  inEdges[nodeId] = oem;
               }
               else
               {
                  // we can remove the current edge
                  res += 1;
                  oem->MarkRemoved();
//                  setCfgFlags(DG_GRAPH_IS_MODIFIED);
               }
            } else
               inEdges[nodeId] = oem;
            
            if ( (lastChild>crtLevel && (lastChild<nodeLev || nodeLev<=crtLevel)) // forward dep
                 || (lastChild<nodeLev && nodeLev<=crtLevel) // back dep
                 || (lastChild == (-1)) )  // uninitialized
               lastChild = nodeLev;
         }
         ++ oem;
      }
      
      // In the recursive part of the algorithm, I can remove at most
      // numChild edges (edges that span more than one level). If there are 
      // no children on the immediate next level, then I can remove at most
      // numChild-1 edges (one edge must remain to keep me connected).
      if (allChild == numChild)
         numChild -= 1;
      if (numChild>0)
      {
         assert(lastChild > 0);
         // determine how many backedges are allowed
         int backEdges = 0;   // is the last child on a backedge?
         if (lastChild<=crtLevel)
            backEdges = 1;
#if DEBUG_GRAPH_CONSTRUCTION
         DEBUG_GRAPH (3,
            cerr << "PruneMemoryDependencies: going recursive for node " << *nn 
                 << " with numChildren=" << numChild << ", lastChildLevel="
                 << lastChild << ", crtLevel=" << crtLevel << endl;
         )
#endif
         res += pruneMemoryRecursively (nn, childMark, lastChild,
               visitedMark, /* num edges */ 0, /* num iters crossed */ 0,
               backEdges, inEdges, numChild);
#if DEBUG_GRAPH_CONSTRUCTION
         DEBUG_GRAPH (3, 
            cerr << "PruneMemoryDependencies: total number of removed dependencies is " 
                 << res << endl;
         )
#endif
      }
   }

   delete[] inEdges;
   return (res);
}
//--------------------------------------------------------------------------------------------------------------------

int
SchedDG::pruneMemoryRecursively (SchedDG::Node* node, unsigned int cMark,
         int lastChild, unsigned int vMark, int numEdges, int numIters, 
         int backEdges, Edge** inEdges, int toRemove)
{
   int res = 0;
   // test when we stop from recursion. Use lastChild info and how many 
   // backedges we can traverse
   int nId = node->getId();
   int nLev = nId;
   if ((nLev>lastChild && backEdges==0) || backEdges<0)
      return (res);

   if (node->visited (vMark))
      return (res);
   
   bool edgeWasRemoved = false;
   if (node->visited (cMark))   // found a direct child
   {
      // determine that we did not reach this node on a direct edge
      if (numEdges > 1)  // not a direct edge
      {
         Edge* dirE = inEdges[nId];
         int dirIters = dirE->getDistance();
         // do not remove edges with distance less than 0. These are special
         // dependencies on one iteration only
         // numIters must be >=0 because I do not traverse negative edges

         // if this child crosses a barrier node and we have at least two
         // barrier nodes in this path, then I can remove it.
         // I cannot really determine when I cross a barrier node, because
         // I follow only memory deps, so I will not visit barrier nodes.
         // Remove all code related to barriers.

         // check that the direct edge cannot be more restrictive than the 
         // chain of edges. Test the number of iterations.
         if (numIters<=dirIters)
         {
            // we found an edge that can be removed
            if (! dirE->IsRemoved())
            {
               res += 1;
               edgeWasRemoved = true;
               dirE->MarkRemoved();
//               setCfgFlags(DG_GRAPH_IS_MODIFIED);
            }
         }
      }
   }
   
   if (!node->visited (cMark) || edgeWasRemoved)
      node->visit (vMark);
   
   OutgoingEdgesIterator oeit(node);
   while ((bool)oeit)
   {
      if (toRemove <= res)
         break;
      // we consider only memory dependencies in this method
      int dist = oeit->getDistance();
      if (dist>=0 && !oeit->IsRemoved() && oeit->getType()==MEMORY_TYPE)
      {
         Node* sink = oeit->sink();
         
         int back = 0;
         if (sink->getId() <= nLev)  // this is a back edge
            back = 1;
         res +=
            pruneMemoryRecursively (sink, cMark, lastChild, vMark, 
                    numEdges+1, numIters+dist, backEdges-back, 
                    inEdges, toRemove-res);
      }
      ++ oeit;
   }
   return (res);
}
//--------------------------------------------------------------------------------------------------------------------

#if VERBOSE_DEBUG_SCHEDULER
static int numCyclesFound = 0;
#endif

typedef DGDominator <SchedDG::SuccessorNodesIterator, 
                   SchedDG::PredecessorNodesIterator> DGPreDominator;
typedef DGDominator <SchedDG::PredecessorNodesIterator,
                   SchedDG::SuccessorNodesIterator> DGPostDominator;

class NodePair
{
public:
   NodePair (SchedDG::Node *_first, SchedDG::Node *_second) :
        first (_first), second (_second)
   { }
   
   SchedDG::Node *first;
   SchedDG::Node *second;
};

typedef std::list<NodePair> NodePairList;

#define DRAW_SUPER_STRUCTURES  0
#define DRAW_DEBUG_GRAPH       1
static PathID targetPath(0x40b985, 2, 1);

void
SchedDG::computeSuperStructures (CycleMap *all_cycles)
{
   int i;
   Node *startNode = NULL;
   bool incl_struct = false;   // specifies if STRUCTURE_TYPE edges are 
                               // to be considered by iterators
   if (!barrierNodes.empty())
      startNode = barrierNodes.front ();
   else
   {
      if (HasAllFlags (DG_GRAPH_IS_MODIFIED))
      {
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (4, 
            cerr << "Start ComputeNodeLevels" << endl;
         )
#endif
         ComputeNodeLevels();
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (4, 
            cerr << "Finished ComputeNodeLevels" << endl;
         )
#endif
      }
      startNode = cfg_top;
      incl_struct = true;
   }
   DGPreDominator predom (this, startNode, incl_struct, "predom");

   if (barrierNodes.empty())
      startNode = cfg_bottom;
   DGPostDominator postdom (this, startNode, incl_struct, "postdom");

#if DRAW_SUPER_STRUCTURES
 if (!targetPath || targetPath==pathId)
 {
    draw_dominator_tree (&predom, "predom");
    draw_dominator_tree (&postdom, "postdom");
 }
#endif
   
   // traverse all nodes and compute a minimum entry and exit node for them
   // NULL is a special value which means there is no such node.
   int numNodes = HighWaterMarker();
   
   // these arrays are indexed by Node->id
   MIAMI::SNode **minEntry = new MIAMI::SNode* [numNodes];
   MIAMI::SNode **minExit = new MIAMI::SNode* [numNodes];
   
   // I need a flag to know if I processed a node already
   int *processed = new int [numNodes];
   memset (processed, 0, numNodes*sizeof(int));
   memcpy (minEntry, predom.getDomTree(), numNodes*sizeof(MIAMI::SNode*));
   memcpy (minExit, postdom.getDomTree(), numNodes*sizeof(MIAMI::SNode*));
   
   // I need a disjoint set representation to mark the nodes that must be
   // part of the same structure
   DisjointSet *preDS = new DisjointSet (numNodes);
   DisjointSet *postDS = new DisjointSet (numNodes);

   int numPreLCAs=0, numPostLCAs=0, preCalls=0, postCalls=0;
   int numInstNodes=0, numRealEdges=0;


   // the main step of the algorithm. For each node:
   // - check all its succesors (has graph edge to them), if they are not
   //   in a relation internal-exit or entry-internal, but not both, then
   //   union them;
   // - check also its minEntry and minExit; if the relationship is both ways
   //   with either one of them, add that node to the union as well;
   // - finally, during the LCA, we union any LCA node that has only one child;
   NodesIterator nit (*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      if (nn->isInstructionNode())
         ++ numInstNodes;
      else
      {
         ++ nit;
         continue;
      }
      int nid = nn->id;
      
//      fprintf (stderr, "Start process for node B%d\n", nn->id);

      MIAMI::SNodeList domset, pdomset;
      bool mustprocess = false;
      int n1 = preDS->Find (nid);
      int n2 = postDS->Find (nid);
      bool first = true;
      while (nn)
      {
         Node *dest = dynamic_cast<Node*>(minEntry [n1]);   // current entry (dominator) node
         if (dest)
         {
            int dID = dest->id;
            int d2 = postDS->Find (dID);
            Node *rr;
            if (d2!=n2 && 
                ((rr=dynamic_cast<Node*>(minExit[d2]))==nn || (rr && postDS->Find(rr->id)==n2)) )
            {
//               fprintf (stderr, "Add entry node B%d to be unioned with node B%d (%d), first=%d\n",
//                       dID, nid, nn->id, first);
               domset.push_back (dest);
               pdomset.push_back (dest);
               mustprocess = true;
            }
         }
         dest = dynamic_cast<Node*>(minExit [n2]);   // current entry (dominator) node
         if (dest)
         {
            int dID = dest->id;
            int d1 = preDS->Find (dID);
            Node *rr;
            if (d1!=n1 && 
                ((rr=dynamic_cast<Node*>(minEntry[d1]))==nn || (rr && preDS->Find(rr->id)==n1)) )
            {
//               fprintf (stderr, "Add exit node B%d to be unioned with node B%d (%d), first=%d\n",
//                       dID, nid, nn->id, first);
               domset.push_back (dest);
               pdomset.push_back (dest);
               mustprocess = true;
            }
         }

         SuccessorNodesIterator snit (nn, incl_struct);
         while ((bool)snit)
         {
            if (first)
               ++ numRealEdges;
            dest = snit;   // the destination node
            // this must be an instruction node;
            int dID = dest->id;
            int d1 = preDS->Find (dID);
            if (d1!=n1 && minEntry[d1]!=nn && minExit[n2]!=dest)
//              (minEntry[d1]==nn && minExit[n2]==dest)))
            {
//               fprintf (stderr, "Add successor node B%d to be unioned with node B%d (%d), first=%d\n",
//                       dID, nid, nn->id, first);
               domset.push_back (dest);
               pdomset.push_back (dest);
               mustprocess = true;
            }
            ++ snit;
         }
         first = false;
#if 0
         PredecessorNodesIterator pnit (nn, incl_struct);
         while ((bool)pnit)
         {
            Node *src = pnit;   // the source node
            // this must be an instruction node;
            int sID = src->id;
            int s2 = postDS->Find (sID);
            if (s2!=n2 && minExit[s2]!=nn && minEntry[n1]!=src)
            {
               domset.push_back (src);
               pdomset.push_back (src);
               mustprocess = true;
            }
            ++ pnit;
         }
#endif
         if (processed[n1] && !mustprocess)
         {
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (5, 
            {
               cerr << "Node B" << nid << " has been processed before as part of a union. "
                    << "Its minEntry is ";
               if (minEntry[n1])
                  cerr << *dynamic_cast<Node*>(minEntry[n1]);
               else
                  cerr << "ZERO";
               cerr << "; and its minExit is ";
               int op1 = postDS->Find (nid);
               if (minExit[op1])
                  cerr << *dynamic_cast<Node*>(minExit[op1]);
               else
                  cerr << "ZERO";
               cerr << endl;
            }
            )
#endif
            break;
         }
         
         // if one of the bounds is zero, then this node cannot be part of a 
         // structure anyway; do nothing
         if (mustprocess || (minEntry[n1] && minExit[n2]))  
            // find proper entry and exit nodes
         {
            bool domchanged = true, pdomchanged = true;
            while (domchanged || pdomchanged)
            {
               if (domchanged)
               {
                  domchanged = false;
#if VERBOSE_DEBUG_SCHEDULER
                  DEBUG_SCHED (7, 
                     cerr << "Calling predom.findLCA for B" << nid 
                          << ", size(domset)=" << domset.size() << endl;
                  )
#endif
                  numPreLCAs += predom.findLCA (domset, nid, preDS, postDS, 
                                &pdomset, minEntry);
                  preCalls += 1;
                  
                  // check if we have any nodes in pdomset and set pdomchanged
                  if (!pdomset.empty())
                     pdomchanged = true;
                  domset.clear ();
               }
               
               if (pdomchanged)
               {
                  pdomchanged = false;
#if VERBOSE_DEBUG_SCHEDULER
                  DEBUG_SCHED (7, 
                     cerr << "Calling postdom.findLCA for B" << nid 
                          << ", size(pdomset)=" << pdomset.size() << endl;
                  )
#endif
                  numPostLCAs += postdom.findLCA (pdomset, nid, postDS, preDS, 
                                 &domset, minExit);
                  postCalls += 1;
                  
                  // check if we have any nodes in domset and set domchanged
                  if (!domset.empty())
                     domchanged = true;
                  pdomset.clear ();
               }
            }
            // we reached a fix point.
            // check if the node is part of a larger union-find set, and set the
            // correct data for the representant node;
            n1 = preDS->Find (nid);
            n2 = postDS->Find (nid);
         }
         processed[n1] = 1;
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (5, 
         {
            cerr << "Node B" << nid << " has just been processed. "
                 << "Its minEntry is ";
            if (minEntry[n1])
               cerr << *dynamic_cast<Node*>(minEntry[n1]);
            else
               cerr << "ZERO";
            cerr << "; and its minExit is ";
            int on1 = postDS->Find (nid);
            if (minExit[on1])
               cerr << *dynamic_cast<Node*>(minExit[on1]);
            else
               cerr << "ZERO";
            cerr << endl;
         }
         )
         DEBUG_SCHED (7, 
            cerr << "Performance stats: num minEntry reads=" << numPreLCAs
                 << ", preLCA calls=" << preCalls << ", minExit reads="
                 << numPostLCAs << ", postLCA calls=" << postCalls << endl;
         )
#endif
         Node *domN = dynamic_cast<Node*>(minEntry[n1]);
         Node *pdomN = dynamic_cast<Node*>(minExit[n2]);
         nn = 0;
         if (domN && pdomN)
         {
            nid = domN->id;
            int pdomId = pdomN->id;
            n1 = preDS->Find (nid);
            n2 = postDS->Find (nid);
            int p1 = preDS->Find (pdomId);
            if (n1 != p1)   // if they are not Unioned already
            {
               // It doesn't work as well if I just test minEntry and minExit
               // instead of true dominance
               bool entryDomsExit = predom.Dominates (domN, pdomN);
               bool exitPdomsEntry = postdom.Dominates (pdomN, domN);
               if ( (entryDomsExit && exitPdomsEntry) ||
                    (!entryDomsExit && !exitPdomsEntry) )  // must union them
               {
//                  fprintf (stderr, "Union preliminary entry node B%d with preliminary exit node B%d, entryDomsExit=%d, exitPdomsEntry=%d\n",
//                        nid, pdomId, entryDomsExit, exitPdomsEntry);
                  domset.push_back (pdomN);
                  pdomset.push_back (pdomN);
                  nn = domN;
                  mustprocess = true;
               }
            }
         }
      }
      ++ nit;
   }
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (5, 
      cerr << "Main Step Performance Stats: num minEntry reads=" << numPreLCAs
           << ", preLCA calls=" << preCalls << ", minExit reads="
           << numPostLCAs << ", postLCA calls=" << postCalls << endl;
   )
#endif

   // print the number of nodes and edges, to use in plotting the algorithm
   // scaling
   if (numSchedEdges == 0)   // if this was not set yet
      numSchedEdges = numRealEdges; // + (numRealEdges>>3);
   
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (1,
      fprintf (stderr, "Path %s, nodes %d, edges %d\n", 
                pathId.Name(), numInstNodes, numRealEdges);
   )
#endif

#if VERBOSE_DEBUG_SCHEDULER
   /* print the minEntry and minExit for all nodes at this point */
   DEBUG_SCHED (8, 
      {
         NodesIterator tnit (*this);
         while ((bool) tnit)
         {
            Node *nn = tnit;
            if (! nn->isInstructionNode()) {
               ++ tnit;
               continue;
            }
            int nid = nn->id;
            int n1 = preDS->Find (nid);
            int n2 = postDS->Find (nid);
            Node *domN = dynamic_cast<Node*>(minEntry[n1]);
            Node *pdomN = dynamic_cast<Node*>(minExit[n2]);
            cerr << "Node " << nn->id << " has minEntry ";
            if (domN) cerr << "node " << domN->id;
            else  cerr << "(NIL)";
            cerr << " and minExit ";
            if (pdomN) cerr << "node " << pdomN->id;
            else  cerr << "(NIL)";
            cerr << endl;
            ++ tnit;
         }
      }
   )
#endif
   // Hoorah! We have all minEntry and minExits computed. Now I have to 
   // traverse them one more time and compute the sets of nodes with equal
   // entry and exit nodes;
   typedef std::map <UIPair, unsigned int, UIPair::OrderPairs> PairUiMap;
   PairUiMap structMap;
   
   listOfNodes = new NodeList* [numInstNodes];
   int *parentIdx = new int [numInstNodes];
   // each structure must contain at least two top nodes to be valid
   // a "top node" is the node that has a direct incoming edge from the
   // structure entry node.
   int *numTopNodes = new int [numInstNodes];
   
   Node **sEntry = new Node* [numInstNodes];
   Node **sExit = new Node* [numInstNodes];
   
   numStructures = 1;
   structMap.insert (PairUiMap::value_type (UIPair (0,0), 0));
   memset (listOfNodes, 0, numInstNodes*sizeof(NodeList*));
   memset (parentIdx, 0, numInstNodes*sizeof(int));
   memset (numTopNodes, 0, numInstNodes*sizeof(int));
   memset (sEntry, 0, numInstNodes*sizeof(Node*));
   memset (sExit, 0, numInstNodes*sizeof(Node*));
   
   // traverse all nodes and build this mapping
   NodesIterator tnit (*this);
   while ((bool)tnit)
   {
      Node *nn = tnit;
      if (! nn->isInstructionNode()) {
         ++ tnit;
         continue;
      }
      int nid = nn->id;
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (7, 
         cerr << "Final Processing Node " << nid << endl;
      )
#endif
      int n1 = preDS->Find (nid);
      int n2 = postDS->Find (nid);
      Node *domN = dynamic_cast<Node*>(minEntry[n1]);
      Node *pdomN = dynamic_cast<Node*>(minExit[n2]);
      bool done = false;
      int crtDepth = 1;
      // add the nodes only if both bounds are non zero
      int lastIdx = -1;
      if (!domN || !pdomN || domN->isCfgEntry() || pdomN->isCfgExit())
         done = true;
      int crtTopNodes = 0;
      while (domN && !domN->isCfgEntry() && pdomN && !pdomN->isCfgExit() && !done)
      {
         int domId=domN->id, pdomId=pdomN->id;
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (7, 
            cerr << "Crt domN is " << domId << " pdomN is " << pdomId << endl;
         )
#endif
         UIPair tempP (domId, pdomId);
         int crtIdx;
         PairUiMap::iterator puit = structMap.find (tempP);
         if (puit == structMap.end ())  // not found
         {
            crtIdx = numStructures ++;
            structMap.insert (PairUiMap::value_type (tempP, crtIdx));
            sEntry[crtIdx] = domN;
            sExit[crtIdx] = pdomN;
         } else  // found an existing structure
         {
            done = true;  // no need to go further finding the parent
            crtIdx = puit->second;
         }
         if (crtDepth == 1)
         {
            // check if this is a top node; I need to look at its first
            // scheduling incoming edge
            nn->structureId = crtIdx;
            PredecessorNodesIterator preit (nn, incl_struct);
            assert ((bool)preit);
            if ((Node*)preit == domN)
               numTopNodes[crtIdx] += 1;
            
            if (listOfNodes[crtIdx] == 0)  // first node in this bin
               listOfNodes[crtIdx] = new NodeList();
            listOfNodes[crtIdx]->push_back (nn);
         } else   // this is the parent node of lastIdx
         {
            assert (lastIdx > 0);
            parentIdx[lastIdx] = crtIdx;
            numTopNodes[crtIdx] += crtTopNodes;
         }

         // Among other things, I need to compute the parent of this structure
         // and a depth for the structure tree. In this tree, leaves have a
         // depth of 1 and the depth grows towards the root.
         // I need to find the relationship between the current entry and 
         // exit nodes. I am interested in their dominator/post-dominator
         // relationship.
         if (!done)
         {
//            bool entryDomsExit = predom.Dominates (domN, pdomN);
//            bool exitPdomsEntry = postdom.Dominates (pdomN, domN);
            int d1 = preDS->Find (domId);
            int d2 = postDS->Find (domId);
            int p1 = preDS->Find (pdomId);
            int p2 = postDS->Find (pdomId);
            bool entryDomsExit = (minEntry[p1]==domN);
            bool exitPdomsEntry = (minExit[d2]==pdomN);
            crtTopNodes = 0;
            if (entryDomsExit && exitPdomsEntry)  
            {  // entry and exit should be part of the same structure
               assert (d1 == p1);
               domN = dynamic_cast<Node*>(minEntry[d1]);
               pdomN = dynamic_cast<Node*>(minExit[p2]);
            } else
            if (entryDomsExit)  // exit does not post-dominates entry
            {
               pdomN = dynamic_cast<Node*>(minExit[p2]);
               Node *tmpN = domN;
               domN = dynamic_cast<Node*>(minEntry[p1]);
               if (tmpN == domN)
                  // domN is unchanged
                  crtTopNodes = 1;
               else
               {
                  // the second part of the assert doesn't quite make sense 
                  // for me anymore. Since the old domN (current tmpN) 
                  // dominates pdomN, but not the other way around, the
                  // minEntry of pdomN should be dominated by tmpN or at most
                  // equal. Because I modified the tests to use minEntry and
                  // minExit instead of Dominate whose cost is harder to
                  // quantify, I think we should not even get to the else.
                  assert (false);
                  assert (!domN || predom.Dominates (domN, tmpN));
                  // should I make sure also that the new pdomN is equal
                  // to the minExit of d2? It should be. Check only if the
                  // existing assertions fail or we get seg fault or incorrect
                  // super-structures. Similarly for the case below.
               }
            } else
            {
               Node *tmpN = pdomN;
               pdomN = dynamic_cast<Node*>(minExit[d2]);
               if (exitPdomsEntry)
               {
                  domN = dynamic_cast<Node*>(minEntry[d1]);
                  if (tmpN != pdomN)   // pdomN is changed
                  {
                     // the new post-dominator should post-dominate the old one
                     // ??? same comment as above. I guess we should not even
                     // get here.
                     assert (false);
                     assert (!pdomN || postdom.Dominates (pdomN, tmpN));
                  }
               } else   // no domination relationship
               {
                  domN = dynamic_cast<Node*>(minEntry[p1]);
               }
            }
         }
         lastIdx = crtIdx;
         ++ crtDepth;
      }
      if (!done)
      {
         // it means the parent is zero
         assert (!domN || !pdomN || domN->isCfgEntry() || pdomN->isCfgExit());
         assert (lastIdx > 0);
         parentIdx[lastIdx] = 0;
      }
      ++ tnit;
   }
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (2, 
      cerr << "There are " << numStructures << " elementary super structures."
           << endl;
   )
#endif

#if DRAW_SUPER_STRUCTURES
   char file_name[64];
   char file_name_ps[64];
   char title_draw[64];

 if (!targetPath || targetPath==pathId)
 {
   sprintf(file_name, "ssn_%s.dot", pathId.Filename());
   sprintf(file_name_ps, "ssn_%s.ps", pathId.Filename());
   sprintf(title_draw, "ssn_%s", pathId.Filename());
   
   ofstream fout1(file_name, ios::out);
   assert(fout1 && fout1.good());

   fout1 << "digraph \"" << title_draw << "\"{\n";
   fout1 << "size=\"7.5,10\";\n";
   fout1 << "center=\"true\";\n";
//   fout1 << "ratio=\"compress\";\n";
   fout1 << "ranksep=.3;\n";
   fout1 << "node[color=black,fontcolor=black,shape=rectangle,font=Helvetica,fontsize=14,height=.3];\n";
   fout1 << "edge[font=Helvetica,fontsize=14,dir=forward,style=\"setlinewidth(2),solid\",color=black,fontcolor=black];\n";
   for (i=0 ; i<numStructures ; ++i)
   {
      fout1 << "S" << i << "[";
      fout1 << "label=\"S" << i << " {" << numTopNodes[i] << "}";
      if (i==0)
         fout1 << " (0)";
      else
         fout1 << " (B" << sEntry[i]->id << ",B"
              << sExit[i]->id << "):";
      if (listOfNodes[i])
      {
         fout1 << "\\n";
         NodeList::iterator lit = listOfNodes[i]->begin ();
         for ( ; lit!=listOfNodes[i]->end() ; ++lit)
            fout1 << " B" << (*lit)->id;
      }
      fout1 << "\"];\n";
      if (i > 0)
      {
         fout1 << "S" << parentIdx[i] << "->S" << i << ";\n";
      }
   }
   fout1 << "}\n";

   fout1.close();
   assert(fout1.good());
 }
#endif

   // I need to traverse structures again and remove the ones that have only
   // one top node. Removal is performed by merging them with their parents.
   // Construct also the list of children for each structure; For this I need
   // two arrays of size numStructures; One holds the index of the first child 
   // of a node, and the second array holds the index of the next sibling
   // 01/13/2012 gmarin: I should dissallow a super-structure whose entry node
   // is the Dependence Graph Entry node and/or exit node is the DG Exit node.
   // These super-structures do not simplify scheduling anyway.
   childIdx = new int [numStructures];
   next_sibling = new int [numStructures];
   memset (childIdx, 0, numStructures*sizeof(int));
   memset (next_sibling, 0, numStructures*sizeof(int));
   
   for (i=1 ; i<numStructures ; ++i)
   {
      int j = i;
      if (numTopNodes[i]>1)
         j = parentIdx[i];
      while (j && numTopNodes[j]<2)
         j = parentIdx[j];
      if (numTopNodes[i]<2 && j!=i)
      {
//         fprintf (stderr, "Path %llx found structure %d (parent %d) with less than two paths. Check it out.\n",
//              pathId, i, j);
         if (listOfNodes[i])
         {
            NodeList::iterator lit = listOfNodes[i]->begin ();
            for ( ; lit!=listOfNodes[i]->end() ; ++lit)
               (*lit)->structureId = j;  // works for j=0 or j!=0
            
            if (j)  // we found a parent which is a proper structure
                    // move all nodes from struct i to struct j
               listOfNodes[j]->splice (listOfNodes[j]->end(), 
                         *(listOfNodes[i]));
            
            // no proper parent; just remove elements in i
            delete (listOfNodes[i]);
            listOfNodes[i] = NULL;
         }
         parentIdx[i] = j;   // kind of a path compression
         // I must still keep some minimal info about struct i, such that its
         // evenual children can be connected to their proper parent
         // however, I am not adding 'i' among the children of 'j';
      }
      if (numTopNodes[i]>1)
      {
         // I must make the proper 'child' and 'sibling' connections
         assert (j != i);
         parentIdx[i] = j;
         // add i in front of the list of children of j
         next_sibling[i] = childIdx[j];
         childIdx[j] = i;
      }
   }
   
#if DRAW_SUPER_STRUCTURES
/*
   char file_name[64];
   char file_name_ps[64];
   char title_draw[64];
*/
 if (!targetPath || targetPath==pathId)
 {
   sprintf(file_name, "ssnf_%s.dot", pathId.Filename());
   sprintf(file_name_ps, "ssnf_%s.ps", pathId.Filename());
   sprintf(title_draw, "ssnf_%s", pathId.Filename());
   
   ofstream fout(file_name, ios::out);
   assert(fout && fout.good());

   fout << "digraph \"" << title_draw << "\"{\n";
   fout << "size=\"7.5,10\";\n";
   fout << "center=\"true\";\n";
//   fout << "ratio=\"compress\";\n";
   fout << "ranksep=.3;\n";
   fout << "node[color=black,fontcolor=black,shape=rectangle,font=Helvetica,fontsize=14,height=.3];\n";
   fout << "edge[font=Helvetica,fontsize=14,dir=forward,style=\"setlinewidth(2),solid\",color=black,fontcolor=black];\n";
   for (i=0 ; i<numStructures ; ++i)
   {
      if (i && numTopNodes[i]<2)
         continue;
      fout << "S" << i << "[";
      fout << "label=\"S" << i << " {" << numTopNodes[i] << "}";
      if (i==0)
         fout << " (0)";
      else
         fout << " (B" << sEntry[i]->id << ",B"
              << sExit[i]->id << "):";
      if (listOfNodes[i])
      {
         fout << "\\n";
         NodeList::iterator lit = listOfNodes[i]->begin ();
         for ( ; lit!=listOfNodes[i]->end() ; ++lit)
            fout << " B" << (*lit)->id;
      }
      fout << "\"];\n";
      if (i > 0)
      {
         fout << "S" << parentIdx[i] << "->S" << i << ";\n";
      }
   }
   fout << "}\n";

   fout.close();
   assert(fout.good());
   
#if DRAW_DEBUG_GRAPH
   // print also the debug graph at this point
   sprintf (file_name, "v-%s", pathId.Filename());
   draw_debug_graphics (file_name); //"x6_T");
#endif
 }
#endif

   delete[] minEntry;
   delete[] minExit;
   delete[] processed;
   delete preDS;
   delete postDS;
   delete[] parentIdx;	
   delete[] numTopNodes;

   // At this point I have all structures well defined. All remaining 
   // structures are completely valid and all nodes are labeled with the
   // correct structure id. Define a recursive function that constructs
   // the super edges and their latencies, and marks the replaced nodes and
   // edges as invisible?? 
   // Once this is done, call the find cycles routine
   recursive_buildSuperStructures_header (0, sEntry, sExit);

   if (HasAllFlags (DG_GRAPH_IS_MODIFIED))
      ComputeNodeLevels();
   
   // deallocate memory at the end
   delete[] sEntry;
   delete[] sExit;
   for (i=numStructures ; i<numInstNodes ; ++i)
      if (listOfNodes[i])
      {
         assert (! "Found nodes associated with a structure id outside range!!");
         delete listOfNodes[i];
      }
}


void
SchedDG::recursive_buildSuperStructures_header (int crtIdx, Node **sEntry, 
             Node **sExit)
{
   while (1)
   {
      try {
         recursive_buildSuperStructures (crtIdx, sEntry, sExit);
         return;
      } catch (RecomputePathsToExit rpte)
      {
         /* nothing, recompute the paths */
         fprintf (stderr, "Catch RecomputePathsToExit exception for pathID %s, crtIdx %d\n",
                pathId.Name(), crtIdx);
      }
   }
}

void
SchedDG::recursive_buildSuperStructures (int crtIdx, Node **sEntry, 
             Node **sExit)
{
   int i, res=0;
   for (i=childIdx[crtIdx] ; i ; i=next_sibling[i])
   {
      // first call it recursively for each child
      recursive_buildSuperStructures_header (i, sEntry, sExit);
      // all inner structures have been processed and nodes and edges replaced
      // by a single super-edge. Nodes and edges are still there, but they
      // are marked with an invisible flag.
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (5, 
         cerr << "recursive_superStructures: build paths for structureId "
              << i << endl;
      )
#endif
      unsigned int mm = new_marker();
      unsigned int mm2 = new_marker();
      int maxPathToExit = 0, maxEdgesToExit = 0, itersToExit = 0;
      int maxPathFromEntry = 0, maxEdgesFromEntry = 0, itersFromEntry = 0;
      bool add_edge = true;
//ozgurS
      int maxPathToExitCPU = 0, maxPathToExitMem = 0;
//ozgurE
//      bool segmE_is_new = false;
      Edge *segmE = sEntry[i]->findOutgoingEdge (sExit[i], OUTPUT_DEPENDENCY,
             SUPER_EDGE_TYPE, ANY_DEPENDENCE_DISTANCE, ANY_DEPENDENCE_LEVEL);
      if (! segmE)
      {
         segmE = new Edge (sEntry[i], sExit[i], OUTPUT_DEPENDENCY, 
                  SUPER_EDGE_TYPE, 0, 0, 0);
         segmE->subEdges = new EdgeSet();
         segmE->structureId = i;
//         segmE_is_new = true;
      } else
      {
         add_edge = false;
         segmE->subEdges->clear ();
      }
      
      EdgeList finishedEdges;
      int entryLevel = (sEntry[i]->isCfgEntry() ? 0 : sEntry[i]->getLevel());
      int exitLevel = (sExit[i]->isCfgExit() ? maximumGraphLevel : sExit[i]->getLevel());
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout << __func__ << " " <<"max path to exit ";
      )
#endif
      OutgoingEdgesIterator oeit (sEntry[i]);
      while ((bool)oeit)
      {
         Edge *ee = oeit;
         Node *nsink = ee->sink();
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout <<(unsigned int*)ee->sink()->getAddress()<<" ";
            )
#endif
         if (!(ee->IsRemoved()) && !(ee->IsInvisible(i)) && 
              (nsink->structureId==i) &&
              (ee->isSchedulingEdge() || ee->isSuperEdge() ||
                  sEntry[i]->isCfgEntry())
            )
         {
            // should all edges that originate from the entry node be forward edges?
            if (! ee->is_forward_edge(entryLevel, exitLevel))
            {
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (2, 
                  cerr << "HEY: Superstructure " << i << " found edge from entry which is "
                       << "Non-Forward edge(" << entryLevel << "," << exitLevel << "), srcLevel=" 
                       << ee->source()->getLevel() << ", sinkLevel=" << ee->sink()->getLevel()
                       << " : " << *ee << endl;
               )
#endif
//               assert(ee->getDistance()==0);
            }
            res = findLongestPathToExit (nsink, mm, mm2, sExit[i], 
                       i, segmE, ee, entryLevel, exitLevel);
//            ee->superE = segmE;
//            ee->MarkInvisible (i);
            if (res >= 0)
            {
/*
 * ozgurS
 * adding new vars and functions
 */

               int memlat = ee->getMemLatency();
               int cpulat = ee->getCPULatency();
//ozgurE
               int lat = ee->getLatency();
               int dist = ee->getDistance();
               int edges = 1;
               if (res > 0)
               {
//ozgurS
                  memlat += nsink->pathToExitMem;
                  cpulat += nsink->pathToExitCPU;
//ozgurE
                  lat += nsink->pathToExit;
                  dist += nsink->distToExit;
                  edges += nsink->edgesToExit;
               }
               // if this is not the first path I see, check if all paths 
               // to the structure's exit node have the same distance
               if (maxEdgesToExit && itersToExit!=dist) {
#if VERBOSE_DEBUG_SCHEDULER
                  DEBUG_SCHED (2, 
                     cerr << "recursive_buildSuperStructures: " << i 
                          << " found entry node " << *sEntry[i]
                          << " part of a super-structure, with paths of different"
                          << " distance to the exit node. Old distance=" 
                          << itersToExit << ", and new distance=" << dist
                          << " produced by outgoing edge " << *ee 
                          << ". Size(finishedEdges)=" << finishedEdges.size() << endl;
                  )
                  DEBUG_SCHED (4, 
                     EdgeList::iterator elit = finishedEdges.begin();
                     cerr << "=== List of previously finished edges ===" << endl;
                     for ( ; elit!=finishedEdges.end() ; ++elit)
                        cerr << " --- " << *(*elit) << endl;
                  )
#endif
                  correctEdgeDistances (ee, dist, finishedEdges, itersToExit,
                            mm, sExit[i], i);
                  assert (itersToExit == dist);
               }
               if (maxEdgesToExit==0)
               {
                  itersToExit = dist;
//ozgurS
                  maxPathToExitCPU = cpulat;
                  maxPathToExitMem = memlat;
//ozgurE
                  maxPathToExit = lat;
                  maxEdgesToExit = edges;
               } else if (maxPathToExit<lat)
               {
//ozgurS
                 maxPathToExitCPU = cpulat;
                 maxPathToExitMem = memlat;
//ozgurE
                 maxPathToExit = lat;
                 maxEdgesToExit = edges;
               }
               finishedEdges.push_back (ee);
            }
         }
         ++ oeit;
      }
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout  <<std::endl;
            )
#endif
   
      mm = new_marker();
      mm2 = new_marker();
      IncomingEdgesIterator ieit (sExit[i]);
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout << __func__ << " " <<"pathFromEntry: ";
            )
#endif
      while ((bool)ieit)
      {
         Edge *ee = ieit;
         Node *nsrc = ee->source();
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout <<(unsigned int*)ee->source()->getAddress()<<" ";
            )
#endif
         if (!(ee->IsRemoved()) && !(ee->IsInvisible(i)) && 
              (nsrc->structureId==i) &&
              (ee->isSchedulingEdge() || ee->isSuperEdge() ||
                  sExit[i]->isCfgExit())
            )
         {
            res = findLongestPathFromEntry (nsrc, mm, mm2, sEntry[i], 
                       i, segmE, ee, entryLevel, exitLevel);
            ee->superE = segmE;
            segmE->subEdges->insert (ee);
            ee->MarkInvisible (i);
            if (res >= 0)
            {
/*
 * ozgurS
 * adding new vars and functions
 */

               int memlat = ee->getMemLatency();
               int cpulat = ee->getCPULatency();
//ozgurE
               int lat = ee->getLatency();
               int dist = ee->getDistance();
               int edges = 1;
               if (res > 0)
               {
//ozgurS
                  cpulat += nsrc->pathFromEntryCPU;
                  memlat += nsrc->pathFromEntryMem;
//ozgurE
                  lat += nsrc->pathFromEntry;
                  dist += nsrc->distFromEntry;
                  edges += nsrc->edgesFromEntry;
               }
               // for edges incoming to the exit node, the longest path length
               // is equal to the longest path to entry.
               ee->longestStructPath = lat;
               if (maxEdgesFromEntry)  // this is not the first path I see
                  // make sure all paths from the structure's entry node have
                  // the same distance
                  if (itersFromEntry != dist) {
                     cerr << "recursive_buildSuperStructures: found exit node " << *sExit[i]
                          << " part of a super-structure, with paths of different"
                          << " distance from the entry node. Old distance=" 
                          << itersFromEntry << ", and new distance=" << dist
                          << " produced by incoming edge " << *ee << endl;
                     assert (itersFromEntry == dist);
                  }
               if (maxEdgesFromEntry==0)
               {
                  itersFromEntry = dist;
                  maxPathFromEntry = lat;
                  maxEdgesFromEntry = edges;
               } else if (maxPathFromEntry<lat)
               {
                 maxPathFromEntry = lat;
                 maxEdgesFromEntry = edges;
               }
            }
         }
         ++ ieit;
      }
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout <<std::endl;
            )
#endif
      
      if (maxPathToExit != maxPathFromEntry)
      {
         fprintf (stderr, "Error: PathID %s, super-structure %d, found maxPathToExit=%d <> maxPathFromEntry=%d\n",
              pathId.Name(), i, maxPathToExit, maxPathFromEntry);
         assert (maxPathToExit == maxPathFromEntry);
      }
      // must update super-edge with the longest path info
      segmE->minLatency = maxPathToExit;
      segmE->realLatency = maxPathToExit;
     /*
 * ozgurS
 * adding new CPU latencies for maxPathtoExit
 */
      segmE->minCPULatency = maxPathToExitCPU;
      segmE->realCPULatency = maxPathToExitCPU;
      segmE->minMemLatency = maxPathToExitMem;
      segmE->realMemLatency = maxPathToExitMem;
     
//ozgurE
      // must update super-edge with correct distance
      assert (itersToExit == itersFromEntry);
      if (itersFromEntry)
      {
         segmE->dlevel = 1;
         segmE->ddistance = itersFromEntry;
      }
      // setting the HAS_BYPASS flag ensures the latency is not recomputed
      // when the source node is scheduled. For regular edges real latency is 
      // computed after the source node is scheduled because an instructions
      // can have several different length templates.
      segmE->_flags |= (LATENCY_COMPUTED_EDGE | HAS_BYPASS_RULE_EDGE);
      if (add_edge)
         add (segmE);
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (3, 
           cerr << "##> Added a super edge for structure " << i << ": " 
                << *segmE << endl;
      )
#endif
   }
}


int
SchedDG::lowerPathDistances (Node* node, unsigned int m, unsigned int m2,
            Node *finalN, int sId, int toLower, bool superEntry)
{
   int res;
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (4, 
      cerr << "LPD::Enter(" << m << "," << m2 << "): node " << *node
           << " has structureId=" << node->structureId << ", _visited="
           << node->_visited << endl;
   )
#endif
   // I should not find cycles while constructing these super-structures
   // paths
   if (node->visited(m))   // found a cycle
   {
      // while it is possible to find cycles inside structures, I shouldn't
      // be able to find one while lowering distances because I already 
      // removed the edge which was closing the cycle.
      assert (!"I cannot be in a cycle in lowerPathDistances");
      return (-1);
   }
   
   // if this is the final node, return 0
   if (node==finalN) 
   {
      assert (toLower==0 || !"Could not eliminate the entire extra distance on this path");
      return (0);
   }
   // I need to go over edges that were traversed while lowering distance
   // on other paths. However, when I go over them again, I need to consider
   // that I lowered already the distance of some edges. I will store in 
   // distToExit the amount lowered on all paths leaving from this node.
   if (node->visited(m2))
   {
      // I cannot have a different amount needed to be lowered when
      // coming from different edges
      assert (node->distToExit == toLower);
      return (0);
   }
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (4, 
      cerr << "* LPD::Before(" << m << "," << m2 << "): node " << *node
           << " has structureId=" << node->structureId << ", sId="
           << sId << ", superE?=" << superEntry << ", toLower=" << toLower
           << ". Final node is " << *finalN << endl;
   )
#endif
   // there should not be any paths out of the structure that do not pass
   // through the exit node (finalN)
   if (node->structureId!=sId && !superEntry)
   {
      cerr << "SchedDG::lowerPathDistances: node " << *node
           << " has structureId=" << node->structureId << " different than "
           << sId << ". Final node is " << *finalN << endl;
      assert (node->structureId==sId || superEntry);
   }
   node->visit(m);
   
   OutgoingEdgesIterator oeit(node);
   while ((bool)oeit)
   {
      Edge *ee = oeit;
      Node *nsink = ee->sink();
      if (!(ee->IsRemoved()) && !(ee->IsInvisible (sId)) && 
              (ee->isSchedulingEdge() || ee->isSuperEdge()) &&
              (!superEntry || ee->sink()->structureId==sId))
      {
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (4, 
            cerr << "-- LPD: Processing edge " << *ee << " with distance "
                 << ee->ddistance << endl;
         )
#endif
         int edgeLower = toLower;
         if (ee->ddistance > 0)
         {
            if (ee->isSuperEdge())
            {
               unsigned int m2 = new_marker ();
               unsigned int m22 = new_marker ();
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (4, 
                  cerr << "** LPD::CallForSuperE(" << m2 << "," << m22 << "): node " 
                       << *node << " with SuperE id=" << ee->structureId
                       << " old sId=" << sId << ". FinalN=" << *nsink << endl;
               )
#endif
               res = lowerPathDistances (node, m2, m22, nsink, 
                         ee->structureId, min(ee->ddistance, edgeLower), true);
               assert (res == 0);
            }
            if (ee->ddistance >= edgeLower)
            {
               ee->ddistance -= edgeLower;
               edgeLower = 0;
            } else
            {
               edgeLower -= ee->ddistance;
               ee->ddistance = 0;
            }
            if (ee->ddistance == 0)
               ee->dlevel = 0;
         }
         if (edgeLower > 0)
         {
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (4, 
               cerr << "** LPD::CallRestPath(" << m << "," << m2 << "): node " 
                    << *nsink << " with sId=" << sId << ", edgeLower=" << edgeLower
                    << ". FinalN=" << *finalN << endl;
            )
#endif
            res = lowerPathDistances (nsink, m, m2, finalN, sId, edgeLower, false);
            assert (res == 0);
         }
      }
      ++ oeit;
   }
   // unmark the node when returning from recurrence
   node->visit(m2);
   node->distToExit = toLower;
   return (0);
}


void
SchedDG::correctEdgeDistances (Edge *ee, int &dist, EdgeList &finishedEdges, 
       int &oldDist, unsigned int m, Node *finalN, int sId)
        throw (RecomputePathsToExit)
{
   bool throwException = false;
   int res;
   unsigned int mm3 = new_marker ();
   if (dist > oldDist)  // the new path
   {
      int toLower = dist - oldDist;
      if (ee->ddistance > 0)
      {
         if (ee->isSuperEdge())
         {
            unsigned int m2 = new_marker ();
            unsigned int m22 = new_marker ();
            res = lowerPathDistances (ee->source(), m2, m22, ee->sink(), 
                      ee->structureId, min(ee->ddistance, toLower), true);
            assert (res == 0);
            throwException = true;
         }
         if (ee->ddistance >= toLower)
         {
            ee->ddistance -= toLower;
            toLower = 0;
         } else
         {
            toLower -= ee->ddistance;
            ee->ddistance = 0;
         }
         if (ee->ddistance == 0)
            ee->dlevel = 0;
      }
      if (toLower > 0)
      {
         res = lowerPathDistances (ee->sink(), m, mm3, finalN, sId, toLower, false);
         assert (res == 0);
         throwException = true;
      }
      dist = oldDist;
   } else
   {
      EdgeList::iterator elit = finishedEdges.begin();
      for ( ; elit!=finishedEdges.end() ; ++elit)
      {
         Edge *lee = (*elit);
         int toLower = oldDist - dist;
         if (lee->ddistance > 0)
         {
            if (lee->isSuperEdge())
            {
               unsigned int m2 = new_marker ();
               unsigned int m22 = new_marker ();
               res = lowerPathDistances (lee->source(), m2, m22, lee->sink(), 
                         lee->structureId, min(lee->ddistance, toLower), true);
               assert (res == 0);
               throwException = true;
            }
            if (lee->ddistance >= toLower)
            {
               lee->ddistance -= toLower;
               toLower = 0;
            } else
            {
               toLower -= lee->ddistance;
               lee->ddistance = 0;
            }
            if (lee->ddistance == 0)
               lee->dlevel = 0;
         }
         if (toLower > 0)
         {
            res = lowerPathDistances (lee->sink(), m, mm3, finalN, sId, toLower, false);
            assert (res == 0);
            throwException = true;
         }
      }
      oldDist = dist;
   }
   if (throwException)
      throw (RecomputePathsToExit ());
}

int
SchedDG::findLongestPathToExit (Node* node, unsigned int m, unsigned int m2,
              Node *finalN, int sId, Edge *segmE, Edge *lastE,
              int entryLevel, int exitLevel)
{
   // I should not find cycles while constructing these super-structures
   // paths
   if (node->visited(m))   // found a cycle
   {
      // I actually think it is possible to find some cycles inside structures.
      // However, these tend to be false cycles due to spill/unspill onto
      // the stack most of the time. Even if we have some actual instructions
      // the cycle should not be limiting since its loop independent section
      // through the structure has length less than the longest path through
      // the structures. The problem would be only if the path does not
      // have a cycle otherwise and we could software pipeline the segment
      // which would be limited by the cycle otherwise.
      // Still, I will take the decision to remove the loop carried 
      // dependency in any cycle I find. Or even better, I will remove the
      // last edge that lead me to close the cycle.
      //
      // gmarin, 10/31/2013: Found another problematic instance of this issue. 
      // I think the way to handle back-edges inside super-structures, is to 
      // move them outside the super-structure. 
      // Internal nodes of super-structures are not supposed to have edges 
      // going outside the super-structure. These back-edges do not
      // go outside the structure technically, but logically they do.
      // I must find the longest paths to the exit / from entry, using only 
      // forward edges. Forward is different from loop independent in the 
      // original ordering of the instructions. Then, find back-edges, and 
      // compute their restriction on the instruction schedule. I replace
      // all internal back-edges with a back-edge from the exit node to the
      // entry node of the structure. I must set the length of this back-edge
      // so that it enforces the strictest of the original constraints. 
      // It is possible that the length may end up being negative. I'll have to
      // add support for negative latency if it raises any asserts.
      // This is a big change, but it should handle these problematic cases 
      // for good.
      // gmarin, 11/06/2013: Current solution: I added a method to check if an
      // edge is forward, that is, if it makes progress from the structure
      // entry to its exit. I am using this method as a filter inside super-
      // structures. I only follow forward edges, and I am removing any non-
      // forward edges, which create cycles. This works much better than 
      // removing the edge that was closing the cycle in a DFS traversal, 
      // because that approach was removing forward edges, and would leave
      // back edges in the structure. This solution fixes the issue I had with
      // the MPAS-Ocean code, and all the removed back edges have had 1 cycle
      // latency.
      
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (0, 
      {
         fprintf (stderr, "Error: SchedDG::findLongestPathToExit: found a cycle.\n");
         fprintf (stderr, "Warning: I will remove the last edge that closed the cycle.\n");
         cerr << " ++ SchedDG::findLongestPathToExit (" << m << "," << m2 
              << "), sId=" << sId << " removing edge " << *lastE << endl;
      }
      )
#endif
      lastE->MarkRemoved ();
      setCfgFlags(DG_GRAPH_IS_MODIFIED);
      return (-1);
      assert (! node->visited (m));
   }
   
   // if this is the final node, return 0
   if (node==finalN) 
   {
      return (0);
   }
   if (node->visited(m2))
      return (1);
   
   // there should not be any paths out of the structure that do not pass
   // through the exit node (finalN)
   assert (node->structureId == sId);
   node->visit(m);
   int pathLatency = 0, numEdges = 0, numIters = 0;
//ozgurS
   int pathLatencyMem = 0, pathLatencyCPU = 0;
//ozgurE
   EdgeList finishedEdges;  // keep a list of edges already traversed
                            // at this level, in case we need to lower dist
   
   OutgoingEdgesIterator oeit(node);
   while ((bool)oeit)
   {
      Edge *ee = oeit;
      Node *nsink = ee->sink();
      if (!(ee->IsRemoved()) && !(ee->IsInvisible (sId)) && 
              (ee->isSchedulingEdge() || ee->isSuperEdge() ||
                  (nsink==finalN && nsink->isCfgExit()))
         )
      {
         if (! ee->is_forward_edge(entryLevel, exitLevel))
         {
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (4, 
               cerr << " ++ SchedDG::findLongestPathToExit (" << dec << m << "," << m2 
                    << "), sId=" << sId << " removing Non-Forward edge " << *ee << endl;
            )
#endif
            ee->MarkRemoved();
            setCfgFlags(DG_GRAPH_IS_MODIFIED);
         } else
         {
            int res = findLongestPathToExit (nsink, m, m2, finalN, sId, segmE, 
                     ee, entryLevel, exitLevel);
   //         ee->superE = segmE;
   //         ee->MarkInvisible (sId);
            if (res >= 0)
            {
//ozgurS
               int cpulat = ee->getCPULatency();
               int memlat = ee->getMemLatency();
//ozgurE
               int lat = ee->getLatency();
               int dist = ee->getDistance();
               int edges = 1;
               if (res > 0)
               {
//ozgurS
                  cpulat += nsink->pathToExitCPU;
                  memlat += nsink->pathToExitMem;
//ozgurE
                  lat += nsink->pathToExit;
                  dist += nsink->distToExit;
                  edges += nsink->edgesToExit;
               }
               // if this is not the first path I see, check that all paths
               // to the structure's exit node have the same distance.
               // I found that in practice it can happen to have paths
               // with different distances. The natural thing to do is to 
               // do is to use the smallest distance and lower the distance
               // of edges that give me a higher distance on other paths.
               // How do I do this efficiently?
               if (numEdges && numIters!=dist) {
#if VERBOSE_DEBUG_SCHEDULER
                  DEBUG_SCHED (2, 
                     cerr << "findLongestPathToExit: found node " << *node
                          << " part of a super-structure (" << sId 
                          << "), with paths of different"
                          << " distances to the exit node. Old distance=" 
                          << numIters << ", and new distance=" << dist
                          << " produced by outgoing edge " << *ee 
                          << " Throw exception and restart path recovery." << endl;
                  )
                  DEBUG_SCHED (4, 
                     EdgeList::iterator elit = finishedEdges.begin();
                     cerr << ">>> List of previously finished edges <<<" << endl;
                     for ( ; elit!=finishedEdges.end() ; ++elit)
                        cerr << " --- " << *(*elit) << endl;
                  )
#endif
                  // Find if the longest distance is on the old paths 
                  // (must be on all of them), or on the new path. Then call a 
                  // routine which lowers the distance of the first edges with a
                  // distance > 0 encountered on those paths. 
                  // Next, I have to restart computing the paths for this 
                  // super-structure because the edges with lowered distance 
                  // might have been encountered on paths from other nodes as 
                  // well. I will implement this behavior using exceptions.
                  correctEdgeDistances (ee, dist, finishedEdges, numIters,
                               m, finalN, sId);
                  assert (numIters == dist);
               }
               if (numEdges==0)
               {
                  numIters = dist;
//ozgurS
                  pathLatencyCPU = cpulat;
                  pathLatencyMem = memlat;
//ozgurE
                  pathLatency = lat;
                  numEdges = edges;
               } else if (pathLatency<lat)
               {
//ozgurS
                 pathLatencyCPU = cpulat;
                 pathLatencyMem = memlat;
//ozgurE
                 pathLatency = lat;
                  numEdges = edges;
               } else if (pathLatency==lat && numEdges<edges)
               {
                  numEdges = edges;
               }
               finishedEdges.push_back (ee);
            }
         }
      }
      ++ oeit;
   }
   // unmark the node when returning from recurrence
   node->visit(m2);
//ozgurS
   node->pathToExitCPU = pathLatencyCPU;
   node->pathToExitMem = pathLatencyMem;
//ozgurE
   node->pathToExit = pathLatency;
   node->distToExit = numIters;
   node->edgesToExit = numEdges;
   return (1);
}


int
SchedDG::findLongestPathFromEntry (Node* node, unsigned int m, unsigned int m2,
              Node *finalN, int sId, Edge *segmE, Edge *lastE,
              int entryLevel, int exitLevel)
{
   // I should not find cycles while constructing these super-structures
   // paths
   if (node->visited(m))   // found a cycle
   {
      // I actually think it is possible to find some cycles inside structures.
      // However, these tend to be false cycles due to spill/unspill onto
      // the stack most of the time. Even if we have some actual instructions
      // the cycle should not be limiting since its loop independent section
      // through the structure has length less than the longest path through
      // the structures. The problem would be only if the path does not
      // have a cycle otherwise and we could software pipeline the segment
      // which would be limited by the cycle otherwise.
      // Still, I will take the decision to remove the loop carried 
      // dependency in any cycle I find. Or even better, I will remove the
      // last edge that lead me to close the cycle.
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (0, 
      {
         fprintf (stderr, "Error: SchedDG::findLongestPathFromEntry: found a cycle.\n");
         fprintf (stderr, "Warning: I will remove the last edge that closed the cycle.\n");
      }
      )
#endif
      lastE->MarkRemoved ();
      setCfgFlags(DG_GRAPH_IS_MODIFIED);
      return (-1);
      assert (! node->visited (m));
   }
   
   // if this is the final node, return 0
   if (node==finalN) 
   {
      return (0);
   }
   if (node->visited(m2))
      return (1);
   
   // there should not be any paths out of the structure that do not pass
   // through the entry node (finalN)
   assert (node->structureId == sId);
   node->visit(m);
   int pathLatency = 0, numEdges = 0, numIters = 0;
   //ozgurS
   int pathLatencyCPU = 0, pathLatencyMem = 0;
   //ozgurE
   IncomingEdgesIterator ieit(node);
   while ((bool)ieit)
   {
      Edge *ee = ieit;
      Node *nsrc = ee->source();
      if (!(ee->IsRemoved()) && !(ee->IsInvisible (sId)) && 
              ee->is_forward_edge(entryLevel, exitLevel) &&
              (ee->isSchedulingEdge() || ee->isSuperEdge() ||
                    (nsrc==finalN && nsrc->isCfgEntry()))
         )
      {
         if (! ee->is_forward_edge(entryLevel, exitLevel))
         {
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (4, 
               cerr << " ++ SchedDG::findLongestPathFromEntry (" << dec << m << "," << m2 
                    << "), sId=" << sId << " removing Non-Forward edge " << *ee << endl;
            )
#endif
            ee->MarkRemoved();
            setCfgFlags(DG_GRAPH_IS_MODIFIED);
         } else
         {
            int res = findLongestPathFromEntry (nsrc, m, m2, finalN, sId, segmE, 
                     ee, entryLevel, exitLevel);
            ee->superE = segmE;
            segmE->subEdges->insert (ee);
            ee->MarkInvisible (sId);
            if (res >= 0)
            {
//ozgurS
               int memlat = ee->getMemLatency();
               int cpulat = ee->getCPULatency();
//ozgurE
               int lat = ee->getLatency();
               int dist = ee->getDistance();
               int edges = 1;
               if (res > 0)
               {
                  memlat += nsrc->pathFromEntryMem;//ozgurS
                  cpulat += nsrc->pathFromEntryCPU;//ozgurE
                  lat += nsrc->pathFromEntry;
                  dist += nsrc->distFromEntry;
                  edges += nsrc->edgesFromEntry;
               }
               // pathToExit should have been already computed and node can
               // be only an internal node of a super-structure.
               ee->longestStructPath = lat + node->pathToExit;
               if (numEdges)  // this is not the first path I see
                  // make sure all paths to the structure's entry node have 
                  // the same distance
                  if (numIters != dist) {
                     cerr << "findLongestPathFromEntry: found node " << *node
                          << " part of a super-structure, with paths of different"
                          << " distance to the exit node. Old distance=" 
                          << numIters << ", and new distance=" << dist
                          << " produced by incoming edge " << *ee << endl;
                     assert (numIters == dist);
                  }
               if (numEdges==0)
               {
                  numIters = dist;
//ozgurS
                  pathLatencyCPU = cpulat;
                  pathLatencyMem = memlat;
//ozgurE
                  pathLatency = lat;
                  numEdges = edges;
               } else if (pathLatency<lat)
               {
//ozgurS
                  pathLatencyCPU = cpulat;
                  pathLatencyMem = memlat;
//ozgurE
                  pathLatency = lat;
                  numEdges = edges;
               } else if (pathLatency==lat && numEdges<edges)
               {
                  numEdges = edges;
               }
            }
         }
      }
      ++ ieit;
   }
   // mark the node as complete when returning from recurrence
   node->visit(m2);
//ozgurS
   node->pathFromEntryCPU = pathLatencyCPU;
   node->pathFromEntryMem = pathLatencyMem;
//ozgurE
   node->pathFromEntry = pathLatency;
   node->distFromEntry = numIters;
   node->edgesFromEntry = numEdges;
   return (1);
}


#if 0
void
SchedDG::recursive_buildSuperStructures (int crtIdx, Node **sEntry, 
             Node **sExit, CycleMap *all_cycles)
{
   int i, res=0;
   for (i=childIdx[crtIdx] ; i ; i=next_sibling[i])
   {
      // first call it recursively for each child
      recursive_buildSuperStructures (i, sEntry, sExit, all_cycles);
      // all inner structures have been processed and nodes and edges replaced
      // by a single super-edge. Nodes and edges are still there, but they
      // are marked with an invisible flag.
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (5, 
         cerr << "recursive_superStructures: build paths for structureId "
              << i << endl;
      )
#endif
      // use a list to find the edges that form a path
      EdgeList edgesStack;
      unsigned int mm = new_marker();
      OutgoingEdgesIterator oeit (sEntry[i]);
      while ((bool)oeit)
      {
         Edge *ee = oeit;
         Node *nsink = ee->sink();
         if (!(ee->IsRemoved()) && !(ee->IsInvisible(i)) && 
              (nsink->structureId==i) &&
              (ee->isSchedulingEdge() || ee->isSuperEdge())
            )
         {
            edgesStack.push_back (ee);
            res += findStructPaths (nsink, mm, edgesStack, sExit[i], i,
                       all_cycles);
            edgesStack.pop_back ();
            ee->MarkInvisible (i);
         }
         ++ oeit;
      }
   }
}

int
SchedDG::findStructPaths (Node* node, unsigned int m, EdgeList& _edges, 
              Node *finalN, int sId, CycleMap *all_cycles)
{
   // I should not find cycles while constructing these super-structures
   // paths
   if (node->visited(m))   // found a cycle
   {
      // I actually think it is possible to find some cycles inside structures.
      // However, these tend to be false cycles due to spill/unspill onto
      // the stack most of the time. Even if we have some actual instructions
      // the cycle should not be limiting since its loop independent section
      // through the structure has length less than the longest path through
      // the structures. The problem would be only if the path does not
      // have a cycle otherwise and we could software pipeline the segment
      // which would be limited by the cycle otherwise.
      // Still, I will take the decision to remove the loop carried 
      // dependency in any cycle I find. Or even better, I will remove the
      // last edge that lead me to close the cycle.
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (0, 
      {
         fprintf (stderr, "Error: SchedDG::findStructPaths: found a cycle. Edges in cycle are:\n");
         EdgeList::iterator elit = _edges.begin();
         for( ; elit!=_edges.end() ; ++elit )
             cerr << " @ " << *(*elit) << endl;
         fprintf (stderr, "Warning: SchedDG::findStructPaths: I will remove the last edge that closed the cycle.\n");
      }
      )
#endif
      assert (!_edges.empty ());
      Edge *lastE = _edges.back ();
      lastE->MarkRemoved ();
      setCfgFlags(DG_GRAPH_IS_MODIFIED);
      return (0);
      assert (! node->visited (m));
   }
   
   // if this is the final node, our path is complete
   if (node==finalN) 
   {
      assert (!_edges.empty());   // found a segment

      Edge* firstE = _edges.front ();
      Node *startNode = firstE->source();

      // for these segments I do not want to sort edges by node id; a cycle 
      // can start from any node, but an open segment can have only well
      // defined end nodes.

#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (8, 
         fprintf (stderr, "findStructPaths: found a new structure path %d ... ", 
                  ++numCyclesFound);
      )
#endif
      
      // use addCycle directly because the paths through a structure must
      // be unique by how we build them
      unsigned int cycId = addCycle(_edges, all_cycles, true);
      
//      unsigned int cycId = addUniqueCycle(_edges, all_cycles, true);
      // create also super edges between entry and exit nodes
      // do not create duplicates, but keep the longest one always
      // should I make sure latencies are computed before? Or just create
      // an edge w/o latency and w/o updating if latency not computed yet.
      // In the end, I assert that latencies are computed before.
      // Also, for each scheduling edge, I update the field superE to hold
      // the shortest/inner-most super edge.
      Edge *sedge = updateSuperEdge (startNode, node, _edges);
      if (sedge->structureId)
         assert (sedge->structureId == sId);
      else
         sedge->structureId = sId;
      if (! sedge->pathsIds)
         sedge->pathsIds = new UISet ();
      sedge->pathsIds->insert (cycId);
      return 1;
   }
   
   // there should not be any paths out of the structure that do not pass
   // through the exit node (finalN)
   assert (node->structureId == sId);
   node->visit(m);
   int result = 0;
   
   OutgoingEdgesIterator oeit(node);
   while ((bool)oeit)
   {
      Edge *ee = oeit;
      Node *nsink = ee->sink();
      if (!(ee->IsRemoved()) && !(ee->IsInvisible (sId)) && 
              (ee->isSchedulingEdge() || ee->isSuperEdge()))
      {
         _edges.push_back (ee);
         result |=
              findStructPaths (nsink, m, _edges, finalN, sId, all_cycles);
         _edges.pop_back();
         ee->MarkInvisible (sId);
      }
      ++ oeit;
   }
   // unmark the node when returning from recurence
   node->visit(m-1);
   //node->MarkInvisible (sId);  // should I mark the nodes as well??
   return (result);
}
#endif
//--------------------------------------------------------------------------------------------------------------------

#if 0
void
SchedDG::updateLongestCycleOfPaths (Edge *supE, unsigned int cycLen)
{
   assert (supE && supE->isSuperEdge() && supE->pathsIds);
   UISet::iterator sit = supE->pathsIds->begin();
   for ( ; sit!=supE->pathsIds->end() ; ++sit)
   {
      Cycle *crtCyc = cycles[*sit];
      assert (crtCyc && crtCyc->isSegment ());
      if (crtCyc->cLength==0)
      {
         // WTH? Print all edges for this path
         cerr << "\nSegment " << *sit << " has length " << crtCyc->getLatency()
              << " WTH??" << endl;
         EdgeList::iterator eit2 = crtCyc->edges->begin ();
         for ( ; eit2!=crtCyc->edges->end() ; ++eit2)
         {
            cerr << "   *** " << *(*eit2) << endl;
         }
      }
      int thisCyc = cycLen - supE->getLatency() + crtCyc->cLength;
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (6, 
         cerr << "UpdateLongestCycleOfPaths for super edge " << *supE
              << " parent cyc len=" << cycLen << ", supE len=" 
              << supE->getLatency() << ", this path len=" << crtCyc->cLength
              << ", resulting length=" << thisCyc << endl;
      )
#endif
      assert (thisCyc>0);
      EdgeList::iterator eit = crtCyc->edges->begin ();
      int idxEdge = 0;
      for ( ; eit!=crtCyc->edges->end() ; ++eit, ++idxEdge)
      {
         if (idxEdge)
         {
            Node *nn = (*eit)->source();
            if (nn->longestCycle < thisCyc)
               nn->longestCycle = thisCyc;
            nn->sumAllCycles += thisCyc;
         }
         if ((*eit)->longestCycle < thisCyc)
         {
            (*eit)->longestCycle = thisCyc;
            if ((*eit)->isSuperEdge ())
               updateLongestCycleOfPaths (*eit, thisCyc);
         }
         (*eit)->sumAllCycles += thisCyc;
      }
   }
}
//--------------------------------------------------------------------------------------------------------------------
#endif

void
SchedDG::updateLongestCycleOfSubEdges (Edge *supE, int cycLen)
{
   assert (supE && supE->isSuperEdge() && supE->subEdges && supE->structureId);
   int structId = supE->structureId;
   Node *exitNode = supE->sink();
   int exitId = ds->Find (exitNode->id);
   EdgeSet::iterator eit = supE->subEdges->begin();
   for ( ; eit!=supE->subEdges->end() ; ++eit)
   {
      Edge *ee = *eit;
      Node *nn = ee->source();

      int crtId = ds->Find (nn->id);
      if (exitId != crtId)
      {
         ds->Union (exitId, crtId);
         exitId = ds->Find (exitId);
      }
      
      // since I made all control dependencies of length 0, it is actually
      // possible to have paths of length 0
#if 0
      if (ee->longestStructPath==0)
      {
         // WTH? Print info for this edge
         cerr << "\nStructureId " << structId << " found edge with longestPath=0 " 
              << " WTH?? " << *ee << endl;
      }
#endif
      int thisCyc = cycLen - supE->getLatency() + ee->longestStructPath;
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (6, 
         cerr << "updateLongestCycleOfSubEdges for edge " << *ee 
              << " with super edge " << *supE
              << " parent cyc len=" << cycLen << ", supE len=" 
              << supE->getLatency() << ", this edge's path len=" << ee->longestStructPath
              << ", resulting length=" << thisCyc << endl;
      )
#endif
      // gmarin, 09/09/2013: I am setting the control latency of branches to 
      // zero, so I may have paths composed of consecutive branches that have
      // zero latency.
//      assert (thisCyc>0);
      if (nn->structureId==structId && nn->longestCycle<thisCyc)
         nn->longestCycle = thisCyc;
      if (ee->longestCycle < thisCyc)
      {
         ee->longestCycle = thisCyc;
         if (ee->isSuperEdge())
            updateLongestCycleOfSubEdges (ee, thisCyc);
      }
   }
}
//--------------------------------------------------------------------------------------------------------------------

void
SchedDG::computeNodeDegrees()
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
      if (!eit->IsRemoved() && eit->isSchedulingEdge())
      {
         eit->source()->fanout += 1;
         eit->sink()->fanin += 1;
      }
      ++ eit;
   }
}
//--------------------------------------------------------------------------------------------------------------------

void
SchedDG::DiscoverSCCs(Node *nn, unsigned int mm, NodeList &nodesStack, int *dfsIndex, int *lowIndex)
{
   int idx = nn->getId();
   if (idx < 0) // do the entry/exit nodes have negative indices? ignore them anyway
      return;
   
   dfsIndex[idx] = lowIndex[idx] = globalIndex++;
   nodesStack.push_back(nn);
   nn->visit(mm);     // mark that it is on the stack
   int selfCycle = 0; // detect if this node has a self-cycle, condition not 
                      // captured by the SCC algorithm
   
   // check all its successor edges
   OutgoingEdgesIterator oeit(nn);
   while ((bool)oeit)
   {
      Edge *ee = oeit;
      if (!ee->IsRemoved() && !ee->IsInvisible() && 
           (ee->isSchedulingEdge() || ee->isSuperEdge())
         )
      {
         Node *sinkN = ee->sink();
         int sidx = sinkN->getId();
         if (sidx>=0)
         {
            if (dfsIndex[sidx] < 0)
            {
               DiscoverSCCs(sinkN, mm, nodesStack, dfsIndex, lowIndex);
               lowIndex[idx] = std::min(lowIndex[sidx], lowIndex[idx]);
            } else if (sinkN->visited(mm))  // sinkN is in stack
            {
               lowIndex[idx] = std::min(dfsIndex[sidx], lowIndex[idx]);
               if (sinkN == nn)  // this is a self loop
                  selfCycle = 1;
            }
         }
      }
      ++ oeit;
   }
   
   // after processing all the successors, check if this node is a SCC head
   if (dfsIndex[idx] == lowIndex[idx])
   {
      Node *w = 0;
      unsigned int leaderId = idx; 
      Ui2NSMap::iterator usit;
      
      assert (! nodesStack.empty());  // at least nn should be on the stack
      int sccSize = 0;
      do
      {
         w = nodesStack.back();
         nodesStack.pop_back();
         w->visit(0);  // node is not in stack anymore
         sccSize += 1;
         if (w!=nn)  // this node is part of an SCC
         {
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (6, 
               cerr << "DiscoverSCCs: Node " << *w << " is part of an SCC with leaderID "
                    << leaderId << endl;
            )
#endif

            // should I build the SCC DAG here?? Seems like a more robust 
            // approach to building SCCs, especially if I use heuristics
            // to not enumerate all the elementary cycles anymore (exponential 
            // number of cycles).
            usit = cycleNodes.find (leaderId);
            if (usit == cycleNodes.end())
            {
               usit = cycleNodes.insert (cycleNodes.begin(),
                      Ui2NSMap::value_type (leaderId, NodeSet()));
               // I do not have any info about cycles yet
               setsLongestCycle.insert (UiUiMap::value_type (leaderId, 0));
            } 
            usit->second.insert(w);
            // set SCC ID for w
            w->setSccId(leaderId);
         }
      } while (!nodesStack.empty() && w!=nn);

      if (sccSize>1 || selfCycle)  // this is the leader of the SCC
      {
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (1, 
            cerr << "DiscoverSCCs: Node " << *nn << " is the head node of an SCC of size "
                 << sccSize << ", selfCycle=" << selfCycle << ". "
                 << "DFS index = " << dfsIndex[idx] << ", low index = " << lowIndex[idx]
                 << endl;
         )
#endif
         // should I build the SCC DAG here?? Seems like a more robust 
         // approach to building SCCs, especially if I use heuristics
         // to not enumerate all the elementary cycles anymore (exponential 
         // number of cycles).
         usit = cycleNodes.find (leaderId);
         if (usit == cycleNodes.end())
         {
            usit = cycleNodes.insert (cycleNodes.begin(),
                   Ui2NSMap::value_type (leaderId, NodeSet()));
            setsLongestCycle.insert (UiUiMap::value_type (leaderId, 0));
         } 
         usit->second.insert(nn);
         // set SCC ID for nn
         nn->setSccId(leaderId);

#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (1, 
            cerr << "DiscoverSCCs: add cycleSet for " << leaderId 
                 << ", numCycleSets=" << numCycleSets+1 << endl;
         )
#endif
         cycleDag->addCycleSet(leaderId);
         ++ numCycleSets;
      } else  // not part of any cycle
      {
         // nodes that are not part of a cycle are going to be
         // in a set with leaderId=0;
         usit = cycleNodes.find(0);
         if (usit == cycleNodes.end())
         {
            usit = cycleNodes.insert (cycleNodes.begin(),
                   Ui2NSMap::value_type (0, NodeSet()));
            setsLongestCycle.insert (UiUiMap::value_type (0, 0));
         }
         usit->second.insert(nn);
      }
   }
}
//--------------------------------------------------------------------------------------------------------------------

void
SchedDG::findDependencyCycles()
{
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout<<__func__<<" here"<<endl;
            )
#endif
   int i;
   if (HasAllFlags (DG_CYCLES_COMPUTED))   // cycles are already computed
      assert(! "SchedDG::findDependencyCycles invoked, but cycles were \
      computed before and not yet removed.");

   assert (cycleDag == NULL);
   // I have to compute top nodes first
   // compute the levels and the top nodes
   if (HasAllFlags (DG_GRAPH_IS_MODIFIED))
   {
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (4, 
         cerr << "Start ComputeNodeLevels" << endl;
      )
#endif
      ComputeNodeLevels();
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (4, 
         cerr << "Finished ComputeNodeLevels" << endl;
      )
#endif
   }
   
   assert(cfg_top!=NULL);
   cycleDag = new CycleSetDAG ();
   if (ds) delete (ds);
   int numNodes = HighWaterMarker();
   ds = new DisjointSet (numNodes);
   
#if VERBOSE_DEBUG_SCHEDULER
   numCyclesFound = 0;
#endif
   CycleMap *all_cycles = new CycleMap ();
   limpModeFindCycle = false;
   
   // find super structures and build their coresponding super-edges
   computeSuperStructures (all_cycles);

   // use a stack to find the edges that form a cycle
   EdgeList edgesStack;
   NodeList nodesStack;
   unsigned int mm = new_marker();
   int *dfsIndex = new int[numNodes];
   int *lowIndex = new int[numNodes];
   for (i=0 ; i<numNodes ; ++i)
   {
      dfsIndex[i] = -1;
      lowIndex[i] = -1;
   }
   globalIndex = 0;
   NodesIterator nsc (*this);
   while ((bool)nsc)
   {
      Node *nn = nsc;
      int idx = nn->getId();
      if (nn->isInstructionNode() && idx>=0 && dfsIndex[idx]<0)
      {
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (7, 
            cerr << "Discover SCCs starting from node " << *nn << endl;
         )
#endif
         DiscoverSCCs(nn, mm, nodesStack, dfsIndex, lowIndex);
         // There should be nothing left on stack after I return from the recursive
         // routine
         assert (nodesStack.empty());
      }
      ++ nsc;
   }
   delete[] lowIndex;
   delete[] dfsIndex;
   
   if (numSchedEdges == 0)  // if it has not been set yet
      numSchedEdges = nextEdgeId;
   
   mm = new_marker();
   unsigned int mm2 = new_marker();
   unsigned int minIdx = 0;
//   unsigned int maxBlack = mm2;

   // if we have any unreordable nodes (these are nodes that constrain any
   // path from a root to a leaf node), then start finding cycle (or in fact
   // paths between unreordable nodes) starting from each such node.
   // A cycle (path segment) is detected when we reach an already visited
   // node (cycle), or when we reach another unreordable node (segment).
   if (barrierNodes.empty())  // no unreordable nodes, possible only in
                              // software pipelined inner loops w/o calls.
   {
      // I have identified all the SCCs.
      // I should traverse all edges in the graph:
      // - if both ends of an edge are part of the same SCC and the edge was not
      //   included in any cycle yet, then I should start from the source of the
      //   edge forcing it to take this edge first.
      // - all edges that are part of SCCs, must have at least one cycle associated
      //   with them,
      
      EdgesIterator eit(*this);
      while ((bool)eit)
      {
         Edge *ee = eit;
         int srcScc = 0, sinkScc = 0;
         if (!ee->IsRemoved() && !ee->IsInvisible() && 
              (ee->isSchedulingEdge() || ee->isSuperEdge())
            )
         {
            srcScc = ee->source()->getSccId();
            sinkScc = ee->sink()->getSccId();
         }
         
         if (srcScc && srcScc==sinkScc && // edge is part of an SCC
              ee->cyclesIds.empty())   // and is not part of any cycle yet
         {
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (5, 
               cerr << " ** Edge " << *ee << " is part of SCC " << srcScc
                    << " but it does not have any cycles. Start a search from this edge."
                    << " Total num cycles = " << nextCycleId << endl;
            )
#endif
            
            // add current edge to edgesStack first and set the visited
            // flags appropriately
            Node *src_nn = ee->source();
            src_nn->visit(mm);
            src_nn->tempVal = 1;
            edgesStack.push_back(ee);
            
            findCycle (ee->sink(), mm, mm2, edgesStack, 
                        all_cycles, 1, minIdx, 0, srcScc);
            src_nn->visit(mm-1);
            edgesStack.pop_back();
            assert (edgesStack.empty());
            if (ee->cyclesIds.empty())
            {
               cerr << "ERROR: Edge " << *ee << " is part of SCC " << srcScc
                    << " but it still does not have any cycles after explicitly starting a search with it."
                    << " Total num cycles = " << nextCycleId
                    << ". Check this out!" << endl;
            }
//            if (result) assert (minIdx > 0);
            mm2 = new_marker();
         }
         ++ eit;
      }
   } else
   {
      NodeList::iterator nlit = barrierNodes.begin();
      for ( ; nlit!=barrierNodes.end() ; ++nlit )
      {
         int sccId = (*nlit)->getSccId();
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (5, 
            cerr << "Start findCycle from barrier node " << *(*nlit) 
                 << ", with SCC Id=" << sccId << endl;
         )
#endif
         findCycle((*nlit), mm, mm2, edgesStack, all_cycles, 0, minIdx, 0, sccId);
      }
      
      // after finding all Super Edges, prune unnecessary super edges.
      // I cannot prune these edges, because then I will have regular 
      // scheduling edges which will have their super edge removed
      // pruneSuperEdges ();
      
      // after pruning Super Edges, find all cycles formed by them.
      // I should always have at least one cycle found, because I add control
      // edges from last barrier nodes to the top nodes, and from the bottom
      // nodes to the first barrier node.
      // Also, I should be able to find all cycles by just starting from a 
      // single barrier node.
      assert (edgesStack.empty ());
   }
   
   numActualCycles = 0.0;
   CycleMap::iterator cmit = all_cycles->begin();
   cycles = new Cycle*[nextCycleId];
   memset(cycles, 0, nextCycleId*sizeof(Cycle*) );
   for ( ; cmit!=all_cycles->end() ; ++cmit )
   {
      Cycle *cyc = cmit->second;
      cycles[cyc->getId()] = cyc;
      if (cyc->isCycle())
      {
         numActualCycles += cyc->numberOfPaths ();
      }
   }
   all_cycles->clear ();
   delete (all_cycles);

#if DRAW_DEBUG_GRAPH
   // print also the debug graph at this point
   char file_name[64];
   if (!targetPath || targetPath==pathId)
   {
      sprintf (file_name, "m-%s", pathId.Filename());
      draw_debug_min_graphics (file_name);
   }
#endif

#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (4, 
      cerr << "Number stored segments/cycles=" << nextCycleId-1
           << ", number different cycles in graph=" << numActualCycles
           << endl;
   )
#endif
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout<<__func__<<" numActualCycles: "<<numActualCycles<<endl;
            )
#endif
   setCfgFlags (DG_CYCLES_COMPUTED);
   
   // After computing the cycles, traverse all edges and mark as loop 
   // independent all edges that are not part of a cycle.
   // I should also fold the graph such that the last barrier node is on
   // the last level.
   Node *lastBN = NULL;
   register int lastBarLevel = 0;
   if (!barrierNodes.empty())
   {
      // should I do this only when I have at least 2 barrier node?
      // For now I will do it even with only one barrier.
      lastBN = barrierNodes.back ();
      lastBarLevel = lastBN->_level;
      assert (lastBarLevel > 0);
   }
   EdgesIterator edit(*this);
   bool modified = false;
   while ((bool)edit)
   {
      Edge *edge = edit;
      if (! edge->IsRemoved() && (edge->isSchedulingEdge() ||
                 edge->isSuperEdge()) )
      {
         edge->hasCycles = 0;

         int srcLevel = edge->source()->_level;
         int destLevel = edge->sink()->_level;
         int adjustment = 0;
         if (lastBarLevel>0)
         {
            if (srcLevel<=lastBarLevel && destLevel>lastBarLevel)
            {
               edge->ddistance += 1;
               edge->dlevel = 1;
               modified = true;
               adjustment = 1;

               if (edge->isSuperEdge())  // it is a super-structure
               {
                  // traverse all edges in this super-structure and
                  // update distance to entry and exit for affected nodes.
                  unsigned int mm = new_marker();
                  assert (edge->subEdges);
#if VERBOSE_DEBUG_SCHEDULER
                  DEBUG_SCHED (6, 
                     cerr << endl << "*** Start updating distances for super-edge (T1) "
                          << *edge << endl;
                  )
#endif
                  EdgeSet::iterator eit = edge->subEdges->begin();
                  for ( ; eit!=edge->subEdges->end() ; ++eit)
                  {
                     Edge *ee = *eit;
                     Node *srcNode = ee->source();
                     if (srcNode!=edge->source() && !srcNode->visited(mm))
                     {
                        srcNode->visit(mm);
#if VERBOSE_DEBUG_SCHEDULER
                        DEBUG_SCHED (6, 
                        {
                           cerr << "** Process edge " << *ee << endl;
                           cerr << "## Before update src node " << *srcNode 
                                << " has distToExit " << srcNode->distToExit
                                << " and distFromEntry " << srcNode->distFromEntry
                                << endl;
                        })
#endif
                        if (srcNode->_level <= lastBarLevel)
                           srcNode->distToExit += 1;
                        else
                           srcNode->distFromEntry += 1;
#if VERBOSE_DEBUG_SCHEDULER
                        DEBUG_SCHED (6, 
                           cerr << "$$ After update src node " << *srcNode 
                                << " has distToExit " << srcNode->distToExit
                                << " and distFromEntry " << srcNode->distFromEntry
                                << endl;
                        )
#endif
                     }
                  }
               }
            } else
            if (srcLevel>lastBarLevel && destLevel<=lastBarLevel)
            {
               // this should have been a loop carried dependency
               assert (edge->ddistance > 0);
               edge->ddistance -= 1;
               modified = true;
               adjustment = -1;
               if (edge->ddistance==0)
                  edge->dlevel = 0;

               if (edge->isSuperEdge())  // it is a super-structure
               {
                  // traverse all edges in this super-structure and
                  // update distance to entry and exit for affected nodes.
                  unsigned int mm = new_marker();
                  assert (edge->subEdges);
#if VERBOSE_DEBUG_SCHEDULER
                  DEBUG_SCHED (6, 
                     cerr << endl << "*** Start updating distances for super-edge (T2) "
                          << *edge << endl;
                  )
#endif
                  EdgeSet::iterator eit = edge->subEdges->begin();
                  for ( ; eit!=edge->subEdges->end() ; ++eit)
                  {
                     Edge *ee = *eit;
                     Node *srcNode = ee->source();
                     if (srcNode!=edge->source() && !srcNode->visited(mm))
                     {
                        srcNode->visit(mm);
#if VERBOSE_DEBUG_SCHEDULER
                        DEBUG_SCHED (6, 
                        {
                           cerr << "** Process edge " << *ee << endl;
                           cerr << "## Before update src node " << *srcNode 
                                << " has distToExit " << srcNode->distToExit
                                << " and distFromEntry " << srcNode->distFromEntry
                                << endl;
                        })
#endif
                        if (srcNode->_level > lastBarLevel)
                        {
                           srcNode->distToExit -= 1;
                           assert (srcNode->distToExit >= 0);
                        }
                        else
                        {
                           srcNode->distFromEntry -= 1;
                           assert (srcNode->distFromEntry >= 0);
                        }
#if VERBOSE_DEBUG_SCHEDULER
                        DEBUG_SCHED (6, 
                           cerr << "$$ After update src node " << *srcNode 
                                << " has distToExit " << srcNode->distToExit
                                << " and distFromEntry " << srcNode->distFromEntry
                                << endl;
                        )
#endif
                     }
                  }
               }
            } else
            if (edge->isSuperEdge() && edge->ddistance==1)
            {
               // traverse all edges in this super-structure and
               // update distance to entry and exit for affected nodes.
               unsigned int mm = new_marker();
               int structLevel = edge->source()->_level;
               assert (edge->subEdges);
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (6, 
                  cerr << endl << "*** Start updating distances for super-edge (T3) "
                       << *edge << endl;
               )
#endif
               EdgeSet::iterator eit = edge->subEdges->begin();
               for ( ; eit!=edge->subEdges->end() ; ++eit)
               {
                  Edge *ee = *eit;
                  Node *srcNode = ee->source();
                  if (srcNode!=edge->source() && !srcNode->visited(mm))
                  {
                     srcNode->visit(mm);
#if VERBOSE_DEBUG_SCHEDULER
                     DEBUG_SCHED (6, 
                     {
                        cerr << "** Process edge " << *ee << endl;
                        cerr << "## Before update src node " << *srcNode 
                             << " has distToExit " << srcNode->distToExit
                             << " and distFromEntry " << srcNode->distFromEntry
                             << endl;
                     })
#endif
                     if (structLevel<=lastBarLevel && 
                               srcNode->_level>lastBarLevel)
                     {
                        srcNode->distToExit -= 1;
                        srcNode->distFromEntry += 1;
                        assert (srcNode->distToExit >= 0);
                     }
                     if (structLevel>lastBarLevel && 
                               srcNode->_level<=lastBarLevel)
                     {
                        srcNode->distFromEntry -= 1;
                        srcNode->distToExit += 1;
                        assert (srcNode->distFromEntry >= 0);
                     }
#if VERBOSE_DEBUG_SCHEDULER
                     DEBUG_SCHED (6, 
                        cerr << "$$ After update src node " << *srcNode 
                             << " has distToExit " << srcNode->distToExit
                             << " and distFromEntry " << srcNode->distFromEntry
                             << endl;
                     )
#endif
                  }
               }
            }
         }
         // - If edge has a super edge, find the top super-edge and check if
         // it is part of a cycle or not. An edge that has a super-edge means
         // that it is part of a structure. We can have nested structures.
         // Can a structure have back-edges internaly? Can a super-structure 
         // contain cycles? // I will not check this for now, but I should 
         // have an assert if I find a cycle whyle computing super edges.
         // - If edge does not have super-edge,
         // then check if the edge itself is part of a cycle.
         Edge *topSE = edge;
         // find top super-structure;
         // Even if we have edges with multiple super-edges, these would run
         // between the same pair of nodes. If one of them is part of a cycle,
         // then the others will be part of some cycles as well. Its enough to
         // test only along the first super-edge.
         bool foundCycle = false;
         int esrcScc = edge->source()->getSccId();
         int esinkScc = edge->sink()->getSccId();
         
         if (esrcScc && esrcScc==esinkScc) // edge is part of an SCC, must have cycles
         {
            edge->hasCycles = 3;
            edge->source()->hasCycles = 3;
            foundCycle = true;
         }
         while (topSE->superE)
            topSE = topSE->superE;
         if (!topSE->cyclesIds.empty())
         {
            // check if there is at least one cycle, or we got only segments
            UISet::iterator sit = topSE->cyclesIds.begin ();
            for ( ; sit!=topSE->cyclesIds.end() ; ++sit )
               if (cycles[*sit]->isCycle ())
               {
                  edge->hasCycles = 3;
                  edge->source()->hasCycles = 3;
                  foundCycle = true;
                  // set also the SCC Id for the internal nodes
                  int srcScc = topSE->source()->getSccId();
                  int sinkScc = topSE->sink()->getSccId();
                  // the next condition should be always true if the edge is
                  // part of cycles
                  assert (srcScc && srcScc==sinkScc);
                  int eScc = edge->source()->getSccId();
                  if (eScc == 0)
                     edge->source()->setSccId(srcScc);
                  else
                     assert (eScc == srcScc);
                  break;
               }
         } 
         // if the edge is a loop carried dependency, make it loop indep
         if (!foundCycle && edge->getDistance ()>0)
         {
            // if edge does not have any cycles and is loop carried, it 
            // can have only distance 1 me thinks. Put an assert.
            // 09/17/06 mgabi: Found that it can be more than 1 if the 
            // instruction is a replacement instruction built out of
            // two or more instructions that had distance 1 from one to
            // the next. It looks like the Sun compiler performs software
            // pipelining now.
            // 10/22/06 mgabi: Found that there can be memory dependencies
            // (accidentaly even) with very large distances. This happens
            // when the location and the strides are all constant,
            // which would produce a dependency with a long distance even 
            // when there is none in fact.
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (4, 
               cerr << "Found carried dependency edge part of no cycle " 
                    << *edge << endl;
            )
#endif
            assert (edge->ddistance==1 || edge->sink()->CreatedByReplace()
                  || edge->dtype==MEMORY_TYPE);
            adjustment -= edge->getDistance ();
            edge->ddistance = 0;
            edge->dlevel = 0;
            modified = true;
         }

         // if I made an adjustment to this edge, I must also adjust the
         // number of iterations of all paths it is a part of
         if (adjustment && !edge->cyclesIds.empty())
         {
            UISet::iterator sit = edge->cyclesIds.begin ();
            for ( ; sit!=edge->cyclesIds.end() ; ++sit )
            {
               // adjust only segments. Cycles are affected only by the
               // first transformation (translation of the last barrier node
               // as the last node of the graph). However, for a cycle I will
               // always have another edge part of the cycle that is affected
               // in the opposite way such that the total number of iterations
               // crossed is not changed.
               Cycle *crtCyc = cycles[*sit];
               if (crtCyc->isSegment())
               {
                  crtCyc->iters += adjustment;
                  assert (crtCyc->iters >= 0);
               }
            }
         }
      }
      ++ edit;
   }
   
   // compute also latency of cycles
   longestCycle = 0;
   // even for non SWP cases I should have an artificial cycle
   if (avgNumIters > ONE_ITERATION_EPSILON) {
     for (int i=1 ; i<nextCycleId ; ++i) {
       if (cycles[i] != NULL && cycles[i]->isCycle()) {
	 // determine the longest cycle
	 // update cycle length info for each node

            int thisCyc = cycles[i]->getLatency();
//ozgurS
 
            int thisCycMem = cycles[i]->getMemLatency();
            int thisCycCPU = cycles[i]->getCPULatency();
cout<<__func__<<"ozgur: MemLat: "<<thisCycMem<<endl;
cout<<__func__<<"ozgur: CPULat: "<<thisCycCPU<<endl;
//ozgurE
#if VERBOSE_DEBUG_PALM
	 DEBUG_PALM(1,
		    cout << __func__ <<" "<<thisCyc<<" "<<i<<endl;
		    )
#endif
	   if (longestCycle<thisCyc) {
	     longestCycle = thisCyc;
	     longestCycleMem = thisCycMem;//ozgurS
	     longestCycleCPU = thisCycCPU;//ozgurE
	   }
	 EdgeList::iterator eit = cycles[i]->edges->begin();
	 for( ; eit!=cycles[i]->edges->end() ; ++eit ) {
	   Node *nn = (*eit)->source();
	   if (nn->longestCycle < thisCyc) {
	     nn->longestCycle = thisCyc;
	     nn->longestCycleMem = thisCycMem;//ozgurS
	     nn->longestCycleCPU = thisCycCPU;//ozgurE
	   }
	   nn->sumAllCycles += thisCyc;
	   if ((*eit)->longestCycle < thisCyc) {
	     (*eit)->longestCycle = thisCyc;
	     (*eit)->longestCycleMem = thisCycMem;//ozgurS
	     (*eit)->longestCycleCPU = thisCycCPU;//ozgurE
	     if ((*eit)->isSuperEdge ())
	       updateLongestCycleOfSubEdges (*eit, thisCyc);
	   }
	   (*eit)->sumAllCycles += thisCyc;
	 }
       }
     }
   }

   // when cycles are computed I determine which nodes are connected by cycle
   // edges. I use a union-find data structure to mark nodes that are 
   // grouped together. Now I have to traverse all nodes and build a 
   // collection of sets of nodes, grouped by their id. Create also the DAG 
   // nodes. Store also the longest cycle for each of the sets.
   /* gmarin, 09/18/2013:
    * Mark nodes and edges with the ID of the SCC they are part of.
    * I found a DEADLOCK case in the scheduler where several connected
    * short cycles make a Strongly Connected Component, with a diamond-like 
    * structure inside. Because the nodes and edges are part of different 
    * cycles, the scheduling priority picks edges from different cycles 
    * causing the two ends of a diamond to be scheduled before all the middle
    * nodes are scheduled. I think this can happen only with SCCs, a rather
    * rare case. I will use the SCC ID to check that a node has all incoming
    * (or outgoing) edges that are part of an SCC, either scheduled, or not, 
    * before selecting it for scheduling.
    */
   NodesIterator nit (*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      if (nn->isInstructionNode())
      {
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (7, 
            cerr << "End of SchedDG::findCycle: node " << *nn << endl;
         )
#endif
         Ui2NSMap::iterator usit;
         if (nn->hasCycles == 3)
         {
            // node is part of at least one cycle
            unsigned int leaderId = nn->getSccId();
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (6, 
               cerr << "End of SchedDG::findCycle: node " << *nn 
                    << " has leaderId=" << leaderId << endl;
            )
#endif
            usit = cycleNodes.find (leaderId);
            assert (usit != cycleNodes.end());
            unsigned int &crtLC = setsLongestCycle[leaderId];
            if ((int)crtLC < nn->longestCycle)
               crtLC = nn->longestCycle;
         } 
      }
      ++ nit;
   }
#if USE_SCC_PATHS
   // Now, traverse all the SCCs and compute the scc path from root / to leaf
   Ui2NSMap::iterator scit;
   mm = new_marker();
   for (scit= cycleNodes.begin() ; scit!=cycleNodes.end() ; ++scit)
   {
      int sccId = scit->first;
      NodeSet::iterator nsit;
      for (nsit=scit->second.begin() ; nsit!=scit->second.end() ; ++nsit)
      {
         (*nsit)->computeSccPathToLeaf(mm, sccId);
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (6, 
            cerr << "computeSccPathToLeaf (sccId=" << sccId << ") for node " << *(*nsit)
                 << ", sccPathToLeaf=" << (*nsit)->sccPathToLeaf 
                 << ", sccEdgesToLeaf=" << (*nsit)->sccEdgesToLeaf 
                 << endl;
         )
#endif
      }
   }
   mm = new_marker();
   for (scit= cycleNodes.begin() ; scit!=cycleNodes.end() ; ++scit)
   {
      int sccId = scit->first;
      NodeSet::iterator nsit;
      for (nsit=scit->second.begin() ; nsit!=scit->second.end() ; ++nsit)
      {
         (*nsit)->computeSccPathFromRoot(mm, sccId);
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (6, 
            cerr << "computeSccPathFromRoot (sccId=" << sccId << ") for node " << *(*nsit)
                 << ", sccPathFromRoot=" << (*nsit)->sccPathFromRoot
                 << ", sccEdgesFromRoot=" << (*nsit)->sccEdgesFromRoot
                 << endl;
         )
#endif
      }
   }
#endif
   if (modified)
      ComputeNodeLevels();
}
//--------------------------------------------------------------------------------------------------------------------

// I will color the nodes as I traverse them. 
// - m = gray, means I reached the node but I am not done with it. It is used
//   to detect cycles.
// - m2 = means I finished working with this node for now, but I might get
//   back to it on a different path. This value is incremented when we finish
//   working on a SCC.
// I must keep track of colors that are locked. 
// Initially, minBlack = initial m2 (m+1). maxBlack is incremented to cover
// each new locked color.
// gmarin, 11/13/2013: do not allow cycles that include multiple carried 
// dependencies that were created by indirect memory accesses; trying to 
// fix a problematic case. However, I want to traverse each edge at least once.
int
SchedDG::findCycle (SchedDG::Node* node, unsigned int m, unsigned int m2,
           EdgeList& _edges, CycleMap* all_cycles, 
           unsigned int lastIdx, unsigned int &minIdx,
           int iters, int sccId)
{
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout << __func__ << " " <<"n: "<<(unsigned int*)node->getAddress()<<endl;
            )
#endif
//   if (node->visited (m+1, m2))  // node fully explored; nothing to do here
   if (node->visited(m2))  // node fully explored; nothing to do here
      return 0;
   
   if (node->visited(m))   // found a cycle
   {
      // must not include cycles that contain only one node (and edge)
      // and cycles that were discovered before
      // Idea: find node with minimum id and test that we have the same
      // edges starting from that node.
      assert(! _edges.empty());
      // should consider one edge cycles if that is a memory dependency,
      // think pointer chasing. What if it is a memory output dependency?
      // It should be still preserved, so one edge (memory dependency) 
      // cycles are allowed.
      // mgabi 08/16/2005: one edge cycles should be preserved, think sum
      // reduction; I may say that one node cycles that have two register 
      // operands should be preserved? 
      // Preserve also one edge cycles when the edge is a super-edge
      // 06/04/2007 mgabi: pointer chaising is not represented by a memory
      // dependency, but by a register dependency on a load. I cannot have
      // memory dependency on a load (from load to load). Only on a Store
      // and that is an output dependency, not sure what use it has.
      // So, preserve one edge cycles if the node is a load and the dependency
      // type is a ADDR_REGISTER type.
      Edge *lastEdge = _edges.back();
      if (lastEdge->source()==node && lastEdge->getType()!=SUPER_EDGE_TYPE
              && !(lastEdge->getType()==ADDR_REGISTER_TYPE &&
                  node->is_load_instruction ())
              && node->srcRegs.size()<2 )
         return 0;
      
      // recover the edges that are part of the cycle
      EdgeList::iterator elit = _edges.begin();
      for( ; elit!=_edges.end() ; ++elit )
      {
         if ((*elit)->source()==node)  // we found the start of the cycle
            break;
      }
      if (elit == _edges.end())
      {
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (1, 
         {
            fprintf (stderr, "Error: SchedDG::findCycle: found a cycle but I cannot find its start. Edges in cycle are:\n");
            for(elit=_edges.begin() ; elit!=_edges.end() ; ++elit )
               cerr << " @ " << *(*elit) << endl;
         }
         )
#endif
      }
      assert (elit!=_edges.end());

      EdgeList tempEL;
      // find node with minimum id
      int minId = node->getId();
      EdgeList::iterator minPos = elit;
      EdgeList::iterator auxit = elit;
      for ( ; auxit!=_edges.end() ; ++auxit)
      {
         if ((*auxit)->source()->getId() < minId)
         {
            minId = (*auxit)->source()->getId();
            minPos = auxit;
         }
      }
      tempEL.insert (tempEL.end(), minPos, _edges.end());
      tempEL.insert (tempEL.end(), elit, minPos);

#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (5, 
         fprintf (stderr, "findCycle: found a new cycle %d ... \n", ++numCyclesFound);
      )
#endif
      
      addUniqueCycle (tempEL, all_cycles);
      tempEL.clear();
      minIdx = node->tempVal;
      assert (minIdx > 0);
      return (1);
   }

   // I want to determine how many cycles we've found before finishing work with this node
   // Save the current total number of cycles
   int numCyclesOnEntry = nextCycleId;
   
   node->visit(m);
   lastIdx += 1;
   node->tempVal = lastIdx;
   int result = 0;
   
   OutgoingEdgesIterator oeit(node);
   while ((bool)oeit)
   {
      // iters should count the number of carried dependencies.
      Edge *ee = oeit;
      if (ee->sink()->getSccId() != sccId)  // cannot be part of a cycle
      {
         ++ oeit;
         continue;
      }
      // Limiting the total number of cycles to an arbitrary value prevents us
      // from uncovering a cycle for each edge, even if we start our search from 
      // that edge. We may get boggered down in some dense region of the graph, 
      // using up all the available spots before we can find an elementary cycle
      // that goes back to the starting node ...
      //
      // If we are in limp mode, maybe I should limit visiting each node only once;
      // once we process all its successors, mark it as fully processed. However, we
      // open the graph again when we start from a new SCC edge that does not have 
      // any cycles yet. We can take every edge at most once for each start. Thus,
      // complexity would be O(E) * starting edges with no cycles.
      int edist = 0;
      // do not allow cycles with more than 1 iters in limpMode
//      if ((ee->isIndirect() || limpModeFindCycle) && ee->getDistance()!=0)
      // Remove this restriction since I am closing explored nodes to further
      // traversal in limp mode? See how it works
      if (ee->isIndirect() && ee->getDistance()!=0)
         edist = 1;
      
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (5, 
         cerr << "findCycle: considering edge " << *ee 
              << " with edist=" << edist << ", crt iters=" << iters
              << ", visited2(" << m2 << ")=" << ee->visited2(m2) 
              << ", visited(" << m << ")=" << ee->visited(m) 
              << endl;
      )
#endif
      // do not consider control dependencies for cycles
      if (!(ee->IsRemoved()) && !(ee->IsInvisible()) && 
          !(ee->visited(m)) &&
           (ee->isSchedulingEdge() || ee->isSuperEdge()) &&
           (edist+iters<2 || !edist || !ee->visited2(m2))
         )
      {
         _edges.push_back(ee);
         ee->visit2(m2);
         Node *sinkN = ee->sink();
         unsigned int locMinIdx = 0;
         int locRes = findCycle(sinkN, m, m2, _edges, all_cycles, lastIdx, 
              locMinIdx, iters+edist, sccId);
         _edges.pop_back();
         if (locRes && locMinIdx && (!minIdx || minIdx>locMinIdx))
            minIdx = locMinIdx;
         if (locMinIdx && node->tempVal==locMinIdx)
         {
            // this edge leads to a fully explored cycle
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (5, 
               cerr << "Returned from findCycle with locMinIdx==node->tempVal=" 
                    << locMinIdx << " edge " << *((Edge*)oeit) << endl;
            )
#endif
            ee->visit(m);
         }
         result |= locRes;
      }
      ++ oeit;
   }
   // if we found more cycles than the total number of edges in the graph, 
   // starting from just this node, and no less than 100, then fall back onto 
   // a more restrictive cycle discovery algorithm. Notify when it happens. 
   // It is an heuristic.
   int numCyclesNow = nextCycleId - numCyclesOnEntry;
   if (numCyclesNow>numSchedEdges && numCyclesNow>150 && !limpModeFindCycle)  // more cycles than # of edges
   {
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (0, 
         cerr << "findCycle(" << pathId.Name() << "): While processing node " 
              << *node << " we discovered " << numCyclesNow 
              << " new cycles, which exceeds total number of "
              << numSchedEdges << " edges in the graph. Attempting to degrade gracefully." 
              << endl;
      )
#endif
      limpModeFindCycle = true;
   }
   if (limpModeFindCycle)
   {
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (6, 
         fprintf(stderr, "findCycle: In LimpMode, total number of cycles=%d\n",
                  nextCycleId);
      )
#endif
   }
   // unmark the node when returning from recurence
   if (limpModeFindCycle)
      node->visit(m2);
   else
      node->visit(m-1);
   if (result) assert (minIdx > 0);
   return (result);
}
//--------------------------------------------------------------------------------------------------------------------

bool
SchedDG::EdgeListCompare::operator() (const EdgeList* el1, 
              const EdgeList* el2) const
{
   EdgeList::const_iterator eit1 = el1->begin();
   EdgeList::const_iterator eit2 = el2->begin();
   for( ; eit1!=el1->end() && eit2!=el2->end() ; ++eit1, ++eit2 )
   {
      // I will do the sorting by pointer value; can it go wrong?
      // No, it should still find if two edge lists are equal
      if ( (*eit1) < (*eit2))
         return (true);
      if ( (*eit1) > (*eit2))
         return (false);
   }
   // if we are here, then at least one of the iterators is at the end
   // consider a shorter list to be of lesser value. Also, if the lists are
   // equal (including equal length - both iterators at the end), then we
   // must return false (for NOT less).
   if (eit2==el2->end())
      return (false);
   else   // it means eit1 is at the end and eit2 is not
      return (true);
}
//--------------------------------------------------------------------------------------------------------------------

// next two methods can be called only while building the cycles. I store them
// temporarly in a list, but at the end I will copy them in an array
// Specify if the list of edges should be considered as a segment even if
// the first and the last node are the same
unsigned int
SchedDG::addCycle(EdgeList& el, CycleMap* all_cycles, bool is_segment)
{
   unsigned int cycId = nextCycleId ++;

#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (7, 
      fprintf (stderr, "added cycle %d.\n", cycId);
   )
#endif

   SchedDG::Cycle* cyc = new Cycle (cycId, el, ds, is_segment);
   all_cycles->insert (CycleMap::value_type (cyc->edges, cyc));
//   cycles.insert(CycleMap::value_type(cycId, cyc));
   return (cycId);
}
//--------------------------------------------------------------------------------------------------------------------

unsigned int
SchedDG::addUniqueCycle(EdgeList& el, CycleMap* all_cycles, bool is_segment)
{
   // determine if we have seen this cycle before
   CycleMap::iterator cmit = all_cycles->find(&el);
   if (cmit==all_cycles->end())
   {
      unsigned int newCycId = addCycle (el, all_cycles, is_segment);
      return (newCycId);
   }
   else
   {
      // it is not unique cycle
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (7, 
         fprintf (stderr, "not unique.\n");
      )
#endif
      return (cmit->second->getId());
   }
}      
//--------------------------------------------------------------------------------------------------------------------

// this method can be called only after all the cycles are built, and 
// honestly I do not know where it will be used, but anyway our cycles are 
// in an array at this time
int
SchedDG::removeCycle(unsigned int cycleId)
{
   if (cycles[cycleId] == NULL)
      return (0);
   delete (cycles[cycleId]);
   cycles[cycleId] = NULL;
   return (1);
}
//--------------------------------------------------------------------------------------------------------------------

void
SchedDG::removeAllCycles()
{
   for (int i=1 ; i<nextCycleId ; ++i)
      if (cycles[i] != NULL)
         delete (cycles[i]);
   delete[] (cycles);
   nextCycleId = 1;
   assert (cycleDag);
   delete (cycleDag);
   cycleDag = NULL;
   removeCfgFlags (DG_CYCLES_COMPUTED);
}
//--------------------------------------------------------------------------------------------------------------------

SchedDG::Cycle::Cycle(unsigned int _id, EdgeList& eList, DisjointSet *ds,
            bool _is_segment)
{
   cycleId = _id;
   cLength = 0;
   numEdges = 0;
   _flags = 0;
   usedSlack = 0;
   iters = 0;
   revIters = 0;
   
   // 01/07/08 mgabi: There will be no paths stored anymore. Only true cycles
   assert (! _is_segment);
   is_segment = _is_segment;
   bool hasLat = true;
   bool onlySuperEdges = true;
   edges = new EdgeList();
   EdgeList::iterator eit = eList.begin();
   firstNode = (*eit)->source();
   lastNode = eList.back()->sink();
   int lastNodeId = ds->Find (lastNode->id);
   numDiffPaths = 1.0;
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout << __func__ << " "  <<(unsigned int*)firstNode->getAddress()<<" "<<(unsigned int*)lastNode->getAddress()<<" "<<lastNodeId<<endl;
            )
#endif
   for( ; eit!=eList.end() ; ++eit )
   {
      numEdges += 1;
      cLength += (*eit)->getLatency();
      (*eit)->addCycle (cycleId, _is_segment);
      edges->push_back (*eit);
      Node *eSrc = (*eit)->source();
      eSrc->addCycle (cycleId, _is_segment);
      int crtId = ds->Find (eSrc->id);
      if (lastNodeId != crtId)
      {
         ds->Union (lastNodeId, crtId);
         lastNodeId = ds->Find (lastNodeId);
      }
      if (! (*eit)->HasLatency())
      {
         hasLat = false;
         // I will make cycle discovery a private method, and I will invoke it
         // only after latencies are computed. Should not find an edge
         // without latency.
         assert (!"In Cycle constructor, found edge without latency info.");
      }
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout << __func__ << " "  << (*eit)->getLatency() <<" "<<(*eit)->getDistance()<<" "<<(unsigned int*)(*eit)->source()->getAddress()<<" "<<(unsigned int*)(*eit)->sink()->getAddress()<<endl;
            )
#endif
      if ((*eit)->getDistance() > 0)
         iters += (*eit)->getDistance();
      if (! (*eit)->isSuperEdge())
         onlySuperEdges = false;
      else if (!is_segment)
      {
         numDiffPaths *= (*eit)->numPaths;
      }
   }
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout << __func__ << " "  <<(unsigned int*)firstNode->getAddress()<<" "<<(unsigned int*)lastNode->getAddress()<<" "<<lastNodeId<<endl;      )
#endif
   if (hasLat)
      _flags |= (LATENCY_COMPUTED_CYCLE);
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout << __func__ << " " <<"iters: "<<iters<<" (firstNode!=lastNode) "<<(firstNode!=lastNode)<<endl;      )
#endif
  assert (iters>=1 || (firstNode!=lastNode));
//    && lastNode->isBarrierNode()));
   remainingLength = cLength;

   if (firstNode == lastNode)  // it is a cycle, unless is_segment==true
   {
      revIters = 1.0/(float)iters;
      float ilen = (float)cLength * revIters;
      iterLength = (unsigned int)(ceil(ilen)+0.00001);
      if (iters > 1)
      {
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (5, 
            cerr << "##> Found a multi-iteration cycle/segment " << cycleId
                 << ", hasLat " << hasLat << ", length=" << cLength 
                 << ", iters=" << iters << ", iterLength=" << iterLength 
                 << ", is_segment " << is_segment << ". List of Nodes:" << endl;
            for( eit=eList.begin() ; eit!=eList.end() ; ++eit )
               cerr << *((*eit)->source()) << " -- ";
            cerr << endl;
         )
#endif
      }
      if (onlySuperEdges && !is_segment)
      {
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (4, 
            cerr << "##> Found a super edges cycle " << cycleId
                 << ", hasLat " << hasLat << ", length=" << cLength 
                 << ", iters=" << iters << ", iterLength=" << iterLength 
                 << endl;
         )
#endif
      }
   } else
   {
      lastNode->addCycle (cycleId, _is_segment);
      iterLength = 0;  //cLength;   // it is meaningless for segments
   }
}

void
SchedDG::Cycle::PrintObject (ostream& os) const
{
   cerr << "Cycle " << cycleId << " of length=" << cLength 
        << ", iters=" << iters << ", iterLength=" << iterLength 
        << ", is_segment " << is_segment << ". List of Edges:" << endl;
   EdgeList::iterator eit;
   for (eit=edges->begin() ; eit!=edges->end() ; ++eit)
      cerr << "{" << *(*eit) << "} -- ";
   cerr << endl;
}

SchedDG::Cycle::~Cycle()
{
   EdgeList::iterator eit = edges->begin();
   for( ; eit!=edges->end() ; ++eit )
   {
      (*eit)->removeCycle(cycleId);
      (*eit)->source()->removeCycle(cycleId);
   }
   if (firstNode != lastNode)  // a segment
      lastNode->removeCycle(cycleId);
   edges->clear();
   delete edges;
   _flags = 0;
}
//ozgurS
//adding getmem/cpu latency for cycle
//
int
SchedDG::Cycle::getRawMemLatency()
{
//   if (_flags & LATENCY_COMPUTED_CYCLE)
//      return (cLength);
   // otherwise attempt to compute the latency
   int cLength = 0;
   bool hasLat = true;
   EdgeList::iterator eit = edges->begin();
   for( ; eit!=edges->end() ; ++eit )
   {
      cLength += (*eit)->getMemLatency();
      if (! (*eit)->HasLatency())
         hasLat = false;
   }
   if (hasLat)
      _flags |= LATENCY_COMPUTED_CYCLE;
   else
   {
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (1, 
         fprintf (stderr, "WARNING: SchedDG::Cycle::getLatency: required to compute cycle latency, but not all edges have latency information.\n");
      )
#endif
   }
cout<<__func__<<" ozgur: RawMemLat: "<<cLength<<endl;
   return (cLength);
}

int
SchedDG::Cycle::getMemLatency()
{
//   if (_flags & LATENCY_COMPUTED_CYCLE)
//      return (iterLength);
   float ilen = (float)getRawMemLatency() * revIters;
   int memLength = (unsigned int)(ceil(ilen)+0.00001);
cout<<__func__<<"ozgur: memLat: "<<memLength<<endl;
   return (memLength);
}

int
SchedDG::Cycle::getRawCPULatency()
{
//   if (_flags & LATENCY_COMPUTED_CYCLE)
//      return (cLength);
   // otherwise attempt to compute the latency
   int cLength = 0;
   bool hasLat = true;
   EdgeList::iterator eit = edges->begin();
   for( ; eit!=edges->end() ; ++eit )
   {
      cLength += (*eit)->getCPULatency();
      if (! (*eit)->HasLatency())
         hasLat = false;
   }
   if (hasLat)
      _flags |= LATENCY_COMPUTED_CYCLE;
   else
   {
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (1, 
         fprintf (stderr, "WARNING: SchedDG::Cycle::getLatency: required to compute cycle latency, but not all edges have latency information.\n");
      )
#endif
   }
    cout<<__func__<<" ozgur: RawCPULat: "<<cLength<<endl;
   return (cLength);
}

int
SchedDG::Cycle::getCPULatency()
{
//   if (_flags & LATENCY_COMPUTED_CYCLE)
//      return (iterLength);
   float ilen = (float)getRawCPULatency() * revIters;
   int cpuLength = (unsigned int)(ceil(ilen)+0.00001);
cout<<__func__<<" ozgur: cpuLat: "<<cpuLength<<endl;
   return (cpuLength);
}
//ozgurE

int
SchedDG::Cycle::getRawLatency()
{
   if (_flags & LATENCY_COMPUTED_CYCLE)
      return (cLength);
   // otherwise attempt to compute the latency
   cLength = 0;
   bool hasLat = true;
   EdgeList::iterator eit = edges->begin();
   for( ; eit!=edges->end() ; ++eit )
   {
      cLength += (*eit)->getLatency();
      if (! (*eit)->HasLatency())
         hasLat = false;
   }
   if (hasLat)
      _flags |= LATENCY_COMPUTED_CYCLE;
   else
   {
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (1, 
         fprintf (stderr, "WARNING: SchedDG::Cycle::getLatency: required to compute cycle latency, but not all edges have latency information.\n");
      )
#endif
   }
   return (cLength);
}

int
SchedDG::Cycle::getLatency()
{
   if (_flags & LATENCY_COMPUTED_CYCLE)
      return (iterLength);
   float ilen = (float)getRawLatency() * revIters;
   iterLength = (unsigned int)(ceil(ilen)+0.00001);
   return (iterLength);
}

bool
SchedDG::Cycle::equalTo(EdgeList& el)
{
   EdgeList::iterator eit1 = edges->begin();
   EdgeList::iterator eit2 = el.begin();
   for( ; eit1!=edges->end() && eit2!=el.end() ; ++eit1, ++eit2 )
   {
      if ( (*eit1) != (*eit2))
         return (false);
   }
   if (eit1==edges->end() && eit2==el.end())
      return (true);
   else
      return (false);
}

int 
SchedDG::Cycle::findMaxSchedID()
{
   int maxSchedId = 0;
   EdgeList::iterator elit = edges->begin();
   for ( ; elit!=edges->end() ; ++elit)
   {
      int l = (*elit)->source()->scheduleId();
      if (l > maxSchedId)
         maxSchedId = l;
   }
   return (maxSchedId);
}

void 
SchedDG::Edge::addCycle (unsigned int cycId, bool _is_segment)
{
   cyclesIds.insert(cycId);
}

void 
SchedDG::Node::addCycle (unsigned int cycId, bool _is_segment)
{
   cyclesIds.insert (cycId);
//   if (!_is_segment) hasCycles = 3;
}

int 
SchedDG::Edge::removeCycle(unsigned int cycId)
{
   UISet::iterator sit = cyclesIds.find(cycId);
   if (sit==cyclesIds.end())
      return (0);
   cyclesIds.erase(sit);
   return (1);
}

int 
SchedDG::Node::removeCycle(unsigned int cycId)
{
   UISet::iterator sit = cyclesIds.find(cycId);
   if (sit==cyclesIds.end())
      return (0);
   cyclesIds.erase(sit);
   return (1);
}

SchedDG::Edge*
SchedDG::Node::findOutgoingEdge (Node *dest, int edirection, int etype, 
               int eiters, int elevel)
{
   OutgoingEdgesIterator oeit(this);
   while ((bool)oeit)
   {
      Edge* tempE = oeit;
      if ( (tempE->sink()==dest || dest==ANY_GRAPH_NODE) && 
           (tempE->getDirection()==edirection || edirection==ANY_DEPENDENCE_DIRECTION) &&
           (tempE->getType()==etype || etype==ANY_DEPENDENCE_TYPE) && 
           (tempE->getDistance()==eiters || eiters==ANY_DEPENDENCE_DISTANCE) &&
           (tempE->getLevel()==elevel || elevel==ANY_DEPENDENCE_LEVEL)
         )   // return the first edge found
         return (tempE);
      ++ oeit;
   }
   return (NULL);
}

SchedDG::Edge*
SchedDG::Node::findIncomingEdge (Node *src, int edirection, int etype, 
               int eiters, int elevel)
{
   IncomingEdgesIterator ieit(this);
   while ((bool)ieit)
   {
      Edge* tempE = ieit;
      if ( (tempE->source()==src || src==ANY_GRAPH_NODE) && 
           (tempE->getDirection()==edirection || edirection==ANY_DEPENDENCE_DIRECTION) &&
           (tempE->getType()==etype || etype==ANY_DEPENDENCE_TYPE) && 
           (tempE->getDistance()==eiters || eiters==ANY_DEPENDENCE_DISTANCE) &&
           (tempE->getLevel()==elevel || elevel==ANY_DEPENDENCE_LEVEL)
         )   // return the first edge found
         return (tempE);
      ++ ieit;
   }
   return (NULL);
}

// this method is called after all instructions have been parsed and the graph
// was constructed. It checks if there any barrier nodes in this graph, and 
// adds control edges from the last barrier node to the top nodes, and from
// the bottom nodes to the first barrier node
void
SchedDG::finalizeGraphConstruction ()
{
   if (barrierNodes.empty())  // no barrier node, nothing to do here
      return;
   computeTopNodes ();
   Node *firstBN = barrierNodes.front ();
   Node *lastBN = barrierNodes.back ();
   
   // add edges from the bottom nodes to the first barrier node
   NodeList::iterator nlit = bottomNodes.begin ();
   for ( ; nlit!=bottomNodes.end() ; ++nlit )
   {
      // if the last barrier node is a bottom node, do not add an edge to
      // the first barrier node; I will add in the loop below only if the 
      // first barrier node is a top node
      if ((*nlit)!=lastBN)
         addUniqueDependency((*nlit), firstBN, TRUE_DEPENDENCY,
                CONTROL_TYPE, 1, 1, 0, 0.0);
   }
   
   // add also control edges from lastBN to the top nodes
   // if lastBN is the same node as lastBranch, then I already added them
   if (lastBN != lastBranch)
   {
      nlit = topNodes.begin ();
      for ( ; nlit!=topNodes.end() ; ++nlit )
      {
         addUniqueDependency(lastBN, (*nlit), TRUE_DEPENDENCY,
                CONTROL_TYPE, 1, 1, 0, 0.0);
      }
   }
}

int
SchedDG::computeTopNodes()
{
   // delete the old top_nodes
   OutgoingEdgesIterator oeit(cfg_top);
   while ((bool)oeit)
   {
      Edge* tempE = oeit;
      ++ oeit;
      remove (tempE);
      delete (tempE);
   }
   topNodes.clear();

   // delete the old bottom_nodes
   IncomingEdgesIterator ieit(cfg_bottom);
   while ((bool)ieit)
   {
      Edge* tempE = ieit;
      ++ ieit;
      remove (tempE);
      delete (tempE);
   }
   bottomNodes.clear();
   
   // recompute the top nodes and the bottom nodes
//   unsigned int m = new_marker ();
   NodesIterator nit(*this);
   while ((bool)nit) 
   {
      Node *nn = nit;
      if (nn->isInstructionNode())
      {
//         nn->visit (m-2);  // mark all nodes with this value
         bool is_top = true, is_bottom = true;
         IncomingEdgesIterator lieit(nn);
         while ((bool)lieit)
         {
            // I have to test for those conditional branches that are
            // going to be removed because I compute top nodes early on
            // to decide to which nodes I have to add control dependencies
            // from the last branch or barrier node.
            // I need to change this. Perhaps, if the machine has branch
            // prediction, then I do not create these dependencies in the
            // first place. Right now I create them even if I prune them
            // right after that.
            if (!lieit->IsRemoved() && lieit->isSchedulingEdge() &&
                 lieit->getLevel()==0 &&
                   (lieit->getType()!=CONTROL_TYPE ||
                    lieit->getProbability()<BR_HIGH_PROBABILITY ||
                    nn->isBarrierNode() ||
                    lieit->source()->isBarrierNode()) )
            {
               is_top = false;
               break;
            }
            ++ lieit;
         }
         
         OutgoingEdgesIterator loeit(nn);
         while ((bool)loeit)
         {
            if (!loeit->IsRemoved() && loeit->isSchedulingEdge() &&
                 loeit->getLevel()==0 &&
                   (loeit->getType()!=CONTROL_TYPE ||
                    loeit->getProbability()<BR_HIGH_PROBABILITY ||
                    nn->isBarrierNode() ||
                    loeit->sink()->isBarrierNode()) )
            {
               is_bottom = false;
               break;
            }
            ++ loeit;
         }

         if (is_top)
         {
            topNodes.push_back(nn);
            // create also a special type edge
            Edge* edge = new Edge(cfg_top, nn, 0, STRUCTURE_TYPE, 0, 0, 0);
            add(edge);
         }
         if (is_bottom)
         {
            bottomNodes.push_back(nn);
            // create a special type edge
            Edge* edge = new Edge(nn, cfg_bottom, 0, STRUCTURE_TYPE, 0, 0, 0);
            add(edge);
         }
      }
      ++ nit;
   }
#if 0
   // THIS METHOD NEEDS SOME MORE THINKING AND WORK; KEEP THE OLD METHOD
   // FOR NOW.
   // I consider as top nodes only those nodes that do not have any incoming 
   // edges at all. Similarly for bottom nodes.
   // Now I have to do DFS traversals from all top nodes and see if I can
   // reach all instructions. If not, it means some are in some isolated
   // recurrences and I have to pick a top node for them.
   // Do another traversal from the bottom nodes and pick other bottom nodes
   // if necessary.
   NodeList::iterator lit = topNodes.begin ();
   for ( ; lit!=topNodes.end() ; ++lit)
   {
      Node *nn = *lit;
      assert (nn->isInstructionNode ());
      assert (nn->visited (m-2);
      dfs_forward (nn, m, 0);
   }
   // traverse all nodes and see if any of them is not marked to m-1
   bool changed = true;
   do
   {
      changed = false;
      NodesIterator nit2 (*this);
      while ((bool)nit2)
      {
         Node *nn = nit2;
         if (! nn->visited (m-1))  // this node was not visited
         {
            // it must be marked with m-2
            assert (nn->visited (m-2);
            // check if it has any loop independent incoming edges
            bool is_top = true, is_bottom = true;
            IncomingEdgesIterator lieit(nn);
            while ((bool)lieit)
            {
               if (!lieit->IsRemoved() && lieit->isSchedulingEdge() 
                     && lieit->dlevel==0)
               {
                  is_top = false;
                  break;
               }
               ++ lieit;
            }
            if (is_top)
            /* THIS IS way more complicated. I need to keep track how
             * many iterations I moved a node, so in case I get there on
             * a different path with fewer/more iterations crossed, I need
             * to adjust. Problem is if fewer iterations are needed. Then
             * I have to go check which edges previously processed were 
             * affected (start a separate search forward from that node), and
             * also try to push the excess distance on the incoming edges that
             * were already processed. I think it can be done, but it is going
             * to take some thinking and time.
             */
         }
         ++ nit2;
      }
   } while (changed);
#endif 
   return (topNodes.size());
}
//--------------------------------------------------------------------------------------------------------------------

#if 0
// The assumption is that at start, all nodes are marked with a value 
// different than mm or mm-1. When this routine is invoked from computeTopNodes
// all nodes should be marked with mm-2.
// Use marker mm as a temporary value for nodes on current DFS path, 
// and mm-1 as a final value for nodes that I visited completely.
int 
SchedDG::dfs_forward (Node *nn, unsigned int mm, int distance)
{
   if (nn->visited (mm-1))   // processed this node completely before, return
      return (0);
   if (nn->visited (mm))    // I have a cycle
   {
      assert (distance > 0);
      return (1);
   }
   
   nn->visit (mm);
   OutgoingEdgesIterator eit (nn);
   while ((bool)eit)
   {
      Edge *ee = eit;
      if (!ee->IsRemoved() && ee->isSchedulingEdge())
         if (dfs_forward (ee->sink(), mm, distance+ee->ddistance)) 
         {
            // this last edge is a back-edge
            int dist = distance + ee->ddistance;
            assert (dist > 0);
            cerr << "Transform edge " << *ee << " into loop carried with distance "
                 << dist << endl;
            if (dist>1)
               cerr << "WARNING: Changing edge to MULTI-ITERATIONS distance." << endl;
            
            ee->ddistance = dist;
            ee->dlevel = 1;
         } else
         if (ee->dlevel > 0)
         {
            cerr << "Change edge " << *ee << " into loop independent" << endl;
            ee->ddistance = 0;
            ee->dlevel = 0;
         }
      ++ eit;
   }
   nn->visit (mm-1);
   return (0);
}
//--------------------------------------------------------------------------------------------------------------------
#endif

void
SchedDG::Edge::computeLatency(const Machine *_mach, addrtype reloc, SchedDG* sdg)
{
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout << __func__ << " "  <<"SchedDG::Edge::computeLatency "<<(_flags & LATENCY_COMPUTED_EDGE)<<endl;
            )
#endif
   if (_flags & LATENCY_COMPUTED_EDGE)
      return;    // computed it before
   
   InstructionClass sourceType = source()->makeIClass();
   InstructionClass sinkType = sink()->makeIClass();
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout << __func__ << " "  <<Convert_InstrBin_to_string(sourceType.type)<<" "<<Convert_InstrBin_to_string(sinkType.type)<<endl;
            )
#endif
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout << __func__ << " "  << "has bypass: "<<_mach->hasBypassLatency (sourceType, dtype, sinkType)<<endl;
            )
#endif
   int rez;
   if ((rez = _mach->hasBypassLatency (sourceType, dtype, sinkType)) >= 0)
   {
      // has bypass latency defined
      _flags |= HAS_BYPASS_RULE_EDGE;
      minLatency = realLatency = rez;
//ozgurS 
      minCPULatency = realCPULatency = rez;
//ozgurE
   } 
   else
   {
      minLatency = _mach->computeLatency (source()->getAddress(), reloc, sourceType, sinkType);
      // overlapped was introduced to use register dependencies into or
      // from within a loop. It can be different than zero just for register 
      // dependencies that have one end into a loop. It might have other uses
      // in the future, but for now the solution is strictly for register
      // dependencies to/from a loop node.
      if ((dtype==GP_REGISTER_TYPE || dtype==ADDR_REGISTER_TYPE) &&
              overlapped>0)
      {
         if (minLatency > overlapped)
            minLatency -= overlapped;
         else
            minLatency = 1;  // we want a minimum latency of 1 such that the
                             // inner loop does not overlap with any instruct.
      }
//ozgurS
      minCPULatency = minLatency;
//ozgurE
   }
   if (source()->getAddress() != sink()->getAddress()){ //because a single instruction can be represented by multipl uops, only apply addition load latency when transitioning to a new instruction
    minLatency += sdg->img->getMemLoadLatency(source()->getAddress());
//ozgurS
    minMemLatency= realMemLatency = sdg->img->getMemLoadLatency(source()->getAddress());  
//ozgurE
   }
//ozgurS
//    cout << "ozgur: MemLat: "<<minMemLatency<<" minLat: "<<minLatency<<endl;
//ozgurE
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout << __func__ << " "  <<"insn: "<<(unsigned int*)source()->getAddress()<<"->" <<(unsigned int*)sink()->getAddress() << " minlatency: "<<minLatency<<endl;
            )
#endif
   _flags |= LATENCY_COMPUTED_EDGE;
}
//--------------------------------------------------------------------------------------------------------------------

// I shall call this method from updateGraphForMachine, so the Machine info 
// is passed only once. I do not want to impose an artificial calling order 
// for these two methods. If I make public only one of these methods,
// then the calling order problem disappears.
void
SchedDG::computeEdgeLatencies()
{
   // I receive _mach from updateGraphForMachine
   // I need a method in the Instruction class to return the latency given
   // a consumer instruction. I can test there for special cases when an
   // instruction creates inequal length latencies depending on the consumer.
 
//ozgurDELETETHIS
   cout<<"Ozgur:  "<<__func__<<" this func called"<<endl; 
   EdgesIterator eit(*this);
   while ((bool)eit)
   {
      Edge *ee = eit;
      Node *srcNode = ee->source();
      Node *destNode = ee->sink();

      if ( ! ee->IsRemoved() && ee->getType()!=STRUCTURE_TYPE)
      {
         if  (srcNode->isInstructionNode() && 
              destNode->isInstructionNode())
            ee->computeLatency(mach, reloc_offset,this);
      }
      ++ eit;
   }
}
//--------------------------------------------------------------------------------------------------------------------

void
SchedDG::pruneControlDependencies()
{
   EdgesIterator eit(*this);
   while ((bool)eit)
   {
      // do not remove control edges from/into unreordable nodes
      Edge *ee = eit;
      if  (!ee->IsRemoved() && ee->getType()==CONTROL_TYPE && 
           !ee->source()->isBarrierNode() && 
           !ee->sink()->isBarrierNode() && 
           (ee->getProbability()>=BR_HIGH_PROBABILITY ||
             (ee->source()==lastBranch && avgNumIters>1)) )
         {
            ee->MarkRemoved();
            setCfgFlags(DG_GRAPH_IS_MODIFIED);
         }
      ++ eit;
   }
}
//--------------------------------------------------------------------------------------------------------------------

void 
SchedDG::updateGraphForMachine(const Machine* _mach)
{
   std::cerr << "[INFO]SchedDG::updateGraphForMachine: '" << name() << "/" << _mach->MachineName() << "'\n";
   if (HasAllFlags (DG_MACHINE_DEFINED))   // machine defined before
      assert(!"SchedDG::updateGraphForMachine was invoked, but the graph \
      has been previously updated with a machine description.");

   // First of all, if the machine has branch prediction, remove control
   // dependencies with high probability
   if (_mach->hasBranchPrediction())
      pruneControlDependencies();
      
   // if graph is not pruned, then prune it first; some replacement rules
   // work better on the pruned graph
   if (HasNoFlags (DG_GRAPH_PRUNED))
      pruneDependencyGraph();

#if DRAW_DEBUG_GRAPH
   // print also the debug graph at this point
   char file_name[64];
   if (!targetPath || targetPath==pathId)
   {
      sprintf (file_name, "k-%s", pathId.Filename());
      draw_debug_min_graphics (file_name);
   }
#endif

   //draw_debug_graphics ("x6_P");
   mach = _mach;
   
   // There are several things I need to do:
   // - traverse the graph and apply the replacement rules
   // - prune redundant dependencies and compute a latency for the 
   //   remaining ones. Do the pruning first. Cheaper to perform replacements
   //   on pruned graph.
   const ReplacementList* repL = mach->Replacements();
   UIList iters_crossed;
   if (repL!=NULL)  // we have some replacement rules
   {
      unsigned int replacementCount = 0;
      ReplacementList::const_iterator rlit = repL->begin();
      for ( ; rlit!=repL->end() ; ++rlit)
      {
         PatternGraph* findPattern = (*rlit)->TemplateGraph();
         PatternGraph::Node* findNode = findPattern->TopNode();
         // repeat the search until no new replacement is found
         // use try ... catch to unwind the stack
         int count = 0;
         bool found = false;
         do 
         {
            try
            {
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (7, 
                  cerr << "Call patternSearch for rule " 
                       << (*rlit)->RuleName()
                       << endl;
               )
#endif
               patternSearch (findNode, iters_crossed);
               // if I return here it means pattern was not found
               found = false;
            } catch (PatternFoundException pfe)
            {
               found = true;
               count += 1;
               
//               found = false;
               // perform the replacement of the matched subgraph
               ++ replacementCount;
               replacePattern(*rlit, replacementCount, pfe.NumIterations());
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (2, 
                  fprintf(stderr, "PathId %s: Found replacement rule %s with %ld edges for %d times\n",
                       pathId.Name(), (*rlit)->RuleName(), pfe.NumIterations().size(), count);
               )
#endif
            }
         } while (found);
      }
   }
   
   // after all replacement rules are applied, compute the latency of 
   // all remaining edges
   computeEdgeLatencies();
   
   if (haveErrors)
   {
      fprintf(stderr, "Stop execution because program encountered %d errors.\n",
            haveErrors);
      exit(-10);
   }
   setCfgFlags (DG_MACHINE_DEFINED);

   // after computing latencies, prune the graph again because more edges
   // can be pruned once the latencies are known
   pruneDependencyGraph();
}
//--------------------------------------------------------------------------------------------------------------------

void
SchedDG::replacePattern(ReplacementRule* rule, unsigned int replacementIndex,
           UIList& iterCrossed)
{
   // traverse the destination pattern and build the subgraph for it
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (3, 
      fprintf(stderr, "** Inside replacePattern, rule %s **\n", rule->RuleName());
   )
#endif
   GenInstList* dPattern = rule->DestinationPattern();
   PRegMap& regMap = rule->RegisterMap();
   
   // iterCrossed stores the distance of each matched edge.
   // If the source pattern has more than one node, it is possible that
   // not all nodes are from the same iteration (some edges have distance>0).
   // I will create all destination pattern nodes in the same iteration as the
   // last actual node matched by the source pattern. I need to compute how
   // many iterations do I cross for each of the other nodes of the source
   // pattern. For each node (except the last one), number of iterations moved
   // is equal to the sum of distances of the edges in the source pattern
   // matched after this node. I will compute this information in iterCrossed
   // by computing partial sums from the end of the list to each node.
   UIList::iterator ulit = iterCrossed.end();
   unsigned int partSum = 0;
   while (ulit!=iterCrossed.begin())
   {
      -- ulit;
      partSum += (*ulit);
      *ulit = partSum;
   }
   
   // Now traverse the source pattern graph and map this info to each node
   ulit = iterCrossed.begin();
   PatternGraph* findPattern = rule->TemplateGraph();
   PatternGraph::Node* findNode = findPattern->TopNode();
   int useLevel = 0;
   width_t max_width = 0;
   int max_veclen = 0;
   int maxReplIndex = 0;

   while (findNode!=NULL)
   {
      Node* actualN = dynamic_cast<Node*>(findNode->ActualNode());
      assert (actualN != NULL);
      
      // find maximum in_order index across all source nodes
      if (actualN->in_order > maxReplIndex)
         maxReplIndex = actualN->in_order;
      
      // get maximum bit width and maximum vector length of matched nodes
      if (actualN->bit_width > max_width)
         max_width = actualN->bit_width;
      if (actualN->exec_unit==ExecUnit_VECTOR && 
             (actualN->vec_len*actualN->bit_width) > max_veclen)
         max_veclen = actualN->vec_len * actualN->bit_width;
      PatternGraph::Edge* findEdge = findNode->FirstSuccessor();
      // if findEdge is NULL. it means this is the last node in
      // the pattern. 
      if (findEdge==NULL)
      {
         assert (ulit == iterCrossed.end());
         findNode->SetIterationsMoved (0);
         // get the level corresponding to the last node in the pattern
         useLevel = actualN->_level;
         findNode = NULL;
      } else
      {
         assert (ulit!=iterCrossed.end());
         findNode->SetIterationsMoved (*ulit);
         ++ ulit;
         findNode = findEdge->sink();
      }
   };
   assert (useLevel > 0);
   // I need to find also the last Barrier
   Node *lastBarrier = lastBarrierNode[useLevel];
   Node *nextBarrier = nextBarrierNode[useLevel];
   
   // use a temporary local map to understand dependencies between 
   // instructions in the destination pattern. In this map store only register
   // definitions, I do not need to store SOURCE registers
   PRegMap locMap;
   // give a numeric value to the local registers; start with values from
   // 1000 times the index of the replacement operation
   int topLocalReg = replacementIndex*1000;
   
   Node *outputRegNode = NULL;
   
   GenInstList::iterator git = dPattern->begin();
   for ( ; git!=dPattern->end() ; ++git)
   {
//      int itype = (*git)->Type();
      const InstructionClass& ic = (*git)->getIClass();
      Node *node = new Node(this, MAKE_REPLACE_NODE_ID(replacementIndex), 0, ic);
      node->setCreatedByReplace();
      node->setInOrderIndex(maxReplIndex);
      
      // set also the maximum vector length found among the nodes matched by the 
      // source pattern, and the maximum bit width if the destination pattern's node
      // does not have a specified width
      if (node->bit_width==0 || node->bit_width==max_operand_bit_width)
         node->bit_width = max_width;
      if (node->exec_unit==ExecUnit_VECTOR && node->vec_width==0)
      {
         if (node->bit_width!=0 && node->bit_width!=max_operand_bit_width)
         {
            node->vec_width = max_veclen;
            node->vec_len = max_veclen / node->bit_width;
         }
         else
         {
            fprintf (stderr, "Warning: Replacement node has vector unit, but I cannot determine the number of elements in the vector because the bit width is WILDCARD.\n");
            node->vec_len = 1;
         }
         if (node->vec_len < 1)
            node->vec_len = 1;
      } else
         node->vec_len = 1;
      add (node);  // add the new nodes directly so I can use addUniqueDep
      // assign a temporary level to the new nodes
      node->_level = useLevel;
      bool hasDep = false;
      
      GenRegList* regList = (*git)->SourceRegisters();
      GenRegList::iterator rit = regList->begin();
      for ( ; rit!=regList->end() ; ++rit)
      {
         int rittype = (*rit)->Type();
         PRegMap::iterator pit = regMap.find((*rit)->RegName());

         // there are 3 cases I have to consider
         // 1. register found and is a source register -> create proper depend
         // 2. register found and is destination register -> create proper dep
         // 3. register not found or found and is temporary -> use a local map
         if (pit==regMap.end() || (pit->second.regType & TEMPORARY_REG))
         // This is CASE 3
         {
            PRegMap::iterator lit = locMap.find((*rit)->RegName());
            DependencyType dtype = GP_REGISTER_TYPE;
            if (lit != locMap.end())  // register was defined by this pattern
            {
               int regVal = lit->second.index;
               RegisterClass rtype;
               if (lit->second.regType == MEMORY_TYPE)
               {
                  rtype = RegisterClass_MEM_OP;
                  dtype = MEMORY_TYPE;
               }
               else
               {
                  rtype = RegisterClass_REG_OP;
                  if (rittype == MEMORY_TYPE)
                     dtype = ADDR_REGISTER_TYPE;
               }
               node->addRegReads(register_info(regVal, rtype, 0, node->bit_width-1));
                  
               addUniqueDependency (dynamic_cast<Node*>(lit->second.node), 
                   node, TRUE_DEPENDENCY, dtype, LOOP_INDEPENDENT, 0, 0);
            }
            // else, if the register was not defined by the pattern, then 
            // create no dependency. Assume the register was loaded somewhere 
            // before the loop.
         }
         else    // CASES 1 & 2
         {
            // I know for sure that pit does not point to the end of the map
            // check if it is source or destination. 
            if ((pit->second.regType) & SOURCE_REG)   // it was source and it is source
            {
               // the difficult case; I have to create the proper dependency
               int pittype = (pit->second.regType) & REGTYPE_MASK;
               int ridx = pit->second.index;
               // get the source pattern node where the register is defined
               PatternGraph::Node *srcNode = 
                       dynamic_cast<PatternGraph::Node*>(pit->second.node);
               Node* actualN = dynamic_cast<Node*>(srcNode->ActualNode());
               unsigned int iters_moved = srcNode->IterationsMoved ();
               
               // traverse the list of incoming edges into the actualN, and 
               // find the ridx-th edge of type pittype; 
               // should I skip over edges mapped to the source pattern?
               // I skip them now. 
               // I also duplicate all the CONTROL_TYPE dependencies that were 
               // entering the original node
               // I need to test if all nodes have at least a dependency
               // from a node which is after the last barrier node. If not,
               // then create a CONTROL dependency from the last barrier node.
               // FIXME - can it fail for more complex patterns??
               PatternGraph::Edge* mappedEdge = srcNode->FirstPredecessor();
               Edge* actualPred = NULL;
               if (mappedEdge!=NULL)
                  actualPred = dynamic_cast<Edge*>(mappedEdge->ActualEdge());
               
               IncomingEdgesIterator ieit(actualN);
               // when I constructed the register map, register indeces start
               // from zero. Thus, initialize idx with -1
               int idx = -1;
               bool foundEdge = false;
               bool foundReg = false;
//               register_info theFoundReg(MIAMI_NO_REG);
               Edge* theFoundEdge = NULL;
               // I do not differentiate between gp and fp registers anymore (bummer)
               // because there is not such a clear cut distinction between them on x86.
               // xmm can be used as different types, x87 regs (ST0-7) can be used as MMX, etc.
               // 2011-12-27 gmarin: However, now I explicitly mark registers used as
               // part of address calculation, and also mark register dependencies accordingly.
/*
               if (pittype==FP_REGISTER_TYPE) regArray = &(actualN->fpRegSrc);
               else regArray = &(actualN->gpRegSrc);
 */
               while ((bool)ieit)
               {
                  // duplicate all the control dependencies
                  if (! ieit->IsRemoved() && ieit->getType()==CONTROL_TYPE )
                  {
                     if (lastBarrier && 
                           ieit->source()->_level>=lastBarrier->_level)
                        hasDep = true;
                     int eLevel = ieit->getLevel();
                     if (iters_moved>0)
                        eLevel = 1;
                     addUniqueDependency (ieit->source(), node, 
                              ieit->getDirection(), ieit->getType(), 
                              ieit->getDistance()+(int)iters_moved, 
                              eLevel, 0);
                  }
                  // this is a source register/memory; it can be only 
                  // TRUE dependency
                  if (  !ieit->IsRemoved() && actualPred!=(Edge*)ieit && 
                        !foundEdge && ieit->getType()==pittype && 
                        ieit->getDirection()==TRUE_DEPENDENCY )
                  {
                     ++ idx;
                     if (idx == ridx)  // this is my edge
                     {
                        foundEdge = true;
                        theFoundEdge = ieit;
                        // do not break because we want to duplicate all 
                        // control dependencies and addr dependencies
                        // break;
                     }
                  }
                  // if this node has a memory operand, duplicate all address
                  // dependencies as well
                  if (! ieit->IsRemoved() && pittype==MEMORY_TYPE && 
                       ieit->getType()==ADDR_REGISTER_TYPE)
                  {
                     int eLevel = ieit->getLevel();
                     if (iters_moved>0)
                        eLevel = 1;
                     Node* foundSrc = ieit->source();
                     int newDist = ieit->getDistance()+iters_moved;
                     addUniqueDependency (foundSrc, node, 
                              ieit->getDirection(), ieit->getType(), 
                              newDist, eLevel, 0);
                     if (lastBarrier && ((lastBarrier->_level<useLevel && 
                           foundSrc->_level>=lastBarrier->_level && eLevel==0) || 
                           (lastBarrier->_level>useLevel && 
                              ( (foundSrc->_level<=useLevel && newDist==0) || 
                                (foundSrc->_level>lastBarrier->_level && newDist==1)
                              )
                           ) ) )
                        hasDep = true;
                  }
                  ++ ieit;
               }
               idx = -1;
               // I need to find the registers that I add to the newly created replacement node
               // If memory operand, add all MEM_OP registers. Otherwise, count the GP registers
               // for the right one.
               // **** HERE **** DO THIS ****
               RInfoList& regList = actualN->srcRegs;
               RInfoList::iterator lit = regList.begin();
               while (lit!=regList.end())
               {
                  if (lit->type==RegisterClass_MEM_OP)
                  {
                     if (pittype == MEMORY_TYPE)
                     {
                        // add all MEM_OP registers
                        node->addRegReads(*lit);
                        foundReg = true;
                     }
                  }  // not a MEM_OP register; REG_OP, STACK_REG, TEMP_REG and LEA_OP
                     // are all considered General Purpose
                  else
                     if (pittype==GP_REGISTER_TYPE)
                     {
                        ++ idx;
                        if (idx == ridx)
                        {
                           foundReg = true;
                           node->addRegReads(*lit);
//                           theFoundReg = *lit;
                           break;
                        }
                     }
                  ++ lit;
               }
               if (!foundReg)   // I did not find a GP reg
               {
#if VERBOSE_DEBUG_SCHEDULER
                  const Position& fpos = (*rit)->FilePosition();
                  DEBUG_SCHED (6, 
                    fprintf(stderr, "Warning: File %s (%d, %d): Source register $%s in right hand side "
                        "replacement template, was defined as a source register in the left hand side "
                        "template at (%d, %d), but I cannot find an actual register of type %d, index %d. "
                        "Perhaps it is an immediate value operand. PathId %s.\n",
                        fpos.FileName(), fpos.Line(), fpos.Column(), 
                        (*rit)->RegName(), pit->second.pos.Line(), 
                        pit->second.pos.Column(), pittype, ridx, pathId.Name());
                  )
#endif
               }
               if (foundEdge)   // I found my edge
               {
                  // create an edge from the old source to this node, with the
                  // attributes of the old edge
                  int eLevel = theFoundEdge->getLevel();
                  if (iters_moved>0)
                     eLevel = 1;
                  Node* foundSrc = theFoundEdge->source();
                  int newDist = theFoundEdge->getDistance()+iters_moved;
                  addUniqueDependency (foundSrc, node, 
                           theFoundEdge->getDirection(), 
                           theFoundEdge->getType(), newDist, eLevel, 0);
                  if (lastBarrier && ((lastBarrier->_level<useLevel && 
                      foundSrc->_level>=lastBarrier->_level && eLevel==0) || 
                      (lastBarrier->_level>useLevel && 
                         ( (foundSrc->_level<=useLevel && newDist==0) || 
                           (foundSrc->_level>lastBarrier->_level && newDist==1)
                         )
                      ) ) )
                     hasDep = true;
               } else   // did not find it, print warning
               {
#if VERBOSE_DEBUG_SCHEDULER
                  const Position& fpos = (*rit)->FilePosition();
                  DEBUG_SCHED (6, 
                    fprintf(stderr, "Warning: File %s (%d, %d): Source register $%s in right hand side "
                        "replacement template, was defined as a source register in the left hand side "
                        "template at (%d, %d), but I cannot find an actual edge of type %d, index %d. "
                        "Perhaps it is an immediate value operand, or a register defined before the "
                        "loop. PathId %s.\n",
                        fpos.FileName(), fpos.Line(), fpos.Column(), 
                        (*rit)->RegName(), pit->second.pos.Line(), 
                        pit->second.pos.Column(), pittype, ridx, pathId.Name());
                  )
#endif
               }
               
            } else
            {
               assert ((pit->second.regType) & DESTINATION_REG || !"Invalid register type");
               haveErrors += 1;
               const Position& fpos = (*rit)->FilePosition();
               fprintf(stderr, "Error %d: PathId %s: File %s (%d, %d): Register $%s is used as a source register in right hand side "
                      "replacement template, but was defined as a destination register on the left hand side template at (%d, %d)\n",
                      haveErrors, pathId.Name(), fpos.FileName(), fpos.Line(), fpos.Column(), 
                      (*rit)->RegName(), pit->second.pos.Line(), pit->second.pos.Column() );
            }
         }
      }
      
      // now traverse the destination registers for this instruction
      regList = (*git)->DestRegisters();
      rit = regList->begin();
      for ( ; rit!=regList->end() ; ++rit)
      {
         PRegMap::iterator pit = regMap.find((*rit)->RegName());

         // there are 3 cases I have to consider
         // 1. register found and is a source register -> create proper depend
         // 2. register found and is destination register -> create proper dep
         // 3. register not found or found and is temporary -> use a local map
         if (pit==regMap.end() || (pit->second.regType & TEMPORARY_REG))
         // This is CASE 3
         {
            // this register is defined here. I have to include it in the 
            // local map, or if it was used before I have to update the info
            PRegMap::iterator lit = locMap.find((*rit)->RegName());
            if (lit != locMap.end())  // register was defined before
            {
               // update the info
               lit->second.node = node;
               lit->second.index = topLocalReg;
               lit->second.pos = (*rit)->FilePosition();
            } else    // add a new entry for this register
               locMap.insert(PRegMap::value_type((*rit)->RegName(),
                     PRegInfo(node, (*rit)->Type(), topLocalReg,
                     (*rit)->FilePosition() )) );
            
            node->addRegWrites(register_info(topLocalReg++, RegisterClass_TEMP_REG, 0, node->bit_width-1));
         }
         else    // CASES 1 & 2
         {
            // I know for sure that pit does not point to the end of the map
            // check if it is source or destination. 
            if ((pit->second.regType) & DESTINATION_REG) // it was dest and it is dest
            {
               // there should be a single node that produces the output
               // register; assert this.
               // 02/20/2012, gmarin: Well, I can specify replacement rules that split 
               // a larger width instruction into smaller width instructions of the 
               // same type. In such a case, there are multiple output nodes in the 
               // destination pattern. Comment out the assert and keep track only of the
               // last outputRegNode. This variable is only used to set the replacement 
               // node for the removed matched nodes, but that information is not used
               // anywhere after that.
//               assert (outputRegNode==NULL || outputRegNode==node);
               outputRegNode = node;
               
               // the difficult case; I have to create the proper dependencies
               int pittype = (pit->second.regType) & REGTYPE_MASK;
               // int ridx = pit->second.index;
               
               // get the source pattern node where the register is defined
               // This can be only the last node???
               PatternGraph::Node *srcNode = 
                       dynamic_cast<PatternGraph::Node*>(pit->second.node);
               Node* actualN = dynamic_cast<Node*>(srcNode->ActualNode());
               
               // if this is a MEMORY_TYPE register, traverse all the incoming edges
               // of the actual node, and copy the ADDR_REGISTER_TYPE edges and registers
               if (pittype==MEMORY_TYPE)
               {
                  IncomingEdgesIterator ieit(actualN);
                  while((bool)ieit)
                  {
                     if (!ieit->IsRemoved() && ieit->getType()==ADDR_REGISTER_TYPE)
                     {
                        unsigned int iters_moved = srcNode->IterationsMoved ();
                        int eLevel = ieit->getLevel();
                        if (iters_moved>0)
                           eLevel = 1;
                        Node* foundSrc = ieit->source();
                        int newDist = ieit->getDistance()+iters_moved;
                        addUniqueDependency (foundSrc, node, 
                              ieit->getDirection(), ieit->getType(), 
                              newDist, eLevel, 0);
                        if (lastBarrier && ((lastBarrier->_level<useLevel && 
                             foundSrc->_level>=lastBarrier->_level && eLevel==0) || 
                             (lastBarrier->_level>useLevel && 
                                ( (foundSrc->_level<=useLevel && newDist==0) || 
                                  (foundSrc->_level>lastBarrier->_level && newDist==1)
                                )
                             ) ) )
                           hasDep = true;
                     }
                  
                     ++ ieit;
                  }
                  // add also all address registers
                  RInfoList& regList = actualN->srcRegs;
                  RInfoList::iterator lit = regList.begin();
                  while (lit!=regList.end())
                  {
                     if (lit->type==RegisterClass_MEM_OP)
                     {
                        // add all MEM_OP registers
                        node->addRegReads(*lit);
                     }
                     ++ lit;
                  }
               }
               
               // traverse the list of outgoing edges from the actualN, and 
               // create a duplicate edge from this node to each sink node; 
               // Create only dependencies of pittype type??? FIXME
               // Duplicate also all outgoing control dependencies
               OutgoingEdgesIterator oeit(actualN);
               bool found = false;
               while ((bool)oeit)
               {
                  if (! oeit->IsRemoved() && oeit->isSchedulingEdge() &&
                        (oeit->getType()==pittype || oeit->getType()==CONTROL_TYPE ||
                          (pittype==GP_REGISTER_TYPE && oeit->getType()==ADDR_REGISTER_TYPE)
                        ))
                  {
                     if (oeit->getType() != CONTROL_TYPE)
                        found = true;
                     // create an edge from this node to the old sink, 
                     // with the attributes of the old edge
                     addUniqueDependency (node, oeit->sink(), 
                              oeit->getDirection(), oeit->getType(), 
                              oeit->getDistance(), oeit->getLevel(), 0);
                  }
                  ++ oeit;
               }
               // Also, add all the registers of type pittype
               // We have all registers in a single list now. They have an associated
               // type as GP (various styles) or ADDR registers.
               node->destRegs = actualN->destRegs;
               if (! found)   // I did not find any valid edge; print error
               {
                  // I think this would be a weird case. The code cannot just 
                  // finnish suddenly unless this is a store into memory
                  // Found it can happen if the path contains an entrance into
                  // an inner loop where the value might be consumed, but it
                  // is not used by the instructions at current level
#if VERBOSE_DEBUG_SCHEDULER
                  const Position& fpos = (*rit)->FilePosition();
                  DEBUG_SCHED (6, 
                    fprintf(stderr, "Warning: File %s (%d, %d): Destination register $%s in right hand side "
                         "replacement template, was defined as a destination register in the left hand side "
                         "template at (%d, %d), but I cannot find any actual outgoing edge of type %d. PathId %s.\n",
                         fpos.FileName(), fpos.Line(), fpos.Column(), 
                         (*rit)->RegName(), pit->second.pos.Line(), 
                         pit->second.pos.Column(), pittype, pathId.Name());
                  )
#endif
               }
            } else
            {
               assert ((pit->second.regType) & SOURCE_REG || !"Invalid register type");
               haveErrors += 1;
               const Position& fpos = (*rit)->FilePosition();
               fprintf(stderr, "Error %d: PathId %s: File %s (%d, %d): Register $%s is used as a destination register in right hand side "
                      "replacement template, but was defined as a source register in the left hand side template at (%d, %d)\n",
                      haveErrors, pathId.Name(), fpos.FileName(), fpos.Line(), fpos.Column(), 
                      (*rit)->RegName(), pit->second.pos.Line(), pit->second.pos.Column() );
            }
         }   // CASES 1 & 2
      }   // for each destination register

      // if we did not find an edge from the last Barrier, add one
      if (lastBarrier && !hasDep) 
      {
         int myDist = 0;
         int myLevel = 0;
         if (lastBarrier->_level > useLevel)
         {
            myDist = 1; myLevel = 1;
         }
         addUniqueDependency (lastBarrier, node, 
                TRUE_DEPENDENCY, CONTROL_TYPE, 
                myDist, myLevel, 0);
      }

   }   // for each instruction in the destination pattern
   
   // I built a subgraph corresponding to the destination pattern
   // Now I have to remove the nodes and edges mapped to the source pattern
   // taking care to keep the nodes that compute partial results that are used
   // elsewhere
   findNode = findPattern->TopNode();
   // I will traverse the source pattern and check if any of the actual
   // nodes has any outgoing dependency that is not mapped to the source 
   // pattern. Do not consider outgoing control dependencies if this is not
   // the last node in the pattern.
   // At the same time, set actualN and actualE to NULL in the source pattern
   // to catch eventual errors earlier.
   // 10/24/06 mgabi: I need to also check if any of the source nodes of the 
   // removed edges, that are not part of the actual source pattern, 
   // have any remaining outgoing dependencies. If not, we need to add
   // a control dependency to the next barrier node.
   //
   NodeList eraseNodes;  // keep here the nodes that I want to delete
   EdgeList eraseEdges;  // keep here the edges that I want to delete
   NodeSet  predNodes;   // nodes that are predecessors of deleted nodes
   // keep track of the last node that it is replicated. I need to make sure
   // that it will have some dependency to the next barrier node.
   Node* lastNode = NULL;
   while (findNode!=NULL)
   {
      Node* actualN = dynamic_cast<Node*>(findNode->ActualNode());
      assert (actualN!=NULL);
      eraseNodes.push_back(actualN);
      actualN->setReplacementNode (outputRegNode);
      findNode->SetActualNode(NULL);
      
      PatternGraph::Edge* findEdge = findNode->FirstSuccessor();
      // if findEdge is NULL. it means this is the last node in
      // the pattern. I can exit the loop, as the dependencies out of
      // the last node are treated differently.
      if (findEdge==NULL) break;
      
      Edge* actualE = dynamic_cast<Edge*>(findEdge->ActualEdge());
      assert (actualE!=NULL);
      // see if actualN has any other valid outgoing dependencies
      OutgoingEdgesIterator oeit(actualN);
      while ((bool)oeit)
      {
         if (! oeit->IsRemoved() && oeit->isSchedulingEdge() &&
               oeit->getType()!=CONTROL_TYPE && actualE!=oeit)  
         // found another edge
         {
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (2, 
               fprintf(stderr, "ReplacePattern: Found node in the pattern at address 0x%lx, type %d with at least one more dependency\n",
                      actualN->getAddress(), actualN->getType() );
               cerr << (Edge*)oeit << endl;
            )
#endif
            // clear the lists of nodes and edges that must be erased
            // mark the nodes that are not erased as being replicated
            NodeList::iterator nlit = eraseNodes.begin();
            for ( ; nlit!=eraseNodes.end() ; ++nlit )
               (*nlit)->setNodeIsReplicated();
            eraseNodes.clear();
            eraseEdges.clear();
            lastNode = actualN;
            break;  // no need to check for additional edges, found one
         }
         ++ oeit;
      }
      eraseEdges.push_back(actualE);
      findEdge->SetActualEdge(NULL);
      findNode = findEdge->sink();
   };
   
   // delete the nodes and edges in the erase lists; delete the edges first
   EdgeList::iterator elit = eraseEdges.begin();
   for ( ; elit!=eraseEdges.end() ; ++elit )
   {
      Edge* tempE = *elit;
      remove (tempE); delete (tempE);
   }
   eraseEdges.clear();
   NodeList::iterator nlit = eraseNodes.begin();
   for ( ; nlit!=eraseNodes.end() ; ++nlit )
   {
      Node* tempN = *nlit;
      // traverse all its incoming edges and add the predecessors into a set
      IncomingEdgesIterator ieit (tempN);
      while ((bool)ieit)
      {
         Node *predN = ieit->source();
         if (!ieit->IsRemoved() && ieit->isSchedulingEdge() &&
             !predN->IsRemoved() && predN->isInstructionNode())
            predNodes.insert (predN);
         ++ ieit;
      }
      tempN->MarkRemoved ();
      remove (tempN); 
      // do not delete the nodes. They will be added into a separate set
      //delete (tempN);
   }
   eraseNodes.clear();
   
   // check all predNodes for remaining outgoing dependencies
   if (nextBarrier != NULL)
   {
      NodeSet::iterator nsit = predNodes.begin ();
      for ( ; nsit!=predNodes.end() ; ++nsit)
      {
         Node *predN = *nsit;
         if (!predN->IsRemoved())
         {
            int predLevel = predN->_level;
            Node *predNextBarrier = nextBarrierNode[predLevel];
            bool hasDep = false;
            // since nextBarrier is not NULL, we should find a next barrier
            // node for any level (I think). Put the assert and see if it ever
            // fails. Moreover, with the restrictions currently in place
            // regarding edges that cross barrier nodes, I think that 
            // predNextBarrier should be the same node as nextBarrier.
            // Should I assert this as well?
            assert (predNextBarrier != NULL);
            if (predN == predNextBarrier)
               continue;
            OutgoingEdgesIterator oeit(predN);
            while ((bool)oeit)
            {
               if (!oeit->IsRemoved() && oeit->isSchedulingEdge())
               {
                  Node *sinkNode = oeit->sink ();
                  int eDistance = oeit->getDistance ();
                  if ( (predLevel<predNextBarrier->_level && 
                        sinkNode->_level<=predNextBarrier->_level && 
                        eDistance==0) ||
                       (predLevel>predNextBarrier->_level && 
                          ( (sinkNode->_level>predLevel && eDistance==0) ||
                            (sinkNode->_level<=predNextBarrier->_level 
                                    && eDistance==1)))
                     )
                  {
                     hasDep = true;
                     break;
                  }
               }
               ++ oeit;
            }
            if (!hasDep)
            {
               int myDist = 0;
               int myLevel = 0;
               if (predNextBarrier->_level < predLevel)
               {
                  myDist = 1; myLevel = 1;
               }
               addUniqueDependency (predN, predNextBarrier,
                    TRUE_DEPENDENCY, CONTROL_TYPE, myDist, myLevel, 0);
            }
         }
      }
   }
   predNodes.clear ();
   
   // If I have any replicated node, check here if it got dependencies to the
   // next barrier node. If not, then add one.
   if (nextBarrier!=NULL && lastNode!=NULL)
   {
      // I have replicated node and there are barrier nodes.
      // check all dependencies outgoing from lastNode
      bool hasDep = false;
      OutgoingEdgesIterator oeit(lastNode);
      while ((bool)oeit)
      {
         if (!oeit->IsRemoved() && oeit->isSchedulingEdge())
         {
            Node *sinkNode = oeit->sink ();
            int eDistance = oeit->getDistance ();
            if ( (useLevel<nextBarrier->_level && 
                  sinkNode->_level<=nextBarrier->_level && 
                  eDistance==0) ||
                 (useLevel>nextBarrier->_level && 
                    ( (sinkNode->_level>useLevel && eDistance==0) ||
                      (sinkNode->_level<=nextBarrier->_level && eDistance==1)))
               )
            {
               hasDep = true;
               break;
            }
         }
         ++ oeit;
      }
      if (!hasDep)
      {
         int myDist = 0;
         int myLevel = 0;
         if (nextBarrier->_level < useLevel)
         {
            myDist = 1; myLevel = 1;
         }
         addUniqueDependency (lastNode, nextBarrier,
              TRUE_DEPENDENCY, CONTROL_TYPE, myDist, myLevel, 0);
      }
   }

   // clear the local map
   locMap.clear();
}
//--------------------------------------------------------------------------------------------------------------------

void
SchedDG::patternSearch (PatternGraph::Node* findNode, UIList& iterCrossed)
      throw (PatternFoundException)
{
   NodesIterator nit(*this);

   // start looking from each node (maybe not the most efficient thing)
   while ((bool)nit)
   {
      Node* nn = nit;
      if (nn->isInstructionNode())
      {
         iterCrossed.clear ();
         // make sure the entire source pattern is in one barrier interval.
         // Use barrierLevels data for this.
         int maxLevel = nextBarrierLevel[nn->_level];
         int backedges = 0;
         if (maxLevel>0 && maxLevel<nn->_level)
            backedges = 1;
         // compute how many iterations are crossed by the pattern match
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (9, 
            cerr << "PatternSearch: from node " << *nn << " on level " 
                 << nn->_level << ", with maxLevel=" << maxLevel 
                 << ", nextNodeId=" << nextNodeId << endl;
         )
#endif
         recursivePatternSearch (nn, findNode, iterCrossed, maxLevel, 
              backedges);
      }
      ++ nit;
   }
}

// find the source pattern in the dependency graph. 
// iterCrossed is a list that grows with the number of nodes matched. 
// The length of the iterCrossed list is equal to the number of edges
// identified in the source pattern (or number of nodes - 1)
void
SchedDG::recursivePatternSearch (Node* node, PatternGraph::Node* findNode, 
             UIList& iterCrossed, int maxLevel, int backedges)
      throw (PatternFoundException)
{
   // test if this node fits the pattern
   const InstructionClass &pic = findNode->getIClass();
   // they match if their characteristics are the same. The test for the bit width is looser.
   // An exact match is required only if the source instruction pattern specifies an explicit width
   if (node->makeIClass()!=pic || (pic.width!=max_operand_bit_width && pic.width!=node->bit_width) ||
      // also, if the node is of VECTOR type, test the vector length in bits (0 means auto detect vec length)
        (node->exec_unit==ExecUnit_VECTOR && pic.vec_width!=0 && 
               pic.vec_width!=(node->vec_len*node->bit_width)
        )
      ) 
      // it does not match
      return;
   
   // check if the instruction requires the use of the same general purpose 
   // register multiple times. 
   if (findNode->requiredRegisterCount() > 0)  // it does
   {
      // Iterate over the actual node's source registers and
      // count how many times each name appears.
      // Since the number of input general purpose registers is very small,
      // 1-4, do this in O(N^2) by checking each register in turn.
      RInfoList::iterator it, jt;
      bool foundReg = false;
      for (it=node->srcRegs.begin() ; it!=node->srcRegs.end() ; ++it)
         if (it->type != RegisterClass_MEM_OP)
         {
            int cnt = 1;
            jt = it;
            ++ jt;
            for ( ; jt!=node->srcRegs.end() ; ++jt)
               if (jt->type!=RegisterClass_MEM_OP && *it==*jt)
                  ++ cnt;
            if (cnt >= findNode->requiredRegisterCount())
            {
               foundReg = true;
               break;
            }
         }
      // did not find a register with the reuired multiplicity use
      if (!foundReg)
         return; 
   }
   findNode->SetActualNode(node);
   // at this point I know the current node fits the current position in the
   // pattern; find the successor edge
   PatternGraph::Edge* findEdge = findNode->FirstSuccessor();
   if (findEdge == NULL)  // no outgoing edge, this is the pattern end
      throw (PatternFoundException(iterCrossed));  // we found one
   
   bool exclusiveOutputMatching = findNode->ExclusiveOutputMatch();
   // if we are here it means we must match an edge now
   OutgoingEdgesIterator oem(node);   // iterate all children
   int eType = findEdge->getType();
   int eDir =  findEdge->getDirection();
        
   // if exclusiveOutputMatching is set, I have to check if there are other 
   // true dependency edges besides the matched edge.
   int numEdges = 0;
   if (exclusiveOutputMatching)
   {
      while ((bool)oem)
      {
         if (! oem->IsRemoved() && oem->getType()!=CONTROL_TYPE && 
                oem->getDirection()==TRUE_DEPENDENCY)
         {
            numEdges += 1;
         }
         ++ oem;
      }
      if (numEdges>1)  // node has multiple outgoing true dependencies
          return;
      oem.Reset();
   }
   
   while ((bool)oem)
   {
      if (! oem->IsRemoved() && eType==oem->getType() && 
               eDir==oem->getDirection() )
      {
         int oemDist = oem->getDistance();
         int sinkLevel = oem->sink()->_level;
         if (oemDist>=0 && (!maxLevel 
                || (oemDist==0 && (backedges>0 || sinkLevel<=maxLevel))
                || (oemDist==1 && backedges>0 && sinkLevel<=maxLevel) ))
         {
            // we found a matching edge
            findEdge->SetActualEdge (oem);
            // call recursively for the next node
            iterCrossed.push_back (oem->getDistance());
            recursivePatternSearch (oem->sink(), findEdge->sink(), 
                      iterCrossed, maxLevel, backedges-oemDist);
            iterCrossed.pop_back ();
         }
      }
      ++ oem;
   }
   // if we are here it means we did not find a pattern and we have to 
   // back track
}
//--------------------------------------------------------------------------------------------------------------------

// check if this edge is part of any cycle that has at least one node 
// scheduled. I found complicated cases (in a recurrence that multiplies
// complex numbers) where this condition results in a livelock, because of the
// butterfly dependencies create cycles that cross two iterations and contain
// most of the nodes. It is a bit hard to explain. Anyway, I will limit this
// test to cycles that cross one iteration only, unless the edge has only 
// cycles of multiple iterations in which case I will limit only to cycles of
// the minimum number of iterations. [DO NOT CONSIDER THIS ANYMORE, STILL 
// LIVELOCK IN THAT PROBLEMATIC CASE; or if the cycle that crosses multiple 
// iterations is more restrictive].
bool 
SchedDG::edgePartOfAnyFixedCycle (Edge *ee, int &cycId, int &niters)
{
   int min_num_iters = 0;
#if 0
   int most_restrictive_iters = 0;
   int most_restrictive_length = 0;
   bool most_restrictive_has_fixed = false;
#endif
   bool min_iters_has_fixed = false;
   if (ee->cyclesIds.empty())  // it is not part of a cycle
      return (false);
   // if edge has a super edge, it means it is part of a super-structure.
   // Find the top super edge, and check if that is part of a fixed cycle 
   // instead;
   Edge *topSE = ee;
   // Since I am looking only at the most restrictive cycles, I can check only
   // the first super-edge (in case an edge is associated with multiple superE)
   while (topSE->superE)
      topSE = topSE->superE;
   UISet::iterator sit = topSE->cyclesIds.begin ();
   for ( ; sit!=topSE->cyclesIds.end() ; ++sit)
   {
      Cycle* crtCyc = cycles[*sit];
      assert (crtCyc != NULL);
      if (crtCyc->fixedNodes>0 && crtCyc->iters<=1)
      {
         cycId = *sit;
         niters = crtCyc->iters;
         return (true);
      }
      if (crtCyc->iters>0)
      {
         if (min_num_iters==0 || crtCyc->iters<min_num_iters)
         {
            min_num_iters = crtCyc->iters;
            if (crtCyc->fixedNodes>0)
            {
               cycId = *sit;
               niters = crtCyc->iters;
               min_iters_has_fixed = true;
            }
            else
               min_iters_has_fixed = false;
         } else
            if (crtCyc->iters==min_num_iters && crtCyc->fixedNodes>0 && !min_iters_has_fixed)
            {
               cycId = *sit;
               niters = crtCyc->iters;
               min_iters_has_fixed = true;
            }
      }
#if 0
      if (crtCyc->iterLength>0)
      {
         if (most_restrictive_length==0 || crtCyc->iterLength>most_restrictive_length)
         {
            most_restrictive_length = crtCyc->iterLength;
            most_restrictive_iters = crtCyc->iters;
            if (crtCyc->fixedNodes>0)
               most_restrictive_has_fixed = true;
            else
               most_restrictive_has_fixed = false;
         } else
            if (crtCyc->iterLength==most_restrictive_length && crtCyc->fixedNodes>0)
               most_restrictive_has_fixed = true;
      }
#endif
   }
   if (min_iters_has_fixed /* || most_restrictive_has_fixed */)
      return (true);
   return (false);
}
//--------------------------------------------------------------------------------------------------------------------

bool 
SchedDG::edgePartOfAnyFixedSegment (Edge *ee)
{
   if (ee->cyclesIds.empty())  // it is not part of a cycle
      return (false);
   UISet::iterator sit = ee->cyclesIds.begin ();
   for ( ; sit!=ee->cyclesIds.end() ; ++sit)
   {
      Cycle* crtCyc = cycles[*sit];
      assert (crtCyc != NULL);
      if (crtCyc->isSegment() && crtCyc->fixedNodes > 0)
         return (true);
   }
   
   return (false);
}
//--------------------------------------------------------------------------------------------------------------------

// next 2 methods compute scheduling bounds and the flag mixed edges, but 
// compute also the values for edgesDownstream and edgesUpstream
bool
SchedDG::Node::computeLowerBound (ScheduleTime &lbSt, bool& mixedEdges, 
            bool superEdges)
{
   bool has_lower = false;
   bool has_fixed_edges = false;
   bool has_free_edges = false;
   // Find the lower bound
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (3, 
      cerr << "--> In computeLowerBound for " << *this << endl;
   )
#endif
   int edgesCount = 0;
   int minSchedId = 0;
   int freeid = 0, freeiters = 0;
   Edge *freeedge = 0;
   IncomingEdgesIterator lbit (this);
   while ((bool)lbit)
   {
      Edge *ee = (Edge*)lbit;
      Node *srcN = ee->source();
      Node *sinkN = ee->sink();
      if (!ee->IsRemoved() && ee->isSchedulingEdge())
//            || ee->isSuperEdge()) )
      {
         if (ee->source()->isScheduled())
         {
            if ( (superEdges && ee->isSuperEdge()) || 
                 (!superEdges && !ee->isSuperEdge()) )
               has_fixed_edges = true;
            ScheduleTime temp = srcN->schedTime + 
                                ee->getActualLatency();
            int ddist = ee->getDistance();
            if (ddist>0)
               temp %= -ddist;
            if (ddist==0 && (edgesCount<(srcN->edgesUpstream + 1)))
               edgesCount = srcN->edgesUpstream + 1;
            if (srcN->minSchedIdUpstream && (!minSchedId
                  || minSchedId>srcN->minSchedIdUpstream))
               minSchedId = srcN->minSchedIdUpstream;
            if (!minSchedId || minSchedId>srcN->scheduled)
               minSchedId = srcN->scheduled;
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (4, 
               cerr << "--> " << *ee << " gives low bound " << temp << endl;
            )
#endif
            if (! has_lower)
            {
               lbSt = temp;
               has_lower = true;
            }
            else
               if (lbSt<temp)
                  lbSt = temp;
         }
         else
            // self cycle edges do not count
            // a free edge is problematic only if it is part of a cycle
            // that has another vertex scheduled (not an adjaicent 
            // vertex obviously).
            if ( ( (superEdges && ee->isSuperEdge()) || 
                   (!superEdges && !ee->isSuperEdge()) ) &&
                 srcN != sinkN && _in_cfg->edgePartOfAnyFixedCycle(ee, freeid, freeiters) )
            {
               freeedge = ee;
               has_free_edges = true;
            }
      }
      
      if (!ee->IsRemoved() && ee->isSuperEdge() && ee->source()->isScheduled())
      {
            ScheduleTime temp = srcN->schedTime + 
                                ee->getLatency() + ee->usedSlack;
            int ddist = ee->getDistance();
            if (ddist>0)
               temp %= -ddist;
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (4, 
               cerr << "--> " << *ee << " gives low bound " << temp << endl;
            )
#endif
            if (! has_lower)
            {
               lbSt = temp;
               has_lower = true;
            }
            else
               if (lbSt<temp)
                  lbSt = temp;
      }
      ++ lbit;
   }
   if (has_fixed_edges && has_free_edges)
   {
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (3, 
      cerr << "** computeLowerBound for " << *this << " discovered mix edges. "
           << "Free edge " << *freeedge << " is part of cycle " << freeid 
           << " with " << freeiters << " iterations." << endl;
//      cerr << "### " << *(_in_cfg->cycles[freeid]) << " ####" << endl;
   )
#endif
      mixedEdges = true;
   }
   if (mixedEdges)
      edgesUpstream = 0;
   else
      edgesUpstream = edgesCount;
   minSchedIdUpstream = minSchedId;
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (3, 
      cerr << "--> Returning from computeLowerBound.";
      if (has_lower)
         cerr << " Found lower bound " << lbSt << ", edgesUpstream="
              << edgesUpstream << ", minSchedIdUpstream=" 
              << minSchedIdUpstream << ", mixedEdges=" << mixedEdges << endl;
      else
         cerr << " Did not find a lower bound." << endl;
   )
#endif
   return (has_lower);
}
//--------------------------------------------------------------------------------------------------------------------

bool
SchedDG::Node::computeUpperBound (ScheduleTime &ubSt, int &upperSlack,
           bool &only_bypass_edges, bool& mixedEdges, bool superEdges)
{
   bool has_upper = false;
   bool has_fixed_edges = false;
   bool has_free_edges = false;
   only_bypass_edges = true;
   upperSlack = 0;
   ScheduleTime nonBypassST;
   
   // Find the upper bound
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (3, 
      cerr << "==> In computeUpperBound for " << *this << endl;
   )
#endif
   int edgesCount = 0;
   int minSchedId = 0;
   int freeid = 0, freeiters = 0;
   Edge *freeedge = 0;
   OutgoingEdgesIterator ubit (this);
   while ((bool)ubit)
   {
      Edge *ee = (Edge*)ubit;
      Node *srcN = ee->source ();
      Node *sinkN = ee->sink ();
      if (!ee->IsRemoved() && ee->isSchedulingEdge())
//             || ee->isSuperEdge()) )
      {
         if (ee->sink()->isScheduled())
         {
            if ( (superEdges && ee->isSuperEdge()) || 
                 (!superEdges && !ee->isSuperEdge()) )
               has_fixed_edges = true;
            ScheduleTime temp = sinkN->schedTime -
                             ee->getLatency();  // use min latency as I did
                                                // not choose a template yet
            int ddist = ee->getDistance();
            if (ddist>0)
               temp %= ddist;
            if (ddist==0 && (edgesCount<(sinkN->edgesDownstream+1)))
               edgesCount = sinkN->edgesDownstream + 1;
            if (sinkN->minSchedIdDownstream && (!minSchedId
                  || minSchedId>sinkN->minSchedIdDownstream))
               minSchedId = sinkN->minSchedIdDownstream;
            if (!minSchedId || minSchedId>sinkN->scheduled)
               minSchedId = sinkN->scheduled;
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (4, 
               cerr << "==> " << *ee << " gives upper bound " << temp << endl;
            )
#endif
            if (! has_upper)
            {
               ubSt = temp;
               has_upper = true;
            }
            else
               if (ubSt>temp)
                  ubSt = temp;
            
            if (!ee->isBypassEdge())
            {
               if (only_bypass_edges)
               {
                  only_bypass_edges = false;
                  nonBypassST = temp;
               } else
                  if (nonBypassST > temp)
                     nonBypassST = temp;
            }
         }
         else
            // self cycle edges do not count
            // a free edge is problematic only if it is part of a cycle
            // that has another vertex scheduled (not an adjaicent 
            // vertex obviously).
            if ( ( (superEdges && ee->isSuperEdge()) || 
                   (!superEdges && !ee->isSuperEdge()) ) &&
                 srcN != sinkN && _in_cfg->edgePartOfAnyFixedCycle(ee, freeid, freeiters) )
            {
               freeedge = ee;
               has_free_edges = true;
            }
      }
      
      if (!ee->IsRemoved() && ee->isSuperEdge() && ee->sink()->isScheduled())
      {
            ScheduleTime temp = sinkN->schedTime - 
                                ee->getLatency() - ee->usedSlack;
            int ddist = ee->getDistance();
            if (ddist>0)
               temp %= ddist;
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (4, 
               cerr << "==> " << *ee << " gives upper bound " << temp << endl;
            )
#endif
            if (! has_upper)
            {
               ubSt = temp;
               has_upper = true;
            }
            else
               if (ubSt>temp)
                  ubSt = temp;
      }
      ++ ubit;
   }
   if (has_upper && !only_bypass_edges)
   {
      upperSlack = nonBypassST - ubSt;
      assert (upperSlack >= 0);
   }
   if (has_fixed_edges && has_free_edges)
   {
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (3, 
      cerr << "** computeUpperBound for " << *this << " discovered mix edges. "
           << "Free edge " << *freeedge << " is part of cycle " << freeid 
           << " with " << freeiters << " iterations." << endl;
//      cerr << "### " << *(_in_cfg->cycles[freeid]) << " ####" << endl;
   )
#endif
      mixedEdges = true;
   }
   if (mixedEdges)
      edgesDownstream = 0;
   else
      edgesDownstream = edgesCount;
   minSchedIdDownstream = minSchedId;
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (3, 
      cerr << "==> Returning from computeUpperBound.";
      if (has_upper)
      {
         cerr << " Found upper bound " << ubSt << ", edgesDownstream="
              << edgesDownstream << ", minSchedIdDownstream="
              << minSchedId << ", mixedEdges=" << mixedEdges;
         if (!only_bypass_edges) 
            cerr << " and upperSlack=" << upperSlack;
      }
      else
         cerr << " Did not find an upper bound.";
      cerr << endl;
   )
#endif
   return (has_upper);
}
//--------------------------------------------------------------------------------------------------------------------

// Next two routines were used in an heuristic in case two long recurrences 
// were connected by some acyclic dependecy or by a much shorter recurrence.
// But now I prioritize scheduling based on how much remains to be scheduled
// from a cycle/path. Thus, at some point is normal even for a short 
// reccurence to be partially scheduled before the long recurrence is 
// completely mapped. I will disable this. I will also modify these two 
// routines to return (false) always. I should make them to return true only
// if one node is not part of a cycle, while the other is.
bool 
SchedDG::Node::operator << (const Node& n2)
{
   return (hasCycles==0 && n2.hasCycles==3);
   if (_in_cfg->resourceLimited)
      return ((resourceScore*2) < n2.resourceScore);
   else
      return ((longestCycle<<1) < n2.longestCycle);
}
//--------------------------------------------------------------------------------------------------------------------

bool 
SchedDG::Node::operator >> (const Node& n2)
{
   return (hasCycles==3 && n2.hasCycles==0);
   if (_in_cfg->resourceLimited)
      return ((resourceScore*2) > n2.resourceScore);
   else
      return ((longestCycle<<1) > n2.longestCycle);
}
//--------------------------------------------------------------------------------------------------------------------

bool 
SchedDG::Node::InOrderDependenciesSatisfied(bool initial, Edge **dep)
{
   bool has_deps = false;
   
   // we should have the in_order index set for all instruction nodes
   assert (in_order > 0);
   IncomingEdgesIterator ieit (this);
   while ((bool)ieit && !has_deps)
   {
      Edge *ee = (Edge*)ieit;
      if (!ee->IsRemoved() && ee->isSchedulingEdge())
      {
         Node *srcN = ee->source();
         assert(srcN->in_order > 0);
         if (srcN!=this && srcN->in_order<=this->in_order && 
                   (initial || !srcN->isScheduled()))
         {
            has_deps = true;  // we found unsatisfied dependencies;
            if (dep)
               *dep = ee;
         }
      }
      ++ ieit;
   }
   // check also outgoing edges??
   // No, let's stick with incoming dependencies only
#if 0
   OutgoingEdgesIterator oeit (this);
   while ((bool)oeit && !has_deps)
   {
      Edge *ee = (Edge*)oeit;
      if (!ee->IsRemoved() && ee->isSchedulingEdge())
      {
         Node *sinkN = ee->sink();
         assert(sinkN->in_order > 0);
         if (sinkN!=this && sinkN->in_order<=this->in_order &&
                   (initial || !sinkN->isScheduled()))
         {
            has_deps = true;  // we found unsatisfied dependencies;
            if (dep)
               *dep = ee;
         }
      }
      ++ oeit;
   }
#endif
   return (! has_deps);
}
//--------------------------------------------------------------------------------------------------------------------

bool
SchedDG::create_schedule_main (int& unit, Node* &fNode, Edge* &fEdge, 
           int& extraLength, int& numTries, int& segmentId)
{
   int i;
   int numInstNodes = 0;

   // must sort the nodes according to their properties (dependencies, 
   // resources). Use a priority queue, and use different priorities if the
   // path is resource limited or dependency limited.
   // Adjust priorities based on limpMode being On or Off.
   if (limpModeScheduling)
   {
      nodesQ = new NodePrioQueue (
              SchedDG::SortNodesByDependencies(heavyOnResources, 
                      resourceLimited, true));
      edgesQ = 0;
      mixedQ = 0;
   } else
   {
      nodesQ = new NodePrioQueue (SchedDG::
                     SortNodesByDependencies(heavyOnResources, resourceLimited));
      edgesQ = new EdgePrioQueue (SchedDG::
                     SortEdgesByDependencies(avgNumIters>1.0, heavyOnResources,
                     resourceLimitedScore));
      mixedQ = new EdgePrioQueue (SchedDG::
                     SortEdgesByDependencies(avgNumIters>1.0, heavyOnResources,
                     resourceLimitedScore));
   }
   fflush (stdout);
   fflush (stderr);

#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (8, cerr << "\n==========\nAll Nodes:\n";)
#endif

   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      if (nn->isInstructionNode())  // cfg_top has type == -1
      {
         ++ numInstNodes;

#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (8, cerr << "Found " << (*nn) << endl;)
#endif

         nn->resetSchedule (schedule_length);
         if (limpModeScheduling)
         {
            // in limp mode I insert all nodes that have no dependencies on
            // nodes with lower in_order indices
            if (nn->InOrderDependenciesSatisfied(true))
            {
               nodesQ->push(nn);
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (8, cerr << "Pushing node to nodeQ in limp mode." << endl;)
#endif
            }
         } else  // else, in normal mode
         {
            // do not queue nodes that are part of super-structure;
            // the ends of a super-structure are not marked as part of the 
            // structure proper
            if (! nn->structureId)
            {
               nodesQ->push(nn);
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (8, cerr << "Pushing node to nodeQ." << endl;)
#endif
            }
         }
      }
      ++ nit;
   }
   // there must be at least one node which is not part of any super-structure
   assert (! nodesQ->empty());

#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (8, cerr << "\n==========\nAll Edges:\n";)
#endif

   EdgesIterator egit(*this);
   while ((bool)egit)
   {
      Edge *ee = egit;
      if (! ee->IsRemoved() && (ee->isSchedulingEdge() || 
               ee->isSuperEdge()) )
      {

#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (8, cerr << (*ee) << endl;)
#endif

         ee->resetSchedule ();
      }
      ++ egit;
   }
   for (i=1 ; i<nextCycleId ; ++i)
      if (cycles[i] != NULL)
      {
         cycles[i]->resetScheduleInfo();
      }
   cycleDag->resetSchedule ();
   
   nextScheduleId = 1;
   globalTries = 0;
   
   longestCycles.clear ();
   longCyclesLeft = false;
   if (! heavyOnResources)
   {
      UiUiMap::iterator uit = setsLongestCycle.begin();
      for ( ; uit!=setsLongestCycle.end() ; ++uit)
      {
         longestCycles.push_back (UIPair (uit->first, uit->second));
      }
      if (longestCycle>1)
         longCyclesLeft = true;
   
      std::sort (longestCycles.begin(), longestCycles.end(), 
                UIPair::OrderDescByKey2());
   }
#if 0
   PairArray::iterator pait = longestCycles.begin ();
   for ( ; pait!=longestCycles.end() ; ++pait)
   {
      fprintf (stderr, "Found longestCycles entry for id %d, longestCycle %d\n",
          pait->first, pait->second);
   }
#endif

   try
   {
      create_schedule( );
   } 
   catch (UnsuccessfulSchedulingException ussex)
   {
      if (nodesQ)
         delete nodesQ;
      if (edgesQ)
         delete edgesQ;
      if (mixedQ)
         delete mixedQ;
      unit = ussex.Unit();
      fNode = dynamic_cast<SchedDG::Node*>(ussex.Node());
      fEdge = dynamic_cast<SchedDG::Edge*>(ussex.Edge());
      extraLength = ussex.ExtraLength();
      numTries = ussex.NumTries();
      segmentId = ussex.SegmentId();  // do I still need this?
      
      return (false);
   } 
   catch (SuccessfulSchedulingException ssex)
   {
      if (numInstNodes >= nextScheduleId)  // hmm, not all the nodes have been scheduled
      {
         fprintf(stderr, "ERROR: Path %s has %d instruction nodes, nextScheduleId=%d and schedule_length=%d. Cannot I catch a break?\n",
               pathId.Name(), numInstNodes, nextScheduleId, schedule_length);
         
         // Iterate over all the nodes and print the ones scheduled and unscheduled
         NodesIterator nnit(*this);
         while ((bool)nnit)
         {
            Node *nn = nnit;
            if (nn->isInstructionNode())  // cfg_top has type == -1
            {
               cerr << " - " << *nn << " in_order=" << nn->in_order;
               if (!nn->isScheduled())
               {
                  Edge *eg = 0;
                  bool is_ready = nn->InOrderDependenciesSatisfied(false, &eg);
                  if (is_ready)
                     cerr << " is READY.";
                  else
                     cerr << " Has Deps, like: " << *eg;
               }
               cerr << endl;
            }
            ++ nnit;
         }
         
         // nextScheduleId starts counting from 1, while numInstNodes counts from 0;
         assert(numInstNodes == (nextScheduleId-1));
      }
      
      if (nodesQ)
         delete nodesQ;
      if (edgesQ)
         delete edgesQ;
      if (mixedQ)
         delete mixedQ;
      return (true);
   }
   
   return (true);
}
//--------------------------------------------------------------------------------------------------------------------

// startNode is the node that could not be scheduled; I must unschedule all
// nodes that contribute to its lower bound, as well as the nodes upstream 
// of them;
void 
SchedDG::undo_schedule_upstream (Node* startNode) 
{
   // should this unschedule super edges too? Better not
   // Also, do not unschedule loop carried dependencies or backedges
   NodeList processNodes;
   processNodes.push_back (startNode);
   while (! processNodes.empty ())
   {
      Node* node = processNodes.front ();
      processNodes.pop_front ();
      IncomingEdgesIterator edit(node);
      while ((bool)edit)
      {
         Edge* ee = (Edge*)edit;
         if (!ee->IsRemoved() && ee->isSchedulingEdge() && ee->getLevel()==0)
         {
            Node* srcNode = ee->source();
            // do not unschedule barrier nodes
            if (srcNode->isInstructionNode() && srcNode->isScheduled() &&
                   !srcNode->isBarrierNode() )
            {
               // this is a predecessor node that contributes to my lower bound
               unscheduleNode (srcNode);
               nodesQ->push(srcNode);
               processNodes.push_back (srcNode);
            }
         }
         ++ edit;
      }
   }
   
   requeueAllEdges ();
}
//--------------------------------------------------------------------------------------------------------------------

// startNode is the node that could not be scheduled; I must unschedule all
// nodes that contribute to its upper bound, as well as the nodes downstream 
// of them;
void 
SchedDG::undo_schedule_downstream (Node* startNode)
{
   // should this unschedule super edges too? Better not
   // Also, do not unschedule loop carried dependencies or backedges
   NodeList processNodes;
   processNodes.push_back (startNode);
   while (! processNodes.empty ())
   {
      Node* node = processNodes.front ();
      processNodes.pop_front ();
      OutgoingEdgesIterator edit(node);
      while ((bool)edit)
      {
         Edge* ee = (Edge*)edit;
         if (!ee->IsRemoved() && ee->isSchedulingEdge() && ee->getLevel()==0)
         {
            Node* destNode = ee->sink();
            if (destNode->isInstructionNode() && destNode->isScheduled() &&
                  !destNode->isBarrierNode() )
            {
               // this is a successor node that contributes to the upper bound
               unscheduleNode (destNode);
               nodesQ->push(destNode);
               processNodes.push_back (destNode);
            }
         }
         ++ edit;
      }
   }
   
   requeueAllEdges ();
}
//--------------------------------------------------------------------------------------------------------------------

void 
SchedDG::unscheduleNode (Node *nn)
{
   if (nn->scheduled == 0)
      return;
   PairList &selectedUnits = nn->allocatedUnits;
   int vwidth = 0;
   if (nn->exec_unit == ExecUnit_VECTOR)
      vwidth = nn->vec_len * nn->bit_width;
   Instruction *ii = mach->InstructionOfType((InstrBin)nn->type, nn->exec_unit, vwidth, 
           nn->exec_unit_type, nn->bit_width);
   assert (ii);
   ii->deallocateTemplate (schedule_template, schedule_length, 
                nn->schedTime.ClockCycle(), selectedUnits);
   nn->scheduled = 0;
   // decrement fixedNodes for all cycles
   UISet::iterator sit;
   for (sit=nn->cyclesIds.begin() ; sit!=nn->cyclesIds.end() ; ++sit)
   {
      register Cycle* crtCyc = cycles[*sit];
//      if (crtCyc->isCycle() || !nn->isBarrierNode())
      if (crtCyc->isCycle())
         crtCyc->fixedNodes -= 1;
      // sanity check
      assert (crtCyc->fixedNodes >= 0);
   }
}
//--------------------------------------------------------------------------------------------------------------------

void
SchedDG::undo_partial_node_scheduling (Node *nn, EdgeList *newEdges, 
           EdgeList::iterator *endit)
{
   // first traverse the list of edges and undo any allocated slack for
   // them
   EdgeList::iterator elit = newEdges->begin ();
   for ( ; elit!=*endit ; ++elit)
   {
      Edge *crtE = *elit;
      if (!crtE->isScheduled())
         continue;

      // I need to unschedule this edge
      if (crtE->hasCycles == 3)
      {
         UISet::iterator sit = crtE->cyclesIds.begin();
         for ( ; sit!=crtE->cyclesIds.end() ; ++sit)
         {
            Cycle* crtCyc = cycles[*sit];
            assert (crtCyc != NULL);
//            if (crtCyc->isCycle())
//            {
               crtCyc->remainingLength += crtE->minLatency;
               crtCyc->remainingEdges += 1;
//            }
         }
      }
      if (!crtE->isSuperEdge())
      {
         if (crtE->usedSlack>0)
         {
            int drez = deallocateSlack (crtE);
            assert (! drez);
         }
         crtE->resetSchedule();
      } else
         crtE->markUnScheduled ();
   }
   
   // finally, deallocate the resources used by this node
   PairList &selectedUnits = nn->allocatedUnits;
   int vwidth = 0;
   if (nn->exec_unit == ExecUnit_VECTOR)
      vwidth = nn->vec_len * nn->bit_width;
   Instruction *ii = mach->InstructionOfType((InstrBin)nn->type, nn->exec_unit, vwidth, 
                 nn->exec_unit_type, nn->bit_width);
   assert (ii);
   ii->deallocateTemplate (schedule_template, schedule_length, 
               nn->schedTime.ClockCycle(), selectedUnits);
   nn->scheduled = 0;
}
//--------------------------------------------------------------------------------------------------------------------

void
SchedDG::requeueAllEdges ()
{
   UISet::iterator sit;

   // Traverse all edges, and unschedule those that do not have both ends
   // scheduled. Make sure to deallocate slack info if necessary.
   //edgesQ->clear();
   while (! edgesQ->empty())
      edgesQ->pop();
   while (! mixedQ->empty())
      mixedQ->pop();
   
   EdgeList mixedReadyEdges;
   EdgeList readyEdges;
   EdgesIterator edit(*this);
   while ((bool)edit)
   {
      Edge* ee = (Edge*)edit;
      if ( !ee->IsRemoved() && (ee->isSchedulingEdge() 
                     || ee->isSuperEdge()) )
      {
         Node* srcNode = ee->source();
         Node* destNode = ee->sink();
         if (ee->isScheduled() && 
               (!srcNode->isScheduled() || !destNode->isScheduled()) )
         {
            // I need to unschedule this edge
            if (ee->hasCycles == 3)
               for (sit=ee->cyclesIds.begin() ; sit!=ee->cyclesIds.end() ; ++sit)
               {
                  Cycle* crtCyc = cycles[*sit];
                  assert (crtCyc != NULL);
//                  if (crtCyc->isCycle())
//                  {
                     crtCyc->remainingLength += ee->minLatency;
                     crtCyc->remainingEdges += 1;
//                  }
               }
            if (!ee->isSuperEdge())
            {
               if (ee->usedSlack>0)
               {
                  int drez = deallocateSlack (ee);
                  assert (! drez);
               }
               ee->resetSchedule();
               // I have to mark all its super-edges as unscheduled as well.
               // Should I do this explicitly, or is it guaranteed that I 
               // cannot unschedule a node internal to a structure without
               // unscheduling at least one of the structure's ends?
               // I have to check this somehow and assert. Perhaps, in 
               // unscheduleNode, if the node has structureId set, get its
               // super-edge and assert that at least one end of the super-
               // structure must be unscheduled as well (if both are scheduled).
            } else
               ee->markUnScheduled ();
         }
         if (!ee->isScheduled() && !ee->isSuperEdge() &&
               (srcNode->isScheduled() || destNode->isScheduled()) )
            readyEdges.push_back (ee);
      }
      ++ edit;
   }
   
   EdgeList::iterator elit;
   for (elit=readyEdges.begin() ; elit!=readyEdges.end() ; ++elit)
   {
      Edge* crtE = *elit;
      computeMaxRemaining (crtE);
      // add the edge in the edges prio queue
      edgesQ->push (crtE);
   }
   
   // Edges from mixed queue will be considered after all edges from ready 
   // queue are scheduled. The value set for maxRemaining will most likely
   // be meaningless by then, or it is possible some of the edges are 
   // inserted into the ready queue by another path. Therefore, I will not 
   // spend the time to set maxRemaining to the correct current value, 
   // but I will set it equal to the longest cycle value.
   for (elit=mixedReadyEdges.begin() ; elit!=mixedReadyEdges.end() ; ++elit)
   {
      Edge* crtE = *elit;
      crtE->maxRemaining = crtE->longestCycle;
      // add the edge in the mixed edges prio queue
      mixedQ->push (crtE);
   }
}
//--------------------------------------------------------------------------------------------------------------------

void 
SchedDG::undo_schedule (Node* newNode, bool aggressive)
{
   // flag aggressive means that all nodes that are not part of cycles should
   // be deallocated.
   //
   // limitSId = newNode->scheduled
   // for all nodes in graph
   //    unschedule nodes with Id > limitSId

   // Nodes scheduled more recently than the threshold Id will be unscheduled.
   int limitSId = newNode->scheduled;  // this is the threshold Id. 

   //nodesQ->clear();
   // clear is not a method of priority queue. I have to remove all the
   // elemnets one by one.
   while (! nodesQ->empty())
   {
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (8, 
         Node *nn = nodesQ->top();
         cerr << "+++ Popping node " << *nn << endl;
      )
#endif
      nodesQ->pop();
   }
   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      if (nn->isInstructionNode() && (nn->scheduled>=limitSId ||
             (aggressive && nn->hasCycles==0)) )
      {
         unscheduleNode (nn);
      }
      if (nn->isInstructionNode() && nn->scheduled==0 && 
                 ! nn->structureId)
      {
         nodesQ->push(nn);
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (8, 
            cerr << "+++ Pushing node " << *nn << endl;
      )
#endif
      }
      ++ nit;
   }
   
   requeueAllEdges ();
   
   nextScheduleId = limitSId;
}
//--------------------------------------------------------------------------------------------------------------------

// create_schedule() is the meat and potatoes of the scheduling algorithm
bool
SchedDG::create_schedule ()
       throw (UnsuccessfulSchedulingException, SuccessfulSchedulingException)
{
   int i;

   bool repeatNode = false;
   const int* unitsIndex = mach->getUnitsIndex();
   do
   {
      Edge *e;
      Node *newNode = 0;
      Node *oldNode;
      int unit;
      int localTries = 0;
      bool isSrcNode = false;
      int trycase;

#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (3, 
         cerr << "\n1 -> In create_schedule, schedule_length=" << schedule_length << endl;
      )
#endif
         
      if (repeatNode)
         repeatNode = false;
      else if (limpModeScheduling)
      {
         e = NULL;
         newNode = NULL;
         oldNode = NULL;
         unit = DEFAULT_UNIT;  // Default value for unit.
         localTries = 0;
         isSrcNode = false;
         trycase = TC_NO_TRYCASE;
         
         // In Limp Mode, I have ready Nodes in nodeQ.
         // Just select the top one and schedule it. Easy.
         while (! nodesQ->empty ())
         {
            newNode = nodesQ->top ();
            nodesQ->pop ();
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (8, 
               cerr << "-->> Popping node (limp) " << *newNode << endl;
            )
#endif
            if (! newNode->isScheduled ())
               break;
         }
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (4, 
            if (newNode && !newNode->isScheduled())
               cerr << "3a ->> In create_schedule (limp), found node " << *newNode << endl;
         )
#endif
         if (newNode==NULL || newNode->isScheduled() ) 
            // all nodes have been scheduled. Coolness.
            throw (SuccessfulSchedulingException());
      } else
      {
         e = NULL;
         newNode = NULL;
         oldNode = NULL;
         unit = DEFAULT_UNIT;  // Default value for unit.
         localTries = 0;

         isSrcNode = false;
         trycase = TC_NO_TRYCASE;
         
         // use the higher level DAG first. Check if there are long cycles
         // that have not been fixed yet. 
         // Hmm, I do not know if some of the fixed cycles are not in fact
         // short :(. Need to change the strat a bit. TODO ******
         if (longCyclesLeft)
         {
            unsigned int halfLongestCycle = longestCycle >> 1;
            PairArray::iterator pait = longestCycles.begin ();
            for ( ; pait!=longestCycles.end() ; ++pait)
            {
               // cycles are sorted descending from largest to smallest
               // so stop at first not fixed cycle;
//               fprintf (stderr, "Check isSetFixed for id %d, longestCycle %d\n",
//                      pait->first, pait->second);
               if (! cycleDag->isSetFixed (pait->first))
                  break;
            }
            // test of this cycle set has a length at least half as large
            // as the longest cycle. "half" is arbitrary and may change.
            if (pait->second >= halfLongestCycle)
            {
               // build a nodes queue with the nodes part of this cycle set
               // and choose one of them
               Ui2NSMap::iterator nsmapit = cycleNodes.find (pait->first);
               assert (nsmapit != cycleNodes.end());
               NodePrioQueue *tempNQ = new NodePrioQueue (SchedDG::
                        SortNodesByDependencies (resourceLimited));
               NodeSet &theNodes = nsmapit->second;
               NodeSet::iterator nsit = theNodes.begin ();
               for ( ; nsit!=theNodes.end() ; ++nsit)
                  // add only nodes that are not part of a super-structure
                  if ((*nsit)->structureId==0)
                     tempNQ->push (*nsit);
               
               // I changed the above loop to queue only nodes that are not
               // part of any structure. If it is possible to have cycles
               // inside a structure, than no node would be queued. You must
               // analyze the problematic input code and decide on the best
               // solution if the assert fails.
               assert (! tempNQ->empty());
               
               // once all these nodes are added, select an unscheduled node
               // all nodes in this queue should be unscheduled
               while (! tempNQ->empty ())
               {
                  newNode = tempNQ->top ();
                  tempNQ->pop ();
                  assert (! newNode->isScheduled ());
                  assert (newNode->hasCycles == 3);
                  break;
               }
               assert (newNode && !newNode->isScheduled());
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (4, 
                     cerr << "1a ->> In create_schedule, found cycleSet " 
                          << pait->first << " node " << *newNode << endl;
               )
#endif
               delete (tempNQ);
               cycleDag->fixSet (pait->first);
               if (cycleDag->getNumFixedSets() >= numCycleSets)
                  longCyclesLeft = false;
            } else  // there are no more long cycles
               longCyclesLeft = false;
         }
         
         if (newNode == NULL)
         {
            while (! edgesQ->empty ())
            {
               e = edgesQ->top ();
               edgesQ->pop ();
               if (! e->isScheduled ())
               {
                  // if edge is part of super-structure with both ends scheduled,
                  // select the edge only from one end of the superstructure.
                  // Schedule from the end with the lower scheduleId.
                  //
                  // gmarin, 09/19/2013: I changed the logic a bit. Now
                  // I consider loop independent paths through SCCs (Strongly 
                  // Connected Components) as the main criteria when scheduling 
                  // edges part of cycles, in order to avoid the dreaded DEADLOCK
                  // with diamond like structures. This Super-Edge check
                  // makes less sense with the new approach. I will keep it
                  // for now. Remains to be seen if this approach is robust 
                  // in the presence of cycles that cross multiple iterations, 
                  // and if it produces schedules as tight as the old approach.
                  // Right now, I use macros to select the new approache vs. 
                  // the old one. One solution is to compile both versions, 
                  // using a condition variable to select between them. For 
                  // example, try first the old approach (if it creates 
                  // shorter schedules). Then, if I detect DEADLOCK, select 
                  // the new approach after resetting the schedule length to 
                  // the minimum value (??). Hmm, I think I need to reset the 
                  // schedule extra time information in the path's TimeAccount
                  // as well in that case.
                  Edge *supE = e;
                  // find the top super Edge
                  while(supE->superE)
                     supE = supE->superE;
                     
                  if (supE!=e && supE->source()->scheduled &&
                         supE->sink()->scheduled)  // both superE ends are scheduled
                  {
                     if (supE->source()->scheduled < supE->sink()->scheduled)
                     {
                        if (!e->source()->scheduled)
                           continue;
                     } else
                     {
                        if (!e->sink()->scheduled)
                           continue;
                     }
                  }
                  
                  /* The test below is not working properly. I will probably have
                   * to change how I schedule edges part of cycles. Instead of
                   * checking the distance remaining in each cycle, I will just
                   * check the distance and edges remaining to the bottom of the 
                   * SCC. I need this solution to avoid scheduling deadlock with
                   * diamond-like structures. It is not exactly critical-path
                   * based scheduling with the new approach ...
                   */
#if 0
                  // if the unscheduled node is part of an SCC, check that
                  // it does not have edges on the same side that are part
                  // of the SCC and are both scheduled and not.
                  if (!e->sink()->scheduled && e->sink()->getSccId())
                  {
                     // the sink node must be scheduled and it is part of an SCC
                     // check all its incoming edges
                     Node *nn = e->sink();
                     int sccId = nn->getSccId();
                     IncomingEdgesIterator iit(nn);
                     bool has_sched_li = false, has_unsched_li = false;
                     bool has_sched_lc = false, has_unsched_lc = false;
                     // stop once I see at least one scheduled and one unscheduled edges
                     // of the same distance (loop indepent or loop carried)
                     while ((bool)iit && (!has_sched_li || !has_unsched_li) &&
                              (!has_sched_lc || !has_unsched_lc))
                     {
                        Edge *ie = (Edge*)iit;
                        if (ie->source()->getSccId()==sccId)
                        {
                           if (ie->source()->scheduled)
                           {
                              if (ie->ddistance == 0)
                                 has_sched_li = true;
                              else
                                 has_sched_lc = true;
                           }
                           else
                           {
                              if (ie->ddistance == 0)
                                 has_unsched_li = true;
                              else
                                 has_unsched_lc = true;
                           }
                        }
                        ++ iit;
                     }
                     if ((has_sched_li && has_unsched_li) || (has_sched_lc && has_unsched_lc))
                     {
                        // has both, skip this edge for now
#if VERBOSE_DEBUG_SCHEDULER
                        DEBUG_SCHED (4, 
                           cerr << "1b -> In create_schedule, skipping edge in regular queue " 
                                << *e << " that leads to node with both scheduled and unscheduled ";
                           if (has_sched_li && has_unsched_li)
                              cerr << "loop independent";
                           else
                              cerr << "loop carried";
                           cerr << " predecessors part of SCC " << sccId << endl;
                        )
#endif
                        continue;
                     }
                  }
                  if (!e->source()->scheduled && e->source()->getSccId())
                  {
                     // the sink node must be scheduled and it is part of an SCC
                     // check all its incoming edges
                     Node *nn = e->source();
                     int sccId = nn->getSccId();
                     OutgoingEdgesIterator oit(nn);
                     bool has_sched_li = false, has_unsched_li = false;
                     bool has_sched_lc = false, has_unsched_lc = false;
                     // stop once I see at least one scheduled and one unscheduled edges
                     // of the same distance (loop indepent or loop carried)
                     while ((bool)oit && (!has_sched_li || !has_unsched_li) &&
                              (!has_sched_lc || !has_unsched_lc))
                     {
                        Edge *oe = (Edge*)oit;
                        if (oe->sink()->getSccId()==sccId)
                        {
                           if (oe->sink()->scheduled)
                           {
                              if (oe->ddistance == 0)
                                 has_sched_li = true;
                              else
                                 has_sched_lc = true;
                           }
                           else
                           {
                              if (oe->ddistance == 0)
                                 has_unsched_li = true;
                              else
                                 has_unsched_lc = true;
                           }
                        }
                        ++ oit;
                     }
                     if ((has_sched_li && has_unsched_li) || (has_sched_lc && has_unsched_lc))
                     {
                        // has both, skip this edge for now
#if VERBOSE_DEBUG_SCHEDULER
                        DEBUG_SCHED (4, 
                           cerr << "1c -> In create_schedule, skipping edge in regular queue " 
                                << *e << " that leads to node with both scheduled and unscheduled ";
                           if (has_sched_li && has_unsched_li)
                              cerr << "loop independent";
                           else
                              cerr << "loop carried";
                           cerr << " successors part of SCC " << sccId << endl;
                        )
#endif
                        continue;
                     }
                  }
#endif

                  // if it is an edge part of a cycle, recompute its 
                  // priority first and push it back if it is changed
                  if (e->hasCycles==3 && e->prioTimeStamp!=nextScheduleId
                        && !computeMaxRemaining (e))
                     edgesQ->push (e);
                  else
                     break;
               }
            }
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (4, 
               if (e && !e->isScheduled())
                  cerr << "2a -> In create_schedule, found edge in regular queue " 
                       << *e << endl;
            )
#endif

#if !USE_SCC_PATHS
            if (e && e->hasCycles==3 && e->maxRemaining>e->longestCycle)
            {
               assert (!"What the heck! MaxRemaining > LongestCycle!");
            }
#endif

            if (e == NULL || e->isScheduled())   // regular edges prio queue is empty
            // try the mixed edges queue
            {
               while (! mixedQ->empty ())
               {
                  e = mixedQ->top ();
                  mixedQ->pop ();
                  if (! e->isScheduled ())
                     break;
               }
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (4, 
                  if (e && !e->isScheduled())
                     cerr << "2b -> In create_schedule, found edge in mixed queue " 
                          << *e << endl;
               )
#endif
            }

            if (e == NULL || e->isScheduled()) 
            // both edges queues are empty, pick a node
            {
               e = NULL;
               while (! nodesQ->empty ())
               {
                  newNode = nodesQ->top ();
                  nodesQ->pop ();
#if VERBOSE_DEBUG_SCHEDULER
                  DEBUG_SCHED (8, 
                     cerr << "-->> Popping node " << *newNode << endl;
                  )
#endif
                  if (! newNode->isScheduled ())
                     break;
               }
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (4, 
                  if (newNode && !newNode->isScheduled())
                     cerr << "3a ->> In create_schedule, found node " << *newNode << endl;
               )
#endif
               if (newNode==NULL || newNode->isScheduled() ) 
                  // all nodes have been scheduled. Coolness.
                  throw (SuccessfulSchedulingException());
            } 
            else   // we selected next edge to schedule, now schedule it
            {
               // if the edge was not marked as scheduled, then exactly one end 
               // should be scheduled and one unscheduled. This is because after I 
               // schedule a new node, I check all incident edges for correctness
               // and I mark as scheduled any edge that has both ends scheduled.
               // Therefore it should not be possible to have an edge with both
               // ends scheduled, but still marked unscheduled. Assert!
               int srcSchedId = e->source()->scheduleId();
               int sinkSchedId = e->sink()->scheduleId();
               assert (srcSchedId>0 || sinkSchedId>0);
               assert (srcSchedId==0 || sinkSchedId==0);
               if (srcSchedId == 0)
               {
                  newNode = e->source ();
                  isSrcNode = true;
                  oldNode = e->sink ();
               }
               else
               {
                  newNode = e->sink ();
                  oldNode = e->source ();
               }
            
               // trycase 1 is when the oldNode is part of a cycle much shorter
               // then the new node (I will use a fraction of 1/2). It captures
               // cases when the oldNode is not part of any cycle (cycleLen is 0)
               // Fix: added the test that this path is dependency limited.
               // If it is resource limited, it does not make sense to unschedule
               // intermidiate nodes (with low resource score) because dependencies
               // will still need to hold. It is just that all cycles are short
               // relative to the high level of instruction parallelism.
               // mgabi 1/03/2006: Actually I will disable this trycase completly.
               // More recently I use a different dynamic priority based on how much
               // of a cycle/path remains to be scheduled. As a result it can 
               // perfectly happen to have a shorter recurrence scheduled interleaved
               // with a longer recurrence.
               // 03/09/2007: I see this trycase was resurrected at some point.
               
               // Always create a TRYCASE if we reach a cycle from an acyclic path
               // I've encountered multiple cases where one SCC (strongly connected 
               // component is reached from more than one acyclic path, creating
               // diamond like structures that result in a scheduling deadlock.
//               if (! limpModeFindCycle)  // FIXME
               {
                  if (e->hasCycles<3 && newNode->hasCycles==3)
                     trycase = TC_SCC_REACHED_FROM_ACYCLIC;
#if 1
                  else
                  if (resourceLimitedScore<1.5 && ((*oldNode) << (*newNode)))
                     trycase = TC_SHORT_BEFORE_LONG_CYCLE;
#endif
               } 
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (4, 
                  cerr << "3b ->> In create_schedule, processed edge, newNode is "
                       << *newNode << ", oldNode is " << *oldNode << ", trycase="
                       << trycase << endl;
               )
#endif
            }
         }  // if newNode == NULL
      } // if repeatNode

      int ss, pp, numTries;
      bool concentric = false;
      int maxExtraLength = -1;
      ScheduleTime *newTime = &(newNode->schedTime);
      int result = computeBoundsForNode (newNode, e, ss, pp, concentric, 
                numTries, *newTime, maxExtraLength, unit, trycase, 
                isSrcNode);
      if (result == (-1))
      {
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (2, 
            cerr << "computeBoundsForNode for " << *newNode << " returned -1. Skipping??" << endl;
         )
#endif
         continue;
      }
      ScheduleTime startTime(*newTime);

#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (4, 
         fprintf (stderr, "5 -> In create_schedule, ss=%d, pp=%d, concentric=%d, numTries=%d, time(iter,cycle)=(%d,%d)\n",
                    ss, pp, concentric, numTries, newTime->Iteration(), newTime->ClockCycle());
         fprintf (stderr, "6 -> In create_schedule, node %d, type %d, longestCycle %d, resourceScore %f\n",
                    newNode->id, newNode->type, newNode->longestCycle, newNode->resourceScore);
      )
#endif
      
      // take into account unit restrictions when allocating an instruction
      // used in call to findAndUseTemplate
      const RestrictionList* restrictions = mach->Restrictions();
      for (i=0 ; i<numTries ; ++i)
      {
         // If I am in trycase BARRIER_SLACK and this is the last valid
         // position to try for this node, I should revert to normal mode
         // such that I do not come back to this node on an exception.
         if (trycase==TC_BARRIER_SLACK && (i+1)>=numTries)
            trycase = TC_NO_TRYCASE;
         int startTIdx = 0;
#if 0
         int maxExtraLength = -1;
         if (has_upper && !only_bypass_edges)
         {
            maxExtraLength = ubSt - *newTime + upperSlack;
            assert (maxExtraLength >= 0);
         }
#endif
         int tempUnit = 0;
         // check if any template works in this place
         PairList &selectedUnits = newNode->allocatedUnits;
         int vwidth = 0;
         if (newNode->exec_unit == ExecUnit_VECTOR)
            vwidth = newNode->vec_len * newNode->bit_width;
         Instruction *ii = mach->InstructionOfType((InstrBin)newNode->type, 
                      newNode->exec_unit, vwidth, newNode->exec_unit_type, 
                      newNode->bit_width);
         assert (ii);
         int templIdx = ii->findAndUseTemplate (schedule_template, schedule_length, 
                  newTime->ClockCycle(), maxExtraLength, startTIdx, 
                  restrictions, unitsIndex, selectedUnits, tempUnit);
         
         if (templIdx>=0)   // found a template, check all incident edges
         {
            // record the template index selected
            newNode->templIdx = templIdx;

            // it is possible now to find a template, but some incident
            // edge fails being either too long or too short.
            int rez = processIncidentEdges (newNode, templIdx, i, unit);
            if (rez > 0)
            {
               // it means there was a failure of sorts. For now the failure
               // can be only an edge which is longer than the space between
               // the two ends. The result (rez) represent how many more 
               // cycles are needed. Anyway, it also means that current node
               // was not part of a cycle and I unscheduled the nodes of one
               // bound. I should just try rescheduling this node again in
               // this case without rechecking that bounds are different.
               repeatNode = true;
               break;
            }
            
            // do not use backtracking in limp mode
            if (trycase==TC_NO_TRYCASE || limpModeScheduling)  // this is not a try case
               break;    // exit from the inner loop that tries to find 
                         // another position; schedule next node iteratively
            // otherwise, we have a try case
            // schedule next node recursively
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (3, 
               fprintf (stderr, "8 -> In create_schedule, trycase=%d, must go recursive\n",
                       trycase);
            )
#endif
            try
            {
               // call current routine recursively
               create_schedule ( );
               // I should not return here ever, because I return only
               // through exceptions. Assert.
               assert (!"Should not be here. I return from recursion \
                only through exception. Remove assert if it is not so.");
            } catch (UnsuccessfulSchedulingException ussex)
            {
               // re-throw the exception if this catch does not match the 
               // exception mask, or if the id of current node is larger 
               // than the id of the projected point of failure.
               if ( ussex.ScheduleId()<newNode->scheduled ||
                    !(ussex.ExceptionMask() & trycase) )
                  throw;  // re-throw it further
               
#if 0   // I think Barrier Slack is not going to be used any more in the 
        // version with super-structures
               // check if this is a valid case of TC_BARRIER_SLACK
               if (trycase == TC_BARRIER_SLACK)
               {
                  Node *failNode = dynamic_cast<Node*>(ussex.Node());
                  Edge *failEdge = dynamic_cast<Edge*>(ussex.Edge());
                  Edge *failSuperE = NULL;
                  if (failEdge)
                     failSuperE = failEdge->superE;
                  if (failSuperE==NULL)
                     failSuperE = failNode->superE;
                  // I should find a super edge because I can be in BARRIER
                  // SLACK mode only if there are at least two barrier nodes.
                  assert (failSuperE != NULL);
                  int srcLevel = failSuperE->source()->_level;
                  int sinkLevel = failSuperE->sink()->_level;
                  int thisLevel = newNode->_level;
                  int failLevel = failNode->_level;
                  
                  // if low bounding node that goes upwards, then rethrow;
                  if ( ((failLevel<sinkLevel && 
                          (thisLevel<failLevel || thisLevel>sinkLevel)) ||
                        (sinkLevel<thisLevel && thisLevel<failLevel)
                       ) && pp>0)
                     throw;
                  // if upper bounding node that goes downwards, then rethrow;
                  if ( ((srcLevel<failLevel && 
                          (thisLevel<srcLevel || thisLevel>failLevel)) ||
                        (failLevel<thisLevel && thisLevel<srcLevel)
                       ) && pp<0)
                     throw;
                  
               }
#endif
               ++ localTries;
               if (localTries > SCHED_MAXIMUM_LOCAL_BACKTRACK)  
                  // if no other re-try allowed at this position,
                  throw;      // re-throw further
               ++ globalTries;
               if (globalTries > SCHED_MAXIMUM_GLOBAL_BACKTRACK) 
                  // no more re-try allowed globally for this schedule
                  throw (UnsuccessfulSchedulingException (0, 
                         ussex.ExceptionMask(), ussex.Unit(),
                         ussex.Node(), ussex.Edge(), ussex.ExtraLength(), 
                         ussex.NumTries(), ussex.SegmentId()) );

#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (3, 
                  fprintf (stderr, " =>>> In create_schedule, CATCH AND PROCESS UnsuccessfulSchedulingException trycase=%x, use.mask=%x, nodeId=%d, use.Id=%d, use.unit=%d, localTries=%d, globalTries=%d\n",
                       trycase, ussex.ExceptionMask(), newNode->scheduled,
                       ussex.ScheduleId(), ussex.Unit(), localTries,
                       globalTries);
               )
#endif
               
               // if I am here, I need to unschedule all nodes that have been
               // scheduled after this node. Also, for special try cases I may
               // need to unschedule the graph reachable from oldNode.
               // A lot of work to do here (better create another routine)
               // pass as arguments: schedule template and current schedule 
               // length to deallocate resources, the nodes queue and edges 
               // queue because they need to be updated, schedule Id of 
               // current node, pointer to oldNode (can be NULL) to un-schedule
               // the smaller cycle.
               // I need to mark current node as unscheduled also, because I
               // attempt to schedule it again at the same position or a 
               // different one. 
               bool to_continue = false;
               bool to_break = false;
               int result = 0;
               switch (trycase)
               {
                  case TC_SCC_REACHED_FROM_ACYCLIC:
                     undo_schedule(newNode, false);
                     concentric = false;
                     // try scheduling first the node on which scheduling failed.
                     // This node must have an adjaicent edge which is acyclic
                     // and the node must be part of the SCC. In fact, it should
                     // be the first node part of the SCC.
                     newNode = static_cast<Node*>(ussex.Node());
                     newTime = &(newNode->schedTime);
                     result = computeBoundsForNode (newNode, /* e */ 0, ss, pp, 
                           concentric, numTries, *newTime, maxExtraLength, 
                           unit, trycase, isSrcNode, true);
                     assert (result != (-1));

#if VERBOSE_DEBUG_SCHEDULER
                     DEBUG_SCHED (4, 
                        fprintf (stderr, "* -> In TC_SCC_REACHED_FROM_ACYCLIC, ss=%d, pp=%d, concentric=%d, numTries=%d, time(iter,cycle)=(%d,%d)\n",
                                   ss, pp, concentric, numTries, newTime->Iteration(), newTime->ClockCycle());
                     )
#endif
                     startTime = *newTime;
                     
                     oldNode = NULL;
                     trycase = TC_CONCENTRIC_TRYCASE;
                     i = 0;      // reset i;
                     // I am switching which node I try to schedule, so
                     // I reset the count to 0.
                     to_continue = true;
                     
                     break;
                     
                  case TC_SHORT_BEFORE_LONG_CYCLE:
                     undo_schedule (newNode, true);   // oldNode);
                     concentric = false;
                     result = computeBoundsForNode (newNode, e, ss, pp, 
                           concentric, numTries, *newTime, maxExtraLength, 
                           unit, trycase, isSrcNode, true);
                     assert (result != (-1));
                  
                     oldNode = NULL;
                     trycase = TC_CONCENTRIC_TRYCASE;
                     i = 0;      // reset i;
                     // let me try the same position after I unscheduled 
                     // the short cycle should I reset newTime to its 
                     // initial value?
                     to_continue = true;
                     break;
                     
                  case TC_NODE_WITH_MIXED_EDGES:
                     newNode->setHasMixedEdges();
                     undo_schedule (newNode, false);   // NULL);
                     to_break = true;
                     break;
                     
                  default:
                     undo_schedule (newNode, false);    // oldNode);
                     break;
               }  // switch (trycase)
               
               // because I cannot break from a switch statement, I will set 
               // some flags and I will use the loop control flow instructions
               // here
               if (to_continue)
                  continue;
               if (to_break)
                  break;

            } // catch (UnsuccessfulSchedulingException)
         }  // if templIdx > 0
         
         // Try another position
         *newTime += ss*pp;
         maxExtraLength -= ss*pp;
#if 1
         if (maxExtraLength < 0)
            maxExtraLength = 0;
#else
         if (maxExtraLength >= 0)
         {
            maxExtraLength -= ss*pp;
            assert (maxExtraLength>=0 || (i+1)==numTries);
         }
#endif
         unit = tempUnit;
         if (concentric)
         {
            ss += 1; pp = -pp;
         }
      }  // for i=0 to numTries
      
      // test if we exit b/c of unsuccess or b/c of a break (success)
      // if unsuccess, use the unit value set by findAndUseTemplate, and 
      // the current nextScheduleId such that it is caught by the most recent
      // try ... catch structure.
      if (i==numTries)
      {
         // unschedule nodes only if they are not part of cycles. Do not allow
         // unscheduling of cycles even if this is a resource heavy path 
         // (1.5 was not that heavy anyway) because it breaks things and isn't helpful
         if (!newNode->numUnschedules && newNode->hasCycles<3
//         (newNode->hasCycles<3 || (resourceLimitedScore>1.5 && 
//                newNode->minSchedIdUpstream != newNode->minSchedIdDownstream))
              && numTries<(int)schedule_length-2)
         {
            // if this node is not part of a segment that has its two ends
            // scheduled, then try to remove one of the bounds by unscheduling
            // some nodes

            assert (newNode->minSchedIdUpstream != 
                         newNode->minSchedIdDownstream);
            cerr << "No available slot found: UNSCHEDULE the nodes ";
            // use the minSchedId info instead of the min number of edges
            // upstream and downstream
            if (newNode->minSchedIdUpstream > newNode->minSchedIdDownstream)
            {
               cerr << "upstream of current node" << endl;
               undo_schedule_upstream (newNode);
            } else
            {
               cerr << "downstream of current node" << endl;
               undo_schedule_downstream (newNode);
            }
#if 0
            if (newNode->edgesUpstream == newNode->edgesDownstream)
               cerr << "(upstream==downstream) ";
            // once I remove a bound, I should mark it that way.
            if (newNode->edgesUpstream < newNode->edgesDownstream)
            {
               cerr << "upstream of current node" << endl;
               undo_schedule_upstream (newNode);
            } else
            {
               cerr << "downstream of current node" << endl;
               undo_schedule_downstream (newNode);
            }
#endif
            newNode->numUnschedules += 1;

            // after unscheduling the nodes, recompute the bounds
            // if the new startng location is different than the old one, or
            // the new numTries is larger, then attempt to reschedule current
            // node again. Otherwise throw an exception.

            concentric = false;
            result = computeBoundsForNode (newNode, e, ss, pp, 
                     concentric, numTries, *newTime, maxExtraLength, 
                     unit, trycase, isSrcNode, true);
            assert (result != (-1));
         
            // 'i' holds the old value of 'numTries'.
            if (*newTime!=startTime || numTries > i)
            {
               repeatNode = true;
               continue;
            }
         }

         // Otherwise throw an exception
         //
         // collect the ids of all nodes limiting this node, separately for
         // the lower and the upper bound. Find the minimum id for each bound.
         // Next, throw UnsuccessfulSchedulingException with the maximum of 
         // the two minimum ids;
         // find the maximum id of the nodes restricting the bounds of this 
         // node
         // I need to find also the id of the segment limiting the node.
         // Barrier nodes have a total ordering, but a scheduling edge may be
         // part of more than one segment if it is preceeded by a join, or 
         // followed by a fork. I want to find the shortest segment surraunding
         // the edge. Shortest segment is unique, so I will detect this info
         // when I construct the segments and store it in the Edge class.
         //
         // If numTries == schedule_length, then do not bother with finding
         // the maxim id. There is absolutely no available slot left in the
         // entire path. This happens only when we have assymetric templates
         // and some instruction types were allocated using more in demand
         // units that are needed by other instruction types with fewer 
         // available templates. This happens because the scheduling priority
         // is not really resource aware, while the algorithm for computing
         // the minimum schedule length due to resources, gives higher 
         // priority to instructions with fewer choices.
         int theId = 0;
         if (numTries < (int)schedule_length-2)
         {
            int minLowId = 0;
            int minUpId = 0;
            ScheduleTime lbTemp, ubTemp;
            IncomingEdgesIterator ieit (newNode);
            while ((bool)ieit)
            {
               Edge *ee = (Edge*)ieit;
               if (!ee->IsRemoved() && (ee->isSchedulingEdge() || ee->isSuperEdge()) &&
                        ee->source()->isScheduled())
               {
                  ScheduleTime temp = ee->source()->schedTime + 
                                      ee->getActualLatency();
                  int ddist = ee->getDistance();
                  if (ddist>0)
                     temp %= -ddist;
                  if (minLowId==0 || lbTemp<temp)
                  {
                     lbTemp = temp;
                     minLowId = ee->source()->scheduled;
                  } else
                     if (lbTemp-temp==0 && minLowId>ee->source()->scheduled)
                        minLowId = ee->source()->scheduled;
               }
               ++ ieit;
            }
            OutgoingEdgesIterator oeit (newNode);
            while ((bool)oeit)
            {
               Edge *ee = (Edge*)oeit;
               if (!ee->IsRemoved() && (ee->isSchedulingEdge() || ee->isSuperEdge()) &&
                        ee->sink()->isScheduled())
               {
                  ScheduleTime temp = ee->sink()->schedTime -
                                ee->getLatency();  // use min latency as I did
                                                   // not choose a template yet
                  int ddist = ee->getDistance();
                  if (ddist>0)
                     temp %= ddist;
                  if (minUpId==0 || ubTemp>temp)
                  {
                     ubTemp = temp;
                     minUpId = ee->sink()->scheduled;
                  } else
                     if (ubTemp-temp==0 && minUpId>ee->sink()->scheduled)
                        minUpId = ee->sink()->scheduled;
               }
               ++ oeit;
            }
            assert (minLowId>0 || minUpId>0);
            if (minLowId<minUpId)
               theId = minUpId;
            else
               theId = minLowId;
         }
         
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (4, 
            fprintf (stderr, "PathId %s, schedule_length %d, num scheduled nodes %d, numTries %d, THROW UnsuccessfulSchedulingException with schedId %d\n",
                  pathId.Name(), schedule_length, nextScheduleId-1, numTries, theId);
         )
#endif
         throw (UnsuccessfulSchedulingException (theId, 
               TC_ALL_EXCEPTIONS, 
               unit, newNode, e, 1, numTries));
      }
         
      // I added already all edges with only one end scheduled
      // nothing more to do here
      
   } while (!nodesQ->empty() || repeatNode || (!limpModeScheduling && !edgesQ->empty()));
   // should not get here, right?
   // Hmm, I never got here with the Sparc version. However, with the new x86 version
   // I found a very simple graph (4 nodes, only 1 self-cycle on one instruction), which
   // gets here and I cannot find anything wrong with that. I should throw the exception
   // from here so we unwind the stack. Ok, this situation happends when the graph contains
   // a single super-structure where only one actual node is not an internal node. Only the 
   // non-internal node is added to nodesQ, thus nodesQ stays empty after the only node is popped.
   throw (SuccessfulSchedulingException());
   
   assert (! "Should not be here because I exit through a throw above.");
   return (true);
}
//--------------------------------------------------------------------------------------------------------------------

int 
SchedDG::computeBoundsForNode (Node *newNode, Edge *e, int &ss, int &pp, 
          bool &concentric, int &numTries, ScheduleTime &newTime, 
          int &maxExtraLength, int &unit, int &trycase, bool isSrcNode, 
          bool relaxed)
       throw (UnsuccessfulSchedulingException)
{
   // if we are here, it means we have an unscheduled node in newNode
   // compute a lower time bound using its incoming edges, and an
   // upper bound using its outgoing edges.
   ScheduleTime lbSt, ubSt;
   bool only_bypass_edges = true;
   int upperSlack = 0;
   
   // when computing the lower and upper bound, test also if this node has
   // mixed edges incoming or outgoing. By mixed I mean both edges that 
   // have the other end scheduled (and cause a limit), and edges that do
   // not have the other end scheduled (cause no restriction). Such cases
   // can lead to deadlock in scheduling if there is a diamond structure
   // and some of the inner nodes in the diamond are scheduled after both 
   // ends of the diamond have been scheduled (increasing the schedule 
   // length does not help in this case). Thus, I detect these cases and
   // I mark them with a trycase. When I catch a failure due to such a 
   // trycase, I will not schedule the node with mixed edges until all the
   // other ready to be scheduled edges were scheduled.
   // NEW: mgabi - 11/11/2005: I will improve the test to catch such 
   // problematic cases. When I catch one, I will not schedule that edge, 
   // but place it in the mixed edges queue, and I will try another edge.
   // The way to catch these cases is to recompute the dynamic priority
   // for the involved edges. This is needed only for edges that are 
   // part of cycles, acyclic edges do not have this problem.
   // I will change computeLowerBound and computeUpperBound to check 
   // these cases. A better solution (implemented now) is to check if the
   // free edge is part of a cycle that has at least another node 
   // scheduled. Only in such cases I will set mixedEdges flags. If both
   // bounds have mixed edges, then I will discard this edge (without 
   // scheduling it) and pick another node. I will reach the current new
   // node on a different path eventually. This is cheaper than recomputing
   // the edge's dynamic priority
   bool mixedEdgesLower = false;
   bool mixedEdgesUpper = false;
 
   bool has_lower = newNode->computeLowerBound(lbSt, mixedEdgesLower, 
             (e!=NULL && e->isSuperEdge()) );
   bool has_upper = newNode->computeUpperBound(ubSt, upperSlack, 
             only_bypass_edges, mixedEdgesUpper, 
             (e!=NULL && e->isSuperEdge()) );
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (4, 
      fprintf (stderr, "4 -> In create_schedule, has_lower %d, has_upper %d\n",
                 has_lower, has_upper);
      if (has_lower)
         cerr << "4l -> In create_schedule, low bound " << lbSt << endl
              << "   lowerMixedEdges=" << mixedEdgesLower << endl;
      if (has_upper)
         cerr << "4u -> In create_schedule, upper bound " << ubSt << endl
              << "   upperMixedEdges=" << mixedEdgesUpper << endl;
   )
#endif
   
   if (limpModeScheduling)
   {
      if (has_lower && has_upper && lbSt>ubSt)
      {
         int overTime = lbSt - ubSt;
         // I must try to produce a better limiting unit.
         unit = DEFAULT_UNIT;
         IncomingEdgesIterator ieit (newNode);
         while ((bool)ieit)
         {
            Edge *ee = (Edge*)ieit;
            if (!ee->IsRemoved() && ee->isSchedulingEdge() &&
                     ee->source()->isScheduled())
            {
               ScheduleTime temp = ee->source()->schedTime + 
                                   ee->getActualLatency();
               int ddist = ee->getDistance();
               if (ddist>0)
                  temp %= -ddist;
               // temp should be <= lbSt, because lbSt was the maximum of them
               if (lbSt-temp < overTime)  // it is a constraining edge
               {
                  if (unit==DEFAULT_UNIT && ee->source()->unit!=DEFAULT_UNIT)
                     unit = ee->source()->unit;
               }
            }
            ++ ieit;
         }
         OutgoingEdgesIterator oeit (newNode);
         while ((bool)oeit)
         {
            Edge *ee = (Edge*)oeit;
            if (!ee->IsRemoved() && ee->isSchedulingEdge() &&
                     ee->sink()->isScheduled())
            {
               ScheduleTime temp = ee->sink()->schedTime -
                             ee->getLatency();  // use min latency as I did
                                                // not choose a template yet
               int ddist = ee->getDistance();
               if (ddist>0)
                  temp %= ddist;
               // temp should be >= ubSt, because ubSt was the minimum of them
               if (temp-ubSt < overTime)  // it is a constraining edge
               {
                  if (unit==DEFAULT_UNIT && ee->sink()->unit!=DEFAULT_UNIT)
                     unit = ee->sink()->unit;
               }
            }
            ++ oeit;
         }
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (3, 
            cerr << "-=- Path " << pathId.Name() << ", schedule_length=" << schedule_length
                 << " newNode " << *newNode << " has lb=" << lbSt << ", ub=" << ubSt
                 << ", overTime=" << overTime << ", unit=" << unit << endl;
         )
#endif
         throw (UnsuccessfulSchedulingException (1, 
               TC_ALL_EXCEPTIONS, unit, newNode, e, overTime));
      } else
      {
         // in limp mode, we always attempt to schedule from the lower bound
         if (!has_lower && !has_upper)
         {
            newTime.setTime (0,0);
            ss = 1; pp = 1;
            numTries = schedule_length;
            concentric = true;
         } else
         {
            // if the lower bound is defined, then we start from it.
            // Otherwise, we start from the upper one (it must be defined if we are here)
            if (has_lower)
            {
               newTime = lbSt;
               ss = 1; pp = 1;
            } else
            {
               assert (has_upper);
               newTime = ubSt;
               ss = 1; pp = -1;
            }
            if (has_lower && has_upper)
               // try scheduling until we find a good position, even if we are 
               // outside our known range. I want to find how much do I need to 
               // increase the schedule length to guarantee a successful scheduling
               // of this node.
               // Using schedule_length may cause the algorithm to fail later on because it
               // hasn't been written to deal with such a situation. It expects that all searched
               // positions are valid.
               numTries = (ubSt - lbSt) + 1;
            else
               numTries = schedule_length;
         }
         // no point in trying more positions than the schedule length
         if (numTries > schedule_length)
            numTries = schedule_length;
         return (0);
      }
   }
   
   // test if the lower bound is greater than the upper bound
   int countBadCase = 0;
   while (has_lower && has_upper && lbSt>ubSt)
   {
      // this cannot happen for a 'relaxed' case: a case were the bounds were
      // computed without this problem and then the conditions were relaxed
      // even more by unscheduling other nodes. So we should not see this 
      // situation in a relaxed case.
      assert (!relaxed);
      
      // this should not be a super edge; I think this case whould not
      // occur at all, but it definetly should not be a super edge
      // 03/09/2007 mgabi: NOT SURE about the next two asserts now. I am
      // not going to consider super-edges for scheduling. Now super-edges
      // represent super-structures, but I always replace them with the
      // edges that form the super-structure.
#if 0
      assert (!e->isSuperEdge());
      // and the newNode is not a barrier node
      assert (!newNode->isBarrierNode());
#endif
      
      ++ countBadCase;
      // unschedule nodes only if they are not part of cycles
      if (countBadCase==1 && newNode->hasCycles<3
//            (newNode->hasCycles<3 || (resourceLimitedScore>1.5 && 
//            newNode->minSchedIdUpstream != newNode->minSchedIdDownstream))
          && !newNode->numUnschedules)
      {
         // find if current node is upstream or downstream of the first 
         // scheduled node, and unschedule the nodes that contribute to the
         // bound on the side opposed from the starting node.
         // check node levels to determine the position of current node
         // relative to the starting node. This may not be always right.
         // Think a bit more about it.
         // Each node contains two more accumulators that record the distance
         // (# edges) to the furthest known scheduled node in each direction:
         // edgesUpstream and edgesDownstream

         assert (newNode->minSchedIdUpstream != 
                       newNode->minSchedIdDownstream);
         cerr << "No available slot found: UNSCHEDULE the nodes ";
         // use the minSchedId info instead of the min number of edges
         // upstream and downstream
         if (newNode->minSchedIdUpstream > newNode->minSchedIdDownstream)
         {
            cerr << "upstream of current node" << endl;
            undo_schedule_upstream (newNode);
         } else
         {
            cerr << "downstream of current node" << endl;
            undo_schedule_downstream (newNode);
         }
         newNode->numUnschedules += 1;
         
         // after unscheduling the nodes, recompute the bounds
         // it is not enough to assume that one bound does not exist,
         // because I may have loop carried dependencies that are not 
         // removed during the unschedule process. For same reason, I 
         // should recompute both bounds, because a carried dependency 
         // from one of the removed nodes, can change the other bound too.
         has_lower = newNode->computeLowerBound(lbSt, mixedEdgesLower, 
                 (e!=NULL && e->isSuperEdge()) );
         has_upper = newNode->computeUpperBound(ubSt, upperSlack, 
                 only_bypass_edges, mixedEdgesUpper, 
                 (e!=NULL && e->isSuperEdge()) );
         continue;
      } else
      {
         // collect the ids of all nodes limiting this node, separately for
         // the lower and the upper bound. Find the minimum id for each bound.
         // Next, throw UnsuccessfulSchedulingException with the maximum of 
         // the two minimum ids;
         // find the maximum id of the nodes restricting the bounds of this node
         int overTime = lbSt - ubSt;
         int minLowId = 0;
         int minUpId = 0;
         // determine if any of the bounds is caused by an acyclic edge that connects
         // two cycles. If yes, it means that the issue is having multiple acyclic 
         // paths reaching these two strongly connected components, which creates a
         // diamond like structure. I have to unschedule all the nodes of the SCC and
         // try scheduling this node first.
         bool sccHasLowAcyclicLimit = false;
         bool sccHasHighAcyclicLimit = false;
         IncomingEdgesIterator ieit (newNode);
         while ((bool)ieit)
         {
            Edge *ee = (Edge*)ieit;
            if (!ee->IsRemoved() && ee->isSchedulingEdge() &&
                     ee->source()->isScheduled())
            {
               ScheduleTime temp = ee->source()->schedTime + 
                                   ee->getActualLatency();
               int ddist = ee->getDistance();
               if (ddist>0)
                  temp %= -ddist;
               // temp should be <= lbSt, because lbSt was the maximum of them
               if (lbSt-temp < overTime)  // it is a constraining edge
               {
                  if (minLowId==0 || minLowId>ee->source()->scheduled)
                     minLowId = ee->source()->scheduled;
                  if (unit==DEFAULT_UNIT && ee->source()->unit!=DEFAULT_UNIT)
                     unit = ee->source()->unit;
                  // test if this node is part of a SCC and the edge is not
                  if (newNode->hasCycles==3 && ee->hasCycles<3)
                  {
                     // I think the edge must belong to an acyclic path connecting
                     // two SCCs, it cannot be a free acyclic edge. We would not have 
                     // scheduled it before a cyclic node, would we?
                     assert (ee->hasCycles>0);
                     sccHasLowAcyclicLimit = true;
                  }
               }
            }
            ++ ieit;
         }
         OutgoingEdgesIterator oeit (newNode);
         while ((bool)oeit)
         {
            Edge *ee = (Edge*)oeit;
            if (!ee->IsRemoved() && ee->isSchedulingEdge() &&
                     ee->sink()->isScheduled())
            {
               ScheduleTime temp = ee->sink()->schedTime -
                             ee->getLatency();  // use min latency as I did
                                                // not choose a template yet
               int ddist = ee->getDistance();
               if (ddist>0)
                  temp %= ddist;
               // temp should be >= ubSt, because ubSt was the minimum of them
               if (temp-ubSt < overTime)  // it is a constraining edge
               {
                  if (minUpId==0 || minUpId>ee->sink()->scheduled)
                     minUpId = ee->sink()->scheduled;
                  if (unit==DEFAULT_UNIT && ee->sink()->unit!=DEFAULT_UNIT)
                     unit = ee->sink()->unit;
                  // test if this node is part of a SCC and the edge is not
                  if (newNode->hasCycles==3 && ee->hasCycles<3)
                  {
                     // I think the edge must belong to an acyclic path connecting
                     // two SCCs, it cannot be a free acyclic edge. We would not have 
                     // scheduled it before a cyclic node, would we?
                     if (! ee->hasCycles)
                     {
                        cerr << "Found edge " << *ee << " with no cycles, which leads to "
                             << "node " << *newNode << " that is part of cycles. Why?" << endl;
                        assert (ee->hasCycles>0);
                     }
                     sccHasHighAcyclicLimit = true;
                  }
               }
            }
            ++ oeit;
         }
         // we cannot have both LowAcyclic and HighAcyclic limits at the same time, right?
         assert(!sccHasLowAcyclicLimit || !sccHasHighAcyclicLimit);
         
         int theId;
         if (minLowId<minUpId)
            theId = minUpId;
         else
            theId = minLowId;
         
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (2, 
            fprintf (stderr, "PathId %s, schedule_length %d, num scheduled nodes %d, THROW UnsuccessfulSchedulingException with schedId %d\n",
                  pathId.Name(), schedule_length, nextScheduleId-1, theId);
         )
#endif
         if (sccHasLowAcyclicLimit || sccHasHighAcyclicLimit)
            throw (UnsuccessfulSchedulingException (nextScheduleId, 
                  TC_SCC_REACHED_FROM_ACYCLIC | TC_CONCENTRIC_TRYCASE, 
                  unit, newNode, e, overTime));
         else
            throw (UnsuccessfulSchedulingException (theId, 
                  TC_ALL_EXCEPTIONS, unit, newNode, e, overTime));
      }
   }
   
   
   // If I did not find either lower, nor upper bound, then try scheduling 
   // this node at iteration 0, cycle 0. Next, try adjacent points further
   // and further from the center: 0, +1, -1, +2, -2, ...
   if (!has_lower && !has_upper)
   {
      newTime.setTime (0,0);
      ss = 1; pp = 1;
      numTries = schedule_length;
      concentric = true;
   } else
   {
      // if both bounds are defined, start scheduling from the bound
      // that does not have mixed edges (if this is the case) or from the
      // bound related to the edge that lead to this node (LB for a sink 
      // node) if both bounds have equivalently mixed edges.
      // If all existing bounds have also mixed edges, then discard the 
      // current edge and select another one. I should reach current node
      // on a different path.
      // gmarin, 03/14/2014: something very wrong happens with dense graphs 
      // when cycle discovery goes in limp mode (we do not have all the cycles). 
      // I do not understand the problem well, but if we are in 
      // limpModeFindCycle, then ignore the mixed edges flags. 
      // If we trigger the scheduling DEADLOCK, we have a fallback scheduling
      // algorithm that will hopefully work in all cases???
      if ( (!has_lower || mixedEdgesLower) && 
           (!has_upper || mixedEdgesUpper) && !limpModeFindCycle )  // no valid bound
      {
         // if neither bound exists, that case should be caught above and
         // not here
         assert (has_lower || has_upper);
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (4, 
            if (e!=NULL)
               cerr << "4f -> In create_schedule, DISCARD edge " 
                    << *e << endl;
         )
#endif
         return (-1);
      }
      
      // if this is a barrier node and there are at least two barrier nodes
      // in this path, then check if this barrier node has some slack 
      // with respect to its cycles, all cycles - not just the super ones.
      int maxSlack = -1;
      
      // we should not attempt to schedule any super-edges. But check if this
      // node has a super-edge (possibly a hierarchy of super-edge), find
      // the inner most super-edge that may impose a restriction on the 
      // schedule
      assert (e==NULL || !e->isSuperEdge());

      if (has_lower && (!has_upper ||
             (!mixedEdgesLower && (mixedEdgesUpper || !isSrcNode)) ||
             // this should trigger only in limpModeFindCycle ...
             (mixedEdgesLower && mixedEdgesUpper && !isSrcNode)  )
         )
      {
         newTime = lbSt;
         ss = 1; pp = 1;
      } else
      {
         assert (has_upper);
         assert (!mixedEdgesUpper || limpModeFindCycle);
         newTime = ubSt;
         ss = 1; pp = -1;
      }
      if (has_lower && has_upper)
         // try scheduling until we find a good position, even if we are 
         // outside our known range. I want to find how much do I need to 
         // increase the schedule length to guarantee a successful scheduling
         // of this node.
         // Using schedule_length may cause the algorithm to fail later on because it
         // hasn't been written to deal with such a situation. It expects that all searched
         // positions are valid.
//         numTries = schedule_length;
         numTries = (ubSt - lbSt) + 1;
      else
         numTries = schedule_length;
      if (maxSlack>=0 && numTries>(maxSlack+1))
         numTries = maxSlack+1;
   }
   if (numTries > schedule_length)
      numTries = schedule_length;  // no point in trying more positions than the
                            // target length of the schedule
   maxExtraLength = -1;
   if (has_upper && !only_bypass_edges)
   {
      maxExtraLength = ubSt - newTime + upperSlack;
      assert (maxExtraLength >= 0);
   }
   return (0);
}
//--------------------------------------------------------------------------------------------------------------------

/* after scheduling a node, traverse all its incident edges to check they all
 * accomodate available slack and add any unscheduled edges to the edge queue.
 * When in limp mode, add the sink ends of outgoing edges to the nodes queue if
 * all their in-order dependencies are satisfied.
 */
int
SchedDG::processIncidentEdges (Node *newNode, int templIdx, int i, int unit)
       throw (UnsuccessfulSchedulingException)
{
   UISet::iterator sit;
   EdgeList newEdges;
   
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (5, 
      cerr << "Processing incident edges for newly scheduled Node " << *newNode
           << " scheduled at time slot " << newNode->schedTime << endl;
   )
#endif
   IncomingEdgesIterator neit(newNode);
   while ((bool)neit)
   {
      if (! neit->IsRemoved() &&
           (neit->isSchedulingEdge() || neit->isSuperEdge()) )
      {
         // I am just scheduling one end, the edge cannot be 
         // scheduled already
         assert (! neit->isScheduled());
         newEdges.push_back ((Edge*)neit);
      }
      ++ neit;
   }
   OutgoingEdgesIterator neot(newNode);
   InstructionClass srcIC = newNode->makeIClass();
   while ((bool)neot)
   {
      Edge *tempE = (Edge*) neot;
      if (! tempE->IsRemoved() && 
            (tempE->isSchedulingEdge() || tempE->isSuperEdge()) )
      {
      
         // I am just scheduling one end, the edge cannot be 
         // scheduled already
         assert (! tempE->isScheduled());
         // for outgoing edges, compute also the actual latency
         // because we know what template was used
         // recompute for edges without bypass rule only
         if (! (tempE->_flags & HAS_BYPASS_RULE_EDGE))
         {
            tempE->realLatency = mach->computeLatency (
                       newNode->getAddress(), 
                       reloc_offset,
                       srcIC,
                       tempE->sink()->makeIClass(), 
                       templIdx) + img->getMemLoadLatency(newNode->getAddress());
//ozgurS
            tempE->realCPULatency = mach->computeLatency (
                       newNode->getAddress(), 
                       reloc_offset,
                       srcIC,
                       tempE->sink()->makeIClass(), 
                       templIdx); 
            tempE->realMemLatency =  img->getMemLoadLatency(newNode->getAddress());
//ozgurE
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout <<__func__<<" "<<(unsigned int*)newNode->getAddress()<<" lat: "<<tempE->realLatency<<endl;
            )
#endif
            if (tempE->overlapped>0 && (tempE->dtype==GP_REGISTER_TYPE
                     || tempE->dtype==ADDR_REGISTER_TYPE))
            {
               if (tempE->realLatency > tempE->overlapped)
               {
                  tempE->realLatency -= tempE->overlapped;
//ozgurS
                  tempE->realCPULatency -= tempE->overlapped;
//ozgurE
               } 
               else//TODO Check if I need to do anything with mem
               {
                  // we want a minimum latency of 1 such that the
                  // inner loop does not overlap with any instruct.
                  tempE->realLatency = 1; 
//ozgurS              
                  tempE->realCPULatency = 1;               
//ozgurE
               }
            }
         }
         newEdges.push_back (tempE);
      }
      ++ neot;
   }

   // check each edge for correctness, slack info and what not
   // if both ends are scheduled, then test that the distance between them
   // is at least as large as the edge's length.
   // Fix: change the tests to reflect that at most one end can be
   // scheduled already.
   // - if one end is scheduled, then check the edge length and mark
   //   the edge as scheduled
   // - if no end is scheduled, will add the edge into the edge queue
   // - if this is limp mode, I will add the sink nodes to the nodes
   //   queue if they have all dependencies from predecessor uops resolved.
   EdgeList::iterator elit = newEdges.begin();
   EdgeList readyEdges;
   for ( ; elit!=newEdges.end() ; ++elit)
   {
      Edge *crtE = *elit;
      if (crtE->isScheduled())
      {
         // it must be an one edge cycle
         assert (crtE->source()==crtE->sink());
         continue;
      }
      int srcSchedId = crtE->source()->scheduleId();
      int sinkSchedId = crtE->sink()->scheduleId();
      int edist = crtE->getDistance();
      // one end must be marked as unscheduled (current node)
      assert (srcSchedId==0 || sinkSchedId==0);
      if (srcSchedId>0 || sinkSchedId>0 || // the other end is scheduled
           crtE->source()==crtE->sink())   // or this is a self cycle
      {
         // test if the distance between nodes is enough to satisfy 
         // the edge length, and not exceed the slack
         int diffCycles = (crtE->sink()->schedTime % edist) - 
                          crtE->source()->schedTime;
         // check if distance is too short
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout <<__func__<<" "<<(unsigned int*)crtE->sink()->getAddress()<<" "<<crtE->sink()->schedTime<<" "<<(edist)<<endl;
            )
#endif
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout <<__func__<<" "<<(unsigned int*)crtE->source()->getAddress()<<" "<<crtE->source()->schedTime<<endl;
            )
#endif
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout <<__func__<<" "<<crtE->getActualLatency()<<" "<<crtE->getLatency()<<" "<<crtE->realLatency<<" "<<diffCycles<<endl;
            )
#endif
         if (diffCycles < (int)(crtE->getActualLatency()))
         {
            // I do not know if I should have such a case
            // me thinks not; assert for now.
            // If this case can occur, make sure to adjust the 
            // exception mask parameter accordingly
            // 03/23/2007 mgabi: I changed the edges priority queue such that
            // I recompute the priority of every edge that is part of a cycle
            // at the point when I pop it from the queue. If the new priority
            // is different (should be smaller), I insert it back.
            // I also changed how I compute the number of tries. When I have
            // both bounds for an edge, I actually want it to try for up to
            // all issue cycles, until a slot is found. I do this by not 
            // taking into account the restriction from one of the bounds.
            // With this change, it can happen that the issue slot found 
            // breaks that respective bound. But in this case I know how many
            // clock cycles I need to add to the schedule length. Should I 
            // deallocate the bound that was broken in this case? I think I
            // will only deallocate this node and throw an exception.
//            assert (!"Should I be here? I'm not sure, check it out.");
            
            // Should I just backtrack from the most recent point, 
            // or find a 'try' before the other end was scheduled?
            unsigned int maxSchedId = nextScheduleId;

#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (3, 
                  cerr << "Remaining space of " << diffCycles 
                       << " clock cycles for edge " << *crtE 
                       << " is too short by " 
                       << crtE->getActualLatency()-diffCycles 
                       << " cycles. Undo! (deallocate)" << endl;
               )
#endif
            // in limp mode we always throw the exception, we do not try
            // to unschedule nodes or backtrack
            if (! limpModeScheduling)
            {
               undo_partial_node_scheduling (newNode, &newEdges, &elit);
               // try first to unschedule the nodes either upstream or downstream
               // if this node is not part of a cycle
               if (!newNode->numUnschedules && newNode->hasCycles<3
   //                (newNode->hasCycles<3 || (resourceLimitedScore>1.5 && 
   //                newNode->minSchedIdUpstream!=newNode->minSchedIdDownstream))
                  )
               {
                  assert (newNode->minSchedIdUpstream != 
                               newNode->minSchedIdDownstream);
                  cerr << "No available slot found: UNSCHEDULE the nodes ";
                  // use the minSchedId info instead of the min number of edges
                  // upstream and downstream
                  if (newNode->minSchedIdUpstream > newNode->minSchedIdDownstream)
                  {
                     cerr << "upstream of current node" << endl;
                     undo_schedule_upstream (newNode);
                  } else
                  {
                     cerr << "downstream of current node" << endl;
                     undo_schedule_downstream (newNode);
                  }
                  newNode->numUnschedules += 1;

                  // after unscheduling the nodes, recompute the bounds
                  // if the new startng location is different than the old one, or
                  // the new numTries is larger, then attempt to reschedule current
                  // node again. Otherwise throw an exception.
                  // 
                  // Question: I already processed some edges. Do I need to
                  // undo whatever I changed to all incident edges already 
                  // processed?
                  return ((int)(crtE->getActualLatency())-diffCycles);
               }
            }
            
            // on unsuccessful exception, pass along the schedule id 
            // of the node that caused the failure, and the unit number
            throw (UnsuccessfulSchedulingException (maxSchedId, 
                     TC_ALL_EXCEPTIONS, unit, newNode, crtE, 
                     (int)(crtE->getActualLatency())-diffCycles, i));
         } 
         // check also if we exceeded the slack info (distance is
         // too long)
         int localSlack = diffCycles - crtE->getLatency ();
         
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (4, 
            cerr << "++ " << (*crtE) << " has local slack " 
                 << localSlack << endl;
         )
#endif
         // Now check all cycles this edge is part of to see if
         // we break the global constraints; only if use any slack
         // ******* HERE ********* TODO *********
         // FIXME: Also, reorder the edges, giving higher priority
         // to those that are part of cycles that have less slack
         // available. Must find an EFFICIENT solution though.
         // MORE: I realized there can be a deadlock for diamond
         // (fork + join) structures. I need to ensure that scheduling
         // order of the edges makes deadlocks impossible.
         // I can build in a system to detect if such an instance 
         // occurs. When detected, report it for investigation plus 
         // terminate the program. For this, I must keep track of
         // last place and reason the scheduling failed. If the same
         // reason repeats a predetermined number of times, then
         // report deadlock. This is implemented.
         // gmarin, 03/13/2014: When I detect DEADLOCK, I go into 
         // limp mode and attempt to reschedule the nodes using a 
         // list scheduler (not critical path based).
         if (localSlack>0 && !crtE->isSuperEdge() && !limpModeScheduling)
         {
            // I have two options here for dealing with the slack
            // 1) try to allocate and if allocation fails then I have to
            // unroll (deallocate) the slack
            // 2) first check if the slack can be accomodated, and allocate
            // the slack only if the answer is 'yes'; no deallocation needed.
            // First approach is best when we expect few unsuccessful 
            // allocations, while the second approach is advantageous in the
            // opposite case. I will go with the first approach here.
            unsigned int restSlack = localSlack;
            Edge *supEdge = 0;
            int cycId = allocateSlack (crtE, restSlack, supEdge);
            if (cycId)
            {
               assert (restSlack);
               if (cycId>0)
               {
#if VERBOSE_DEBUG_SCHEDULER
                  DEBUG_SCHED (3, 
                     cerr << "Allocation of " << localSlack 
                          << " cycles of slack for edge "
                          << *crtE << " has failed, with " << restSlack 
                          << " cycles not being covered due to cycle "
                          << cycId << ". Undo! (deallocate)" << endl;
                  )
#endif
                  assert (! deallocateSlack (crtE));
                  // must throw an exception as well.
                  Cycle *crtCyc = cycles [cycId];
                  assert (crtCyc != NULL);
                  int maxSchedId = crtCyc->findMaxSchedID();

                  undo_partial_node_scheduling (newNode, &newEdges, &elit);
                  
                  // FIXME??? I do not know if the exception mask 
                  // should include TC_NODE_WITH_MIXED_EDGES below
                  throw (UnsuccessfulSchedulingException (maxSchedId,
                           TC_SHORT_BEFORE_LONG_CYCLE | TC_CONCENTRIC_TRYCASE, 
                           unit, newNode, crtE, restSlack, i, cycId));
               } else // cycId < 0, it is a (negated) super-edge id
               {
                  assert (supEdge);
#if VERBOSE_DEBUG_SCHEDULER
                  DEBUG_SCHED (3, 
                     cerr << "Allocation of " << localSlack 
                          << " cycles of slack for edge "
                          << *crtE << " has failed, with " << restSlack 
                          << " cycles not being covered due to super-edge "
                          << *supEdge << ". Undo! (deallocate)" << endl;
                  )
#endif
                  assert (! deallocateSlack (crtE, supEdge));
                  // must throw an exception as well.
                  int maxSchedId = 0;
                  if (supEdge->source()->scheduled < supEdge->sink()->scheduled)
                     maxSchedId = supEdge->sink()->scheduled;
                  else
                     maxSchedId = supEdge->source()->scheduled;

                  undo_partial_node_scheduling (newNode, &newEdges, &elit);
                  
                  // FIXME??? I do not know if the exception mask 
                  // should include TC_NODE_WITH_MIXED_EDGES below
                  throw (UnsuccessfulSchedulingException (maxSchedId,
                           TC_SHORT_BEFORE_LONG_CYCLE | TC_CONCENTRIC_TRYCASE, 
                           unit, newNode, crtE, restSlack, i, 0));
               }
            }
         
         }
         
         // else, dependency is satisfied & slack is not exceeded 
         // -> mark edge as scheduled
         crtE->markScheduled ();

         // Also traverse all cycles for this edge and update 
         // remainingLength
         // In new version do I need to update the local paths I guess?
         if (crtE->hasCycles==3 && !limpModeScheduling)
            for (sit=crtE->cyclesIds.begin() ; 
                         sit!=crtE->cyclesIds.end() ; ++sit)
            {
               Cycle* crtCyc = cycles[*sit];
               crtCyc->remainingLength -= crtE->minLatency;
               crtCyc->remainingEdges -= 1;
               assert (crtCyc->remainingLength >= 0);
               assert (crtCyc->remainingEdges >= 0);
            }
      } else   // neither end is scheduled
      {
         // in limp mode we care only about outgoing dependencies
         if (!limpModeScheduling || newNode==crtE->source())
         {
            // add the ready edges into a list to process further
            if (! crtE->isSuperEdge ())
               readyEdges.push_back (crtE);
         }
      }
   } // for elit = newEdges.begin() ...
   newEdges.clear();

   // schedule time is already set for this node. Mark it as 
   // scheduled. This info is needed in computeMaxRemaining for edges
   newNode->scheduled = nextScheduleId++;
   newNode->unit = unit;
   newNode->resetHasMixedEdges();

   for (elit=readyEdges.begin() ; elit!=readyEdges.end() ; ++elit)
   {
      Edge* crtE = *elit;
      if (limpModeScheduling)
      {
         Node *sinkN = crtE->sink();
         if (sinkN->InOrderDependenciesSatisfied(false))
            nodesQ->push(sinkN);
      } else
      {
         computeMaxRemaining(crtE);
         // add the edge in the edges prio queue
         edgesQ->push(crtE);
      }
   }
   readyEdges.clear ();

   // increment fixedNodes in all cycles
   // Not sure what to do here in the new version which has the hierarchical
   // super edges. Do I mark only cycle at this level or do I have to go 
   // higher on the super edge chain? Since I start scheduling only from nodes
   // that are not part of a super-structure, an interior node can be a member 
   // of a cycle only if both its super-structure ends are part of the cycle.
   // So it should be pretty safe to process only cycles at current level.
   sit = newNode->cyclesIds.begin();
   for ( ; sit!=newNode->cyclesIds.end() ; ++sit )
   {
      register Cycle* crtCyc = cycles[*sit];
//      if (crtCyc->isCycle() || !newNode->isBarrierNode())
      if (crtCyc->isCycle())
         crtCyc->fixedNodes += 1;
   }
   
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (3, 
      cerr << "7 ->>> In create_schedule, just scheduled node " 
           << *newNode << endl;
   )
#endif
   return (0);
}
//--------------------------------------------------------------------------------------------------------------------

// return true if the newly computed edge priority is equal to the old 
// values saved into the Edge object
bool
SchedDG::computeMaxRemaining (Edge *ee)
{
   // must compute the maximum remainingLength from all cycles
   // this edge is part of
   int totalMaxRemaining = 0;
   int totalRemainingEdges = 0;
   Edge *crtE = ee;
   bool src_is_scheduled = (ee->source()->isScheduled());

#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (7, 
      cerr << "Computing maxRemaining for " << *crtE << endl;
   )
#endif
   // one and only one end of the edge can be scheduled at this point
   Node* srcNode = crtE->source();
   Node* destNode = crtE->sink();
   assert (srcNode->isScheduled() || destNode->isScheduled());
   assert (!srcNode->isScheduled() || !destNode->isScheduled());
   while (crtE->superE)
   {
      Edge *supE = crtE->superE;
      int structId = supE->structureId;
      assert (crtE->cyclesIds.empty());
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (7, 
         cerr << "SuperE: " << *supE << ", structId=" << structId
              << ", srcNode->structId=" << srcNode->structureId
              << ", destNode->structId=" << destNode->structureId
              << endl;
      )
#endif
      // check if current node is source or sink for this edge
      if (srcNode->isScheduled() && (src_is_scheduled || !destNode->isScheduled()))
      {
         if (!src_is_scheduled)
         {
            src_is_scheduled = true;
            cerr << "Warning: computeMaxRemaining 1, detected change in schedule direction while traversing edge hierarchy." << endl;
         }
         // outgoing edge
         // do not consider the length of this edge when
         // computing the remaining length. I do this to avoid
         // scheduling the last edge before a fork/join, when 
         // I still have at least one extra node to schedule on 
         // the other branch. It is guaranteed that the other
         // branch will be part of a longer path.
         if (structId == destNode->structureId)
         {
            totalMaxRemaining += destNode->pathToExit;
            totalRemainingEdges += destNode->edgesToExit;
         }
      } else
      {
         if (src_is_scheduled)
         {
            src_is_scheduled = false;
            cerr << "Warning: computeMaxRemaining 2, detected change in schedule direction while traversing edge hierarchy." << endl;
         }
         assert (destNode->isScheduled());
         // incoming edge
         if (structId == srcNode->structureId)
         {
            totalMaxRemaining += srcNode->pathFromEntry;
            totalRemainingEdges += srcNode->edgesFromEntry;
         }
      }
      crtE = crtE->superE;
      srcNode = crtE->source();
      destNode = crtE->sink();
   }
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (7, 
      cerr << "After SuperE loop, totalMaxRemaining=" << totalMaxRemaining
           << ", totalRemainingEdges=" << totalRemainingEdges << endl;
   )
#endif
   if (crtE->hasCycles==3 /*&& !heavyOnResources*/)
   {
      /* gmarin, 09/18/2013: I am not going to use the maximum remaining latency 
       * in a cycle, but the distance from root / to leaf, inside a scc.
       */
#if USE_SCC_PATHS
      // check if current node is source or sink for this edge
      if (srcNode->isScheduled() && (src_is_scheduled || !destNode->isScheduled()))
      {
         if (!src_is_scheduled)
         {
            src_is_scheduled = true;
            cerr << "Warning: computeMaxRemaining (cyc==3), detected change in schedule direction while traversing edge hierarchy." << endl;
         }
         // outgoing edge
         // do not consider the length of this edge when
         // computing the remaining length. I do this to avoid
         // scheduling the last edge before a fork/join, when 
         // I still have at least one extra node to schedule on 
         // the other branch. It is guaranteed that the other
         // branch will be part of a longer path.
         if (crtE->ddistance != 0)
         {
            totalMaxRemaining += 0;
            totalRemainingEdges += 0;
         } else
         {
            totalMaxRemaining += destNode->sccPathToLeaf;
            totalRemainingEdges += destNode->sccEdgesToLeaf;
         }
      } else
      {
         if (src_is_scheduled)
         {
            src_is_scheduled = false;
            cerr << "Warning: computeMaxRemaining (cyc==3),2, detected change in schedule direction while traversing edge hierarchy." << endl;
         }
         assert (destNode->isScheduled());
         // incoming edge
         if (crtE->ddistance != 0)
         {
            totalMaxRemaining += 0;
            totalRemainingEdges += 0;
         } else
         {
            totalMaxRemaining += srcNode->sccPathFromRoot;
            totalRemainingEdges += srcNode->sccEdgesFromRoot;
         }
      }

#else   // USE_SCC_PATHS

      int maxRemainingTemp = 0;
      int remainingEdgesTemp = 0;
      UISet::iterator sit;
      for (sit=crtE->cyclesIds.begin() ; 
                      sit!=crtE->cyclesIds.end() ; ++sit)
      {
         Cycle* crtCyc = cycles[*sit];

#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (7, 
            cerr << "** Cycle/Path " << *sit << ": length="
                 << crtCyc->cLength << ", iters=" << crtCyc->iters
                 << ", remaining=" << crtCyc->remainingLength
                 << ", " << ((crtCyc->isCycle())?"CYCLE":"SEGMENT") 
                 << endl;
         )
#endif
         // do not consider the length of this edge in the 
         // remaining amount. I must do the substraction here
         // to accomodate cycles with multiple iters where I have
         // to divide the amount after the substraction.
         int thisRemaining = crtCyc->remainingLength;
         int thisEdges = crtCyc->remainingEdges;
         thisRemaining -= crtE->minLatency;
         thisEdges -= 1;

         if (crtCyc->isCycle() && crtCyc->iters>1) 
         // a cycle that crosses more than 1 iters
         {
            // compute the remaining value as the average, 
            // rounded to nearest integer
            thisRemaining = (int)((float)thisRemaining *
                      crtCyc->revIters + 0.49);
            thisEdges = (int)((float)thisEdges * 
                      crtCyc->revIters + 0.49);
         }
         if (thisRemaining>maxRemainingTemp || 
               (thisRemaining==maxRemainingTemp &&
                thisEdges>remainingEdgesTemp) )
         {
            maxRemainingTemp = thisRemaining;
            remainingEdgesTemp = thisEdges;
         }
      }
      totalMaxRemaining += maxRemainingTemp;
      totalRemainingEdges += remainingEdgesTemp;
#endif

#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (7, 
         cerr << "Final maxRemaining=" << totalMaxRemaining
              << ", remainingEdges=" << totalRemainingEdges << endl;
      )
#endif
      assert (totalMaxRemaining >= 0);
   } else 
   if (crtE->hasCycles>0 /*&& !heavyOnResources*/) // no cycles, but connects 2 cyc
   {
      // there is a special case for paths of edges that connect two cycles
      // when placing the last edge into the ready queue, I must set the
      // priority such that I do not get a DEADLOCK when I have two or more
      // parallel paths connecting nodes from two cycles. The edge that is 
      // firstly selected, should be the one that connects to the node
      // with the longest remaining distance (in the direction of the edge
      // connecting the cycles) to a root or leaf node.
      // I may need to introduce another value for 'hasCycles' to represent
      // this special case. It must have lower priority than all the other
      // edges that are part of paths connecting cycles (the ones with 
      // hasCycles==1 before, now they have value 2), but higher priority 
      // than simple edges (part of no cycle and not on a path between 
      // cycles). So I kicked everything up a notch. Real cycles have value
      // of 3, paths between cycles have value of 2, and these last edges
      // of a path between cycles will have value of 1.
      // check if current node is source or sink for this edge
      if (srcNode->isScheduled() && (src_is_scheduled || !destNode->isScheduled()))
      {
         if (!src_is_scheduled)
         {
            src_is_scheduled = true;
            cerr << "Warning: computeMaxRemaining 3, detected change in schedule direction while traversing edge hierarchy." << endl;
         }
         // outgoing edge
         // do not consider the length of this edge when
         // computing the remaining length. I do this to avoid
         // scheduling the last edge before a fork/join, when 
         // I still have at least one extra node to schedule on 
         // the other branch. It is guaranteed that the other
         // branch will be part of a longer path.
         // Also, check if the destination node is part of a cycle, which
         // means this edge is the last edge to be scheduled on the path
         // connecting two cycles.
         if (destNode->hasCycles==3 && destNode==ee->sink())
         {
            ee->hasCycles = 1;
            totalMaxRemaining = destNode->pathToLeaf;
            totalRemainingEdges = destNode->edgesToLeaf;
         } else   // we're not there yet
         {
            ee->hasCycles = 2;
            totalMaxRemaining += destNode->pathToCycle;  // why do I use += ??
            totalRemainingEdges += destNode->edgesToCycle;
         }
      } else
      {
         if (src_is_scheduled)
         {
            src_is_scheduled = false;
            cerr << "Warning: computeMaxRemaining 4, detected change in schedule direction while traversing edge hierarchy." << endl;
         }
         assert (destNode->isScheduled());
         // incoming edge
         // check if this is last unscheduled edge before a cycle
         if (srcNode->hasCycles==3 && srcNode==ee->source())
         {
            ee->hasCycles = 1;
            totalMaxRemaining = srcNode->pathFromRoot;
            totalRemainingEdges = srcNode->edgesFromRoot;
         } else   // we're not there yet
         {
            ee->hasCycles = 2;
            totalMaxRemaining += srcNode->pathFromCycle;
            totalRemainingEdges += srcNode->edgesFromCycle;
         }
      }
   } else   // it is not a cycle or a connection between cycles
   {
      // check if current node is source or sink for this edge
      if (srcNode->isScheduled() && (src_is_scheduled || !destNode->isScheduled()))
      {
         if (!src_is_scheduled)
         {
            src_is_scheduled = true;
            cerr << "Warning: computeMaxRemaining 5, detected change in schedule direction while traversing edge hierarchy." << endl;
         }
         // outgoing edge
         // do not consider the length of this edge when
         // computing the remaining length. I do this to avoid
         // scheduling the last edge before a fork/join, when 
         // I still have at least one extra node to schedule on 
         // the other branch. It is guaranteed that the other
         // branch will be part of a longer path.
         //
         // the += is here because for edges that are part of super-structures,
         // I already considered the distance to the structure entry/exit, and
         // the srcNode / destNode point to the entry / exit nodes of the 
         // outermost super-structure.
         totalMaxRemaining += destNode->pathToLeaf;
         totalRemainingEdges += destNode->edgesToLeaf;
      } else
      {
         if (src_is_scheduled)
         {
            src_is_scheduled = false;
            cerr << "Warning: computeMaxRemaining 6, detected change in schedule direction while traversing edge hierarchy." << endl;
         }
         assert (destNode->isScheduled());
         // incoming edge
         //
         // the += is here because for edges that are part of super-structures,
         // I already considered the distance to the structure entry/exit, and
         // the srcNode / destNode point to the entry / exit nodes of the 
         // outermost super-structure.
         totalMaxRemaining += srcNode->pathFromRoot;
         totalRemainingEdges += srcNode->edgesFromRoot;
      }
   }

   ee->prioTimeStamp = nextScheduleId;
   // check and return true if the new priority is equal to the old one
   if (ee->prioTimeStamp && ee->maxRemaining==totalMaxRemaining &&
             ee->remainingEdges==totalRemainingEdges)
   {
      return (true);
   }
   // otherwise just save the new priority info
   ee->maxRemaining = totalMaxRemaining;
   ee->remainingEdges = totalRemainingEdges;
   // record the time when this priority was computed
   return (false);
}
//--------------------------------------------------------------------------------------------------------------------

// following routines: 1) compute how much slack is available for an edge;
// 2) determine if an edge can accomodate a predeterminate amount of slack;
// 2) allocate slack for an edge; and 3) deallocate slack for an edge.
// Edges can be part of a hierarchical structure. Only edges at the top (edges
// without super-edges) can be part of cycles. If an edge has a super-edge, it
// means it is part of a super-structure. I need to find how much slack is
// available at the level of this super-structure and then go up the tree and
// add available slack at each level. Similarly, for allocation of slack 
// I have to execute the same steps, except now I commit the changes.
// Deallocation is more difficult. After I deallocate slack at my current 
// level, I need to check if the critical path in current super-structure is
// shortened. If so, then I have to go the next higher level and do the same 
// check.

#define UNLIMITED_SLACK 0x40000000

// this routine has not been updated for the new approach where we do not
// store any segments but only distance to entry and distance to exit for
// all internal nodes of super-structures.
int
SchedDG::availableSlack (Edge *edge)
{
   UISet::iterator sit;
   Edge *iterE = edge;
   int availSlack = 0;
   while (iterE)
   {
      // if this edge is part of no cycles, then the amount of available
      // slack is unlimited; I can just break out of this loop;
      if (iterE->cyclesIds.empty())
      {
         return (-1);   // means unlimited slack
      }
      
      int maxLengthHere = 0;
      if (iterE->superE)
         // if another path has been already lengthened (and the used slack
         // propagated further), then I can overlap with the extra length on
         // this path as well
         maxLengthHere = iterE->superE->getLatency () + 
                         iterE->superE->usedSlack;
      else
         maxLengthHere = schedule_length;
      int maxSlack = UNLIMITED_SLACK;
      
      for (sit=iterE->cyclesIds.begin() ; 
                 sit!=iterE->cyclesIds.end() ; ++sit)
      {
         int maxSlackHere = 0;
         Cycle* crtCyc = cycles[*sit];
         assert (crtCyc != NULL);
         // if these asserts (isCycle() and isSegment() fail in the
         // future and they are determined to be incorrect, change
         // the IFs to check specifically if the cyc object is a 
         // segment or a cycle instead of testing the superE.
         // if superE is non NULL, all cyc objs should be segments??
         // unless I can have cycles within structs (unsure yet)
         if (crtCyc->isSegment())
         {
            assert (iterE->superE);
            // compare to length of this superE (see if we have
            // any local slack in this path (cyc))
            int extraIters = crtCyc->iters - iterE->superE->ddistance;
            assert (extraIters>=0);
            if (extraIters)
            {
               if (extraIters==1)
                  maxSlackHere = schedule_length;
               else
                  maxSlackHere = (schedule_length*extraIters);
            }
         } else
         {
            // unless I can have cycles within structures
            assert (! iterE->superE); 
            int factorVal = crtCyc->iters;
            factorVal -= 1;
            if (factorVal)
               maxSlackHere = schedule_length*factorVal;
         }
         maxSlackHere += maxLengthHere - crtCyc->cLength
                       - crtCyc->usedSlack;
         
         if (maxSlackHere < maxSlack)
         {
            maxSlack = maxSlackHere;
            if (maxSlack == 0)  // no need to traverse the other cycs
               break;
         }
      }
      assert (maxSlack != UNLIMITED_SLACK);
      assert (maxSlack >= 0);
//      if (maxSlack < 0)   // I do not care for negative slack
//         maxSlack = 0;
      availSlack += maxSlack;
      iterE = iterE->superE;
   }
   return (availSlack);
}
//--------------------------------------------------------------------------------------------------------------------

// return 0 for success; positive value for insuccess (the id of the most
// restrictive cycle); set also the slack parameter to the amount of slack
// that could not be allocated.
int
SchedDG::canAccomodateSlack (Edge *edge, unsigned int &slack)
{
   UISet::iterator sit;
   Edge *iterE = edge;
   int restSlack = slack;
   int cycId =0;
   while (iterE && restSlack)
   {
      // in the newest version I do not store segments any more. So all Cycle
      // objects represent proper cycles. In addition, only edges that are not
      // part of super-structures may be part of any cycles.
      assert (!iterE->superE || iterE->cyclesIds.empty());
      
      // if this edge is part of no cycles, then the amount of available
      // slack is unlimited; I can just break out of this loop;
      if (!iterE->superE && iterE->cyclesIds.empty())
      {
         slack = 0;
         return (0);   // means all slack can be covered
      }
      
      int maxLengthHere = 0;
      int maxSlack = UNLIMITED_SLACK;
      if (iterE->superE)
      {
         Edge *supE = iterE->superE;
         int structId = supE->structureId;
//         int supDist = supE->ddistance;
         // if another path has been already lengthened (and the used slack
         // propagated further), then I can overlap with the extra length on
         // this path as well
#if 0
         if (supE->source()->scheduled && supE->sink()->scheduled)
         {
            maxLengthHere = (supE->sink()->schedTime % supDist) - 
                                supE->source()->schedTime;
            if (maxLengthHere > supE->getLatency()+supE->usedSlack)
               supE->usedSlack = maxLengthHere - supE->getLatency();
            else
               assert (maxLengthHere == supE->getLatency()+supE->usedSlack);
         } else
#endif
            maxLengthHere = supE->getLatency () + supE->usedSlack;
         
         // at least one end of the super-structure must have been already 
         // scheduled
         assert (supE->source()->scheduled || supE->sink()->scheduled);
         // compute the slack piecewise
         int srcDist, sinkDist;
         if (supE->source()->scheduled && iterE->source()->scheduled)
         {
            if (iterE->source()==supE->source())
               srcDist = 0;
            else
               srcDist = (iterE->source()->schedTime % iterE->source()->distFromEntry)
                         - supE->source()->schedTime;
            assert (srcDist >= 0);
         } else
            srcDist = (structId==iterE->source()->structureId)?
                      iterE->source()->pathFromEntry:0;
         if (supE->sink()->scheduled && iterE->sink()->scheduled)
         {
            if (iterE->sink()==supE->sink())
               sinkDist = 0;
            else
               sinkDist = (supE->sink()->schedTime % iterE->sink()->distToExit)
                           - iterE->sink()->schedTime;
            assert (sinkDist >= 0);
         } else
            sinkDist = (structId==iterE->sink()->structureId)?
                       iterE->sink()->pathToExit:0;
         maxSlack = maxLengthHere - srcDist - sinkDist - iterE->getLatency()
                      - iterE->usedSlack;
      }
      else
      {
         maxLengthHere = schedule_length;
         cycId = 0;
      
         for (sit=iterE->cyclesIds.begin() ; 
                    sit!=iterE->cyclesIds.end() ; ++sit)
         {
            int maxSlackHere = 0;
            Cycle* crtCyc = cycles[*sit];
            assert (crtCyc != NULL);
            // if these asserts (isCycle() and isSegment() fail in the
            // future and they are determined to be incorrect, change
            // the IFs to check specifically if the cyc object is a 
            // segment or a cycle instead of testing the superE.
            // if superE is non NULL, all cyc objs should be segments??
            // unless I can have cycles within structs (unsure yet)
            // 01/23/2008 mgabi: should all be Cycles now.
            if (crtCyc->isSegment())
            {
               assert (!"It cannot be a Segment anymore");
               assert (iterE->superE);
               // compare to length of this superE (see if we have
               // any local slack in this path (cyc))
               int extraIters = crtCyc->iters - iterE->superE->ddistance;
               assert (extraIters>=0);
               if (extraIters)
               {
                  if (extraIters==1)
                     maxSlackHere = schedule_length;
                  else
                     maxSlackHere = (schedule_length*extraIters);
               }
            } else
            {
               // unless I can have cycles within structures
               assert (! iterE->superE); 
               int factorVal = crtCyc->iters;
               factorVal -= 1;
               if (factorVal)
                  maxSlackHere = schedule_length*factorVal;
            }
            maxSlackHere += maxLengthHere - crtCyc->cLength
                          - crtCyc->usedSlack;
            if (maxSlackHere < maxSlack)
            {
               maxSlack = maxSlackHere;
               cycId = *sit;
               if (maxSlack == 0)  // no need to traverse the other cycs
                  break;
            }
         }
      }
      assert (maxSlack != UNLIMITED_SLACK);
      assert (maxSlack >= 0);
      if (restSlack <= maxSlack)
      {
         restSlack = 0;
         cycId = 0;
      }
      else
         restSlack -= maxSlack;
      iterE = iterE->superE;
   }
   slack = restSlack;
   return (cycId);
}
//--------------------------------------------------------------------------------------------------------------------

// return 0 for success; positive value for insuccess (the id of the most
// restrictive cycle); set also the slack parameter to the amount of slack
// that could not be allocated.
// return negative value if slack allocation failed due to scheduled super
// edge. Point supEdge to the restrictive super-edge where allocation failed.
int
SchedDG::allocateSlack (Edge *edge, unsigned int &slack, Edge* &supEdge)
{
   UISet::iterator sit;
   assert (! edge->isSuperEdge());
   Edge *iterE = edge;
   int restSlack = slack;
   int cycId = 0;
   // I have to also allocate slack at each level
   while (iterE && restSlack)
   {
      // one end of the edge must be already scheduled
      assert (iterE->source()->scheduled || iterE->sink()->scheduled ||
           iterE->source()==iterE->sink());
      
      // in the newest version I do not store segments any more. So all Cycle
      // objects represent proper cycles. In addition, only edges that are not
      // part of super-structures may be part of any cycles.
      assert (!iterE->superE || iterE->cyclesIds.empty());
      iterE->usedSlack += restSlack;
      
      // if this edge is part of no cycles, then the amount of available
      // slack is unlimited; I can just break out of this loop;
      if (!iterE->superE && iterE->cyclesIds.empty())
      {
         slack = restSlack = 0;
         return (0);   // means all slack can be covered
      }
      
      int maxLengthHere = 0;
      int propagatedSlack = 0;

      if (iterE->superE)
      {
         Edge *supE = iterE->superE;
         int structId = supE->structureId;
//         int supDist = supE->ddistance;
         int srcDist, sinkDist;
         
         // test first which end of the edge is scheduled now. Check if the
         // opposing end of the super-structure is scheduled as well.
         // In this case we cannot even propagate slack further since our
         // scheduling is limited by the two scheduled ends of the 
         // super-structure.
         if (!iterE->source()->scheduled && supE->source()->scheduled)
         {
            // source node is not yet scheduled
            int availSlack = 0;
            assert (iterE->sink()->scheduled && supE->sink()->scheduled);
            if (iterE->sink()->structureId == structId) // iterE->sink!=supE->sink
            {
               availSlack = (iterE->sink()->schedTime % iterE->sink()->distFromEntry)
                           - supE->source()->schedTime 
                           - iterE->source()->pathFromEntry - iterE->getLatency();
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (5, 
                  fprintf(stderr, "ALLOCATE slack, sink is internal node of SuperStructure, availSlack=%d\n",
                       availSlack);
               )
#endif
            }
            else
            {
               availSlack = (supE->sink()->schedTime % supE->ddistance)
                           - supE->source()->schedTime 
                           - iterE->source()->pathFromEntry - iterE->getLatency();
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (5, 
                  fprintf(stderr, "ALLOCATE slack, sink is SuperStructure exit node(?), availSlack=%d\n",
                       availSlack);
               )
#endif
            }
            assert (availSlack >= 0);
            // gmarin, 09/12/2013: I should compare against the edge's total slack,
            // which inludes the restSlack (added above), as well as any previously
            // allocated slack
            if (availSlack < iterE->usedSlack)  //restSlack)  
            // the other end of the super-edge is already scheduled and there
            // isn't enough slack
            {
               cycId = -(supE->id);
               slack = iterE->usedSlack - availSlack;
//               slack = restSlack - availSlack;
               supEdge = supE;
               return (cycId);
            }
         }
         if (!iterE->sink()->scheduled && supE->sink()->scheduled)
         {
            // sink node is not yet scheduled
            int availSlack = 0;
            assert (iterE->source()->scheduled && supE->source()->scheduled);
            if (iterE->source()->structureId == structId) // iterE->source!=supE->source
            {
               availSlack = (supE->sink()->schedTime % iterE->source()->distToExit)
                           - iterE->source()->schedTime 
                           - iterE->sink()->pathToExit - iterE->getLatency();
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (5, 
                  fprintf(stderr, "ALLOCATE slack, source is internal node of SuperStructure, availSlack=%d\n",
                       availSlack);
               )
#endif
            }
            else
            {
               availSlack = (supE->sink()->schedTime % supE->ddistance)
                           - supE->source()->schedTime 
                           - iterE->sink()->pathToExit - iterE->getLatency();
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (5, 
                  fprintf(stderr, "ALLOCATE slack, source is SuperStructure entry node(?), availSlack=%d\n",
                       availSlack);
               )
#endif
            }
            assert (availSlack >= 0);
            if (availSlack < iterE->usedSlack)  //restSlack)  
            // the other end of the super-edge is already scheduled and there
            // isn't enough slack
            {
               cycId = -(supE->id);
               slack = iterE->usedSlack - availSlack;
//               slack = restSlack - availSlack;
               supEdge = supE;
               return (cycId);
            }
         }
         
         // if another path has been already lengthened (and the used slack
         // propagated further), then I can overlap with the extra length on
         // this path as well
#if 0
         if (supE->source()->scheduled && supE->sink()->scheduled)
         {
            maxLengthHere = (supE->sink()->schedTime % supDist) - 
                                supE->source()->schedTime;
            if (maxLengthHere > supE->getLatency()+supE->usedSlack)
               supE->usedSlack = maxLengthHere - supE->getLatency();
            else
               assert (maxLengthHere == supE->getLatency()+supE->usedSlack);
         } else
#endif
            maxLengthHere = supE->getLatency () + supE->usedSlack;
         
         // at least one end of the super-structure must have been already 
         // scheduled
         assert (supE->source()->scheduled || supE->sink()->scheduled);
         // compute the slack piecewise
         if (supE->source()->scheduled && iterE->source()->scheduled)
         {
            if (iterE->source()==supE->source())
               srcDist = 0;
            else
               srcDist = (iterE->source()->schedTime % iterE->source()->distFromEntry)
                          - supE->source()->schedTime;
            assert (srcDist >= 0);
         } else
            srcDist = (structId==iterE->source()->structureId)?
                      iterE->source()->pathFromEntry:0;
         if (supE->sink()->scheduled && iterE->sink()->scheduled)
         {
            if (iterE->sink()==supE->sink())
               sinkDist = 0;
            else
               sinkDist = (supE->sink()->schedTime % iterE->sink()->distToExit)
                          - iterE->sink()->schedTime;
            assert (sinkDist >= 0);
         } else
            sinkDist = (structId==iterE->sink()->structureId)?
                       iterE->sink()->pathToExit:0;
         propagatedSlack = srcDist + sinkDist + iterE->getLatency()
                   + iterE->usedSlack - maxLengthHere;
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (4, 
            cerr << "Propagate slack " << propagatedSlack << " to SuperEdge " << *supE
                 << ", where srcDist=" << srcDist << ", sinkDist=" << sinkDist
                 << ", lat=" << iterE->getLatency() << ", usedSlack=" << iterE->usedSlack
                 << ", and current Max SuperE Length = " << maxLengthHere << endl;
         )
#endif
         if (propagatedSlack < 0) propagatedSlack = 0;
      }
      else
      {
         maxLengthHere = schedule_length;
         cycId = 0;
               
         for (sit=iterE->cyclesIds.begin() ; 
                    sit!=iterE->cyclesIds.end() ; ++sit)
         {
            int extraLength = 0;
            Cycle* crtCyc = cycles[*sit];
            assert (crtCyc != NULL);
            crtCyc->usedSlack += restSlack;  // each path is lengthened by restSlack

            // if these asserts (isCycle() and isSegment() fail in the
            // future and they are determined to be incorrect, change
            // the IFs to check specifically if the cyc object is a 
            // segment or a cycle instead of testing the superE.
            // if superE is non NULL, all cyc objs should be segments??
            // unless I can have cycles within structs (unsure yet)
            // 01/23/2008 mgabi: should all be Cycles now.
            if (crtCyc->isSegment())
            {
               assert (!"It cannot be a Segment anymore");
               assert (iterE->superE);
               // compare to length of this superE (see if we have
               // any local slack in this path (cyc))
               int extraIters = crtCyc->iters - iterE->superE->ddistance;
               assert (extraIters>=0);
               if (extraIters)
               {
                  if (extraIters==1)
                     extraLength = schedule_length;
                  else
                     extraLength = (schedule_length*extraIters);
               }
            } else
            {
               // unless I can have cycles within structures
               assert (! iterE->superE); 
               int factorVal = crtCyc->iters;
               factorVal -= 1;
               if (factorVal)
                  extraLength = schedule_length*factorVal;
            }
            extraLength =  crtCyc->cLength + crtCyc->usedSlack 
                           - maxLengthHere - extraLength;
            if (extraLength > propagatedSlack)
            {
               propagatedSlack = extraLength;
               cycId = *sit;
            }
         }
      }
      assert (propagatedSlack >= 0);
      restSlack = propagatedSlack;
      iterE = iterE->superE;
   }
   slack = restSlack;
   return (cycId);
}
//--------------------------------------------------------------------------------------------------------------------

// deallocate the slack used by 'edge'
int
SchedDG::deallocateSlack (Edge *edge, Edge *supEdge)
{
   assert (! edge->isSuperEdge ());
   Edge *iterE = edge;
   int restSlack = iterE->usedSlack;
   UISet::iterator sit;
   // I have to deallocate slack at one level, then test if critical path 
   // length is changed. If yes, continue to deallocate the length difference
   // from higher levels
   while (iterE!=supEdge && restSlack)
   {
      // in the newest version I do not store segments any more. So all Cycle
      // objects represent proper cycles. In addition, only edges that are not
      // part of super-structures may be part of any cycles.
      assert (!iterE->superE || iterE->cyclesIds.empty());
      
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (5, 
         cerr << "DeallocateSlack for edge " << *iterE << " with usedSlack="
              << iterE->usedSlack << " and restSlack=" << restSlack << endl;
      )
#endif
      assert (iterE->usedSlack >= restSlack);
      iterE->usedSlack -= restSlack;
      // if this edge is part of no cycles, then it must be at the top level;
      // assert that this edge has no super edge and return.
      if (!iterE->superE && iterE->cyclesIds.empty())
      {
         restSlack = 0;
         return (0);
      }
      
      // deallocate restSlack from each cycle/path
      for (sit=iterE->cyclesIds.begin() ; 
                 sit!=iterE->cyclesIds.end() ; ++sit)
      {
         // only edges part of no super-structure can have cycles
         Cycle* crtCyc = cycles[*sit];
         assert (crtCyc != NULL);
         assert (crtCyc->usedSlack >= restSlack);
         crtCyc->usedSlack -= restSlack;
      }
      
      // Now, in case this is not a top edge (it has a super-edge), I have 
      // to check if the critical path through the structure has changed;
      if (iterE->superE)  // all its associated paths should be segments
      {
         int maxLength = 0;
         assert (iterE->superE->subEdges);
         Edge *supE = iterE->superE;
         int structId = supE->structureId;
         EdgeSet::iterator eit;
         for (eit=iterE->superE->subEdges->begin () ; 
                         eit!=iterE->superE->subEdges->end () ; ++eit)
         {
            int srcDist, sinkDist;
            Edge *ee = *eit;
            if (supE->source()->scheduled && ee->source()->scheduled)
            {
               if (ee->source()==supE->source())
                  srcDist = 0;
               else
                  srcDist = (ee->source()->schedTime % ee->source()->distFromEntry)
                             - supE->source()->schedTime;
               assert (srcDist >= 0);
            } else
               srcDist = (structId==ee->source()->structureId)?
                         ee->source()->pathFromEntry:0;
            if (supE->sink()->scheduled && ee->sink()->scheduled)
            {
               if (ee->sink()==supE->sink())
                  sinkDist = 0;
               else
                  sinkDist = (supE->sink()->schedTime % ee->sink()->distToExit)
                             - ee->sink()->schedTime;
               assert (sinkDist >= 0);
            } else
               sinkDist = (structId==ee->sink()->structureId)?
                          ee->sink()->pathToExit:0;
            int crtLength = srcDist + sinkDist + ee->getLatency()
                         + ee->usedSlack;

            if (crtLength > maxLength)
            {
               maxLength = crtLength;
#if VERBOSE_DEBUG_SCHEDULER
               DEBUG_SCHED (6, 
                  cerr << "** New maxLength=" << maxLength << " due to edge "
                       << *ee << " with srcDist=" << srcDist << ", "
                       << " sinkDist=" << sinkDist << ", len=" << ee->getLatency()
                       << " and usedSlack=" << ee->usedSlack << endl;
               )
#endif
            }
         }
         if (maxLength < 0)
         {
            fprintf(stderr, "Found SuperE maxLength=%d\n", maxLength);
            assert (maxLength >= 0);
         }
         // check if the new maxLength is shorter than previous length
         int prevLength = 0;
         if (supE->source()->scheduled && supE->sink()->scheduled)
            prevLength = (supE->sink()->schedTime % supE->ddistance)
                       - supE->source()->schedTime;
         else
            prevLength = supE->getLatency () + supE->usedSlack;
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (6, 
            cerr << "** prevLength=" << prevLength << " where lat="
                 << supE->getLatency() << " and slack=" << supE->usedSlack 
                 << endl;
         )
#endif
         // length cannot be larger after removing some slack
         assert (maxLength <= prevLength);
         restSlack = prevLength - maxLength;
      } else
         restSlack = 0;

      iterE = iterE->superE;
   }
   if (supEdge && iterE==supEdge)
      restSlack = 0;
   return (restSlack);
}
//--------------------------------------------------------------------------------------------------------------------


void
SchedDG::draw_dominator_tree (BaseDominator *bdom, const char* prefix)
{
   char file_name[64];
   char file_name_ps[64];
   char title_draw[64];

   sprintf(file_name, "%s_%s.dot", prefix, pathId.Filename());
   sprintf(file_name_ps, "%s_%s.ps", prefix, pathId.Filename());
   sprintf(title_draw, "%s_%s", prefix, pathId.Filename());
   
   ofstream fout(file_name, ios::out);
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
      if (nn->isInstructionNode () )
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

schedule_result_t
SchedDG::computeScheduleLatency(int graph_level, int detailed)
{
   int i;
   if (HasAllFlags(DG_SCHEDULE_COMPUTED))
   {
      // we've computed the scheduling before, just return the computed value
      return (schedule_result_t(schedule_length, nextScheduleId-1));
   }
   
   if (HasNoFlags(DG_MACHINE_DEFINED))
   {
      // machine was not defined yet, print an error
      fprintf (stderr, "Error in SchedDG::compute_latency, you have to define the target machine before computing the scheduling.\n");
      return (schedule_result_t(-1, 0));
   }
   
   if (HasNoFlags(DG_CYCLES_COMPUTED))  // we did not perform the 
                                         // cycle analysis yet
   {
      findDependencyCycles();
   }
#if DRAW_DEBUG_GRAPH
 if (!targetPath || targetPath==pathId)
 {
   char bbuf1[64];
   sprintf (bbuf1, "t-%s", pathId.Filename());
   draw_debug_graphics (bbuf1); //"x6_T");
 }
#endif
   
   int restrictiveUnit, vecUnit;
   unsigned int totalResources = 0;
   unsigned int min_length = 0;

   unsigned int globalLbCycles = 0;

   unsigned int mm;
   
   int numberOfInnerLoops = 0;
   double vecBound = 0.0;
   
   // mark nodes that are part of address calculations, and those that 
   // compute the loop exit condition
   markLoopBoilerplateNodes();
   
   globalLbCycles = minSchedulingLengthDueToDependencies(); 

   mm = new_marker();
   totalResources = minSchedulingLengthDueToResources(restrictiveUnit, 
                mm, NULL, vecBound, vecUnit);
   
   // we can have any number of barrier nodes. Check how many of these
   // are inner loops. Inner loops do not utilize any resources, but they
   // occupy a clock cycle by themselves (they cannot be overlapped with
   // other instructions from current scope). I must reserve a cycle for
   // each inner loop to account for fragmentation of code.
   if (!barrierNodes.empty())
   {
      NodeList::iterator nlit = barrierNodes.begin();
      for ( ; nlit!=barrierNodes.end() ; ++nlit)
      if ((*nlit)->type == IB_inner_loop)
         numberOfInnerLoops += 1;
   }

#if VERBOSE_DEBUG_SCHEDULER
   const char* resName = 0;
   int unitIdx = 0;
   if (restrictiveUnit>=MACHINE_ASYNC_UNIT_BASE)
      resName = mach->getNameForAsyncResource(restrictiveUnit-MACHINE_ASYNC_UNIT_BASE);
   else if (restrictiveUnit>=0)
      resName = mach->getNameForUnit (restrictiveUnit, unitIdx);
   else
      resName = mach->getNameForRestrictionWithId (- restrictiveUnit);
   DEBUG_SCHED (2, 
      fprintf (stderr, "PathId %s, GLOBAL: Dependency lower bound is %d; resources lower bound is %d, due to unit %d (%s[%d])\n",
          pathId.Name(), globalLbCycles, totalResources, restrictiveUnit, resName, unitIdx);
   )
#endif

#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout <<__func__<<" "<<globalLbCycles<<" "<<totalResources<<" "<<numberOfInnerLoops<<endl;
            )
#endif
   if (globalLbCycles >= totalResources + numberOfInnerLoops)
   {
      min_length = globalLbCycles;
      timeStats.addDependencyTime (globalLbCycles);
   } else
   {
      min_length = totalResources + numberOfInnerLoops;
      timeStats.addResourcesTime (restrictiveUnit, totalResources);
      if (numberOfInnerLoops)
         timeStats.addDependencyTime (numberOfInnerLoops);
   }
   timeStats.addResourcesMinimumTime (totalResources);
   timeStats.addApplicationMinimumTime (globalLbCycles);
   timeStats.addIdealVectorizationTime (vecUnit, vecBound);

#if DRAW_DEBUG_GRAPH
 if (!targetPath || targetPath==pathId)
 {
   char bbuf1[64];
   sprintf (bbuf1, "dag-%s", pathId.Filename());
   cycleDag->draw_debug_graphics (bbuf1); //"x6");
 }
#endif

   // For each non-structure edge, set the resource score as the sum of the
   // resource scores of the two ends; set also the number of neighbors as 
   // the sum of the neighbors of the two end.
   EdgesIterator edit(*this);
   while ((bool)edit)
   {
      if (! edit->IsRemoved() && edit->isSchedulingEdge())
//        (edit->getType()!=STRUCTURE_TYPE))
      {
         Node *_srcN = edit->source();
         Node *_sinkN = edit->sink();
         float edgescore = 0;
         assert (_srcN->isInstructionNode());
         edgescore = _srcN->resourceScore;
         assert (_sinkN->isInstructionNode());
         if (_srcN != _sinkN)
            edgescore += _sinkN->resourceScore;
         edit->resourceScore = edgescore;
         unsigned int numNeighb = _srcN->num_incoming() +
                                  _srcN->num_outgoing();
         if (_srcN != _sinkN)
            numNeighb += _sinkN->num_incoming() +
                         _sinkN->num_outgoing();
         edit->neighbors = numNeighb;
      }
      ++ edit;
   }

   resourceLimited = false;
   schedule_length = min_length;
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout<<__func__<<" sl: "<<schedule_length<<endl;
            )
#endif
   resourceLimitedScore = (float)totalResources / (float)globalLbCycles;
   // 02/15/2012, gmarin: Disable heavyOnResources edge sorting. It leads to DEADLOCK.
   heavyOnResources = false; // (resourceLimitedScore > 2.0);
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (2, 
         fprintf (stderr, "PathId %s, Final: Dependency lower bound is %d; resources lower bound is %d)\n",
             pathId.Name(), globalLbCycles, totalResources);
         fprintf (stderr, "Resource limited score = %f\n",
             resourceLimitedScore);
         fprintf (stderr, "Longest cycle = %d; longest path root to leaf = %d\n",
             longestCycle, maxPathRootToLeaf);
      )
#endif

   // each instruction has its templates sorted by latency (since we computed
   // the latency of each edge - an edge may have a different latency if
   // a different template is used). Anyway, I wanted to make another point
   // which is that probably it would be better if we sorted the templates
   // again based on the global info we have: sorted by latency, or sorted
   // by resource usage (using the global usage to compute a score). 
   // On the other hand it looks like a hassle, so maybe do this optimization
   // later if it looks necessary. Currently, each instruction has its 
   // templates ordered based on their length.

   int numUnits = mach->TotalNumberOfUnits();
   // allocate an array of numUnits size BitSets, with schedule_length entries
   // I need to map the templates of all instructions in the path onto this
   // array.
   bool success = false;
   int lastUnit = -12345;
   Node *lastFNode = NULL;
   Edge *lastFEdge = NULL;
   int lastSegmentId = NO_CYCLE_ID;
   int lastNumTries = 0;
   int *lastUnitUse = new int[numUnits];
   int *unitUse = 0;
   int streak = 0;
   // save the scheduling length attempted before we went on the
   // 3 attempts streak that lead to a deadlock;
   int preDeadlockLength = schedule_length;
   TimeAccount savedTimeStats;
   unsigned int savedTotalResources = totalResources;
   
   for (i=0 ; i<numUnits ; ++i)
      lastUnitUse[i] = 0;

   char file_name[64];
   char file_name_ps[64];
   char title_draw[64];
   
   // Uncomment next two lines to test the fallback scheduling algorithm.
   // Set the targetPath appropriately.
//   if (pathId == targetPath)
//      limpModeScheduling = true;
   
   while (! success)
   {
      int unit = 0;
      Node *fNode = NULL;
      Edge *fEdge = NULL;
      int extraLength;
      int numTries;
      int segmentId;
      schedule_template = new BitSet*[schedule_length];
      for (i=0 ; i<schedule_length ; ++i)
         schedule_template[i] = new BitSet(numUnits);
      success = create_schedule_main (unit, fNode, fEdge, extraLength, 
                    numTries, segmentId);
      if (success)  // schedule succeeded, determine unit usage for this path
         computeUnitUsage(numUnits);
      else   // failure; compute a count of how many times each unit was used
      {
         if (!unitUse)
            unitUse = new int[numUnits];
         for (i=0 ; i<numUnits ; ++i)
            unitUse[i] = 0;
         for (i=0 ; i<schedule_length ; ++i)
         {
            // I must iterate over all the bits that are set and compute sums 
            // accross all template cycles
            BitSet::BitSetIterator bitit(*schedule_template[i]);
            while((bool)bitit)
            {
               unsigned int elem = (unsigned)bitit;
               unitUse[elem] += 1;
               ++ bitit;
            }
         }
      }
         
      for (i=0 ; i<schedule_length ; ++i)
         delete schedule_template[i];
      delete[] schedule_template;

      // in case of succes draw the graph with the complete schedule
      // otherwise, generate a debugging graph with the partial schedule
      // FIXME: Add a flag to decide when to draw these graphs, and for 
      // which paths
      bool make_draw = false;
      if ( (success && (DRAW_GRAPH_ON_SUCCESS & graph_level)) ||
           (!success && (DRAW_GRAPH_ON_FAILURE & graph_level)) )
         make_draw = true;
//      if (pathId==targetPath) make_draw = true;
      if (make_draw)
      {
         char* temp_name = (char*) malloc (64);
         if (! success)
            sprintf (temp_name, "fail_%s", pathId.Filename());
         else
         {
            sprintf (temp_name, "sched_%s", pathId.Filename());
            
            // if successful, adjust also the schedule times such that the 
            // last branch instruction is on the last cycle.
            if (lastBranch != NULL)
            {
               const ScheduleTime& stlb = lastBranch->getScheduleTime ();
               ScheduleTime lastCycle (schedule_length);
               lastCycle.setTime (0,schedule_length-1);
               int deltaTime = lastCycle - stlb;
               if (deltaTime != 0)  // need to adjust times for all nodes
               {
                  NodesIterator allnit(*this);
                  while ((bool)allnit)
                  {
                     Node *a_node = allnit;
                     if (a_node->isInstructionNode ())
                     {
                        assert (a_node->scheduled > 0);
                        a_node->schedTime += deltaTime;
                     }
                     ++ allnit;
                  }
               }
            }
         }
         sprintf(title_draw, "%.10s_%.10s_%s_%03d_%d", routine_name, 
               mach->MachineName(), temp_name, schedule_length, streak);
         sprintf(file_name, "%s.dot", title_draw);
         sprintf(file_name_ps, "%s.ps", title_draw);
         free(temp_name);
         
         ofstream fout(file_name, ios::out);
         assert(fout && fout.good());
         if (fNode)
            fNode->setNodeOfFailure();
         if (fEdge)
            fEdge->setEdgeOfFailure();
         draw_scheduling (fout, title_draw);
         if (fNode)
            fNode->resetNodeOfFailure();
         if (fEdge)
            fEdge->resetEdgeOfFailure();
         fout.close();
         assert(fout.good());
      }
      
      if (! success)
      {
         if (unit<0 || unit>1000)
         {
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (2,
               fprintf(stderr, "PROBLEM?: Found negative bottleneck unit %d / %x\n", unit, unit);
            )
#endif
         }
         // check if extraLength is positive, if not set it to 1
         if (extraLength < 1)
            extraLength = 1;
         // check if segmentId is set, and increase the length of the 
         // corresponding segment (if applicable)
         assert (fNode);
         // I should also check the usage count of all units. I found a case 
         // not inducing of deadlocks, where the same node failed due to the same units
         // more than once, after increasing the schedule length. The problem lies
         // with instructions scheduled before this node. They can use a template that
         // uses the same port as this instruction, or an alternate port. When the schedule
         // was short, they successeded using the alternate port. After the schedule increased,
         // they were scheduled using this instruction's port, negating the effect of the
         // schedule length increase on the port availability.
         // Keep track of how many times we've used each unit during a scheduling attempt.
         if (lastUnit==unit && lastFNode==fNode && lastFEdge==fEdge && 
               lastNumTries==numTries && lastSegmentId==segmentId && 
               !memcmp(lastUnitUse, unitUse, numUnits*sizeof(int))
            )
         {
            ++ streak;
            if (streak>2)   // >= 3
            {
               fflush(stdout);
               fflush(stderr);
               // always print the deadlock message. This is a critical error.
               fprintf (stderr, "Scheduling DEADLOCK for PathId %s: Scheduling failed %d times consecutively due to unit %d, node %p, edge %p, extra length %d, numTries %d, and same unit usage\n",
                      pathId.Name(), streak, unit, fNode, fEdge, extraLength, numTries);
               if (fNode)
                  cerr << "Node of failure: " << (*fNode) << endl;
               if (fEdge)
                  cerr << "Edge of failure: " << (*fEdge) << endl;
               fprintf (stderr, "** Going into limp mode scheduling (list scheduling) for path %s. Pre-deadlock schedule length=%d\n",
                       pathId.Name(), preDeadlockLength);
               limpModeScheduling = true;
               streak = 0;
               schedule_length = preDeadlockLength;
               timeStats = savedTimeStats;
               totalResources = savedTotalResources;
               /**** FIXME TODO *****/
               continue; // ???
//               exit (-5);
            }
         } else
         {
            streak = 0;
            lastUnit = unit;
            lastFNode = fNode;
            lastFEdge = fEdge;
            lastNumTries = numTries;
            lastSegmentId = segmentId;
            memcpy(lastUnitUse, unitUse, numUnits*sizeof(int));
            // save also the old schedule length
            preDeadlockLength = schedule_length;
            savedTimeStats = timeStats;
            savedTotalResources = totalResources;
         }
         if (unit!=DEFAULT_UNIT)
            totalResources += extraLength;
         resourceLimitedScore = (float)totalResources / (float)globalLbCycles;
         // 02/15/2012, gmarin: Disable heavyOnResources edge sorting. It leads to DEADLOCK.
         heavyOnResources = false; // (resourceLimitedScore > 2.0);
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (2,
         fprintf (stderr, "PathId %s, numScheduledNodes=%d, After increase: Dependency lower bound is %d; resources lower bound is %d)\n",
             pathId.Name(), nextScheduleId-1, globalLbCycles, totalResources);
         fprintf (stderr, "Resource limited score = %f\n",
             resourceLimitedScore);
         fprintf (stderr, "Longest cycle = %d; longest path root to leaf = %d\n",
             longestCycle, maxPathRootToLeaf);
      )
#endif
         
//         Edge *superE = NULL;
         bool increased = false;
         int old_schedule_length = schedule_length;

         if (!increased)
         {
            // Make sure that we increase the length by at least
            // 3% of the old value. This makes convergence faster for
            // complex paths with large schedule lengths
            int alternate_extra = ALTERNATE_EXTRA_LENGTH (schedule_length);
            if (alternate_extra > extraLength)
               extraLength = alternate_extra;
            schedule_length += extraLength;
         }
         timeStats.addSchedulingTime (unit, 
                   schedule_length - old_schedule_length, detailed);

#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (1, 
            char uname[128];
            if (unit == DEFAULT_UNIT)
               strcpy(uname, "DefaultUnit");
            else if (unit>=MACHINE_ASYNC_UNIT_BASE)
               snprintf(uname, sizeof(uname), "%s", 
                        mach->getNameForAsyncResource(unit-MACHINE_ASYNC_UNIT_BASE));
            else if (unit>=0)
            {
               const char *tname = 0;
               int unitIdx = -1;
               tname = mach->getNameForUnit(unit, unitIdx);
               snprintf(uname, sizeof(uname), "%s[%d]", tname, unitIdx);
            } else 
               snprintf(uname, sizeof(uname), "%s", 
                        mach->getNameForRestrictionWithId(-unit));
            fprintf (stderr, "Scheduling for path %s: need to increase target length by %d cyles to %d due to unit %d (%s). Longest cycle=%d\n",
                  pathId.Name(), schedule_length - old_schedule_length, 
                  schedule_length, unit, uname, longestCycle);
         )
#endif
      }
   }
   if (unitUse)
      delete[] unitUse;
   if (lastUnitUse)
      delete[] lastUnitUse;
   
   setCfgFlags (DG_SCHEDULE_COMPUTED);
   assert ((double)schedule_length >= vecBound);
   
   // Finally, iterate over all instructions and compute uops types
   countRetiredUops();
   
   return (schedule_result_t(schedule_length, nextScheduleId-1));
}
//--------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------
//ozgurS
//creating my own computeScheduleLatency function to return cpuLatency and memLatency
//
schedule_result_t
SchedDG::myComputeScheduleLatency(int graph_level, int detailed,float &memLatency, float &cpuLatency)
{
   int i;
   if (HasAllFlags(DG_SCHEDULE_COMPUTED))
   {
      // we've computed the scheduling before, just return the computed value
      return (schedule_result_t(schedule_length, nextScheduleId-1));
   }
   
   if (HasNoFlags(DG_MACHINE_DEFINED))
   {
      // machine was not defined yet, print an error
      fprintf (stderr, "Error in SchedDG::compute_latency, you have to define the target machine before computing the scheduling.\n");
      return (schedule_result_t(-1, 0));
   }
   
   if (HasNoFlags(DG_CYCLES_COMPUTED))  // we did not perform the 
                                         // cycle analysis yet
   {
      findDependencyCycles();
   }
#if DRAW_DEBUG_GRAPH
 if (!targetPath || targetPath==pathId)
 {
   char bbuf1[64];
   sprintf (bbuf1, "t-%s", pathId.Filename());
   draw_debug_graphics (bbuf1); //"x6_T");
 }
#endif
   
   int restrictiveUnit, vecUnit;
   unsigned int totalResources = 0;
   unsigned int min_length = 0;

   unsigned int globalLbCycles = 0;

   unsigned int mm;
   
   int numberOfInnerLoops = 0;
   double vecBound = 0.0;
   
   // mark nodes that are part of address calculations, and those that 
   // compute the loop exit condition
   markLoopBoilerplateNodes();
   retValues ret =  myMinSchedulingLengthDueToDependencies(memLatency, cpuLatency); 
   globalLbCycles = ret.ret; 
   memLatency = ret.memret;
   cpuLatency = ret.cpuret;
   mm = new_marker();
   totalResources = minSchedulingLengthDueToResources(restrictiveUnit, 
                mm, NULL, vecBound, vecUnit);
   
   // we can have any number of barrier nodes. Check how many of these
   // are inner loops. Inner loops do not utilize any resources, but they
   // occupy a clock cycle by themselves (they cannot be overlapped with
   // other instructions from current scope). I must reserve a cycle for
   // each inner loop to account for fragmentation of code.
   if (!barrierNodes.empty())
   {
      NodeList::iterator nlit = barrierNodes.begin();
      for ( ; nlit!=barrierNodes.end() ; ++nlit)
      if ((*nlit)->type == IB_inner_loop)
         numberOfInnerLoops += 1;
   }

#if VERBOSE_DEBUG_SCHEDULER
   const char* resName = 0;
   int unitIdx = 0;
   if (restrictiveUnit>=MACHINE_ASYNC_UNIT_BASE)
      resName = mach->getNameForAsyncResource(restrictiveUnit-MACHINE_ASYNC_UNIT_BASE);
   else if (restrictiveUnit>=0)
      resName = mach->getNameForUnit (restrictiveUnit, unitIdx);
   else
      resName = mach->getNameForRestrictionWithId (- restrictiveUnit);
   DEBUG_SCHED (2, 
      fprintf (stderr, "PathId %s, GLOBAL: Dependency lower bound is %d; resources lower bound is %d, due to unit %d (%s[%d])\n",
          pathId.Name(), globalLbCycles, totalResources, restrictiveUnit, resName, unitIdx);
   )
#endif

#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout <<__func__<<" "<<globalLbCycles<<" "<<totalResources<<" "<<numberOfInnerLoops<<endl;
            )
#endif
   if (globalLbCycles >= totalResources + numberOfInnerLoops)
   {
      min_length = globalLbCycles;
      timeStats.addDependencyTime (globalLbCycles);
   } else
   {
      min_length = totalResources + numberOfInnerLoops;
      timeStats.addResourcesTime (restrictiveUnit, totalResources);
      if (numberOfInnerLoops)
         timeStats.addDependencyTime (numberOfInnerLoops);
   }
   timeStats.addResourcesMinimumTime (totalResources);
   timeStats.addApplicationMinimumTime (globalLbCycles);
   timeStats.addIdealVectorizationTime (vecUnit, vecBound);

#if DRAW_DEBUG_GRAPH
 if (!targetPath || targetPath==pathId)
 {
   char bbuf1[64];
   sprintf (bbuf1, "dag-%s", pathId.Filename());
   cycleDag->draw_debug_graphics (bbuf1); //"x6");
 }
#endif

   // For each non-structure edge, set the resource score as the sum of the
   // resource scores of the two ends; set also the number of neighbors as 
   // the sum of the neighbors of the two end.
   EdgesIterator edit(*this);
   while ((bool)edit)
   {
      if (! edit->IsRemoved() && edit->isSchedulingEdge())
//        (edit->getType()!=STRUCTURE_TYPE))
      {
         Node *_srcN = edit->source();
         Node *_sinkN = edit->sink();
         float edgescore = 0;
         assert (_srcN->isInstructionNode());
         edgescore = _srcN->resourceScore;
         assert (_sinkN->isInstructionNode());
         if (_srcN != _sinkN)
            edgescore += _sinkN->resourceScore;
         edit->resourceScore = edgescore;
         unsigned int numNeighb = _srcN->num_incoming() +
                                  _srcN->num_outgoing();
         if (_srcN != _sinkN)
            numNeighb += _sinkN->num_incoming() +
                         _sinkN->num_outgoing();
         edit->neighbors = numNeighb;
      }
      ++ edit;
   }

   resourceLimited = false;
   schedule_length = min_length;
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout<<__func__<<" sl: "<<schedule_length<<endl;
            )
#endif
   resourceLimitedScore = (float)totalResources / (float)globalLbCycles;
   // 02/15/2012, gmarin: Disable heavyOnResources edge sorting. It leads to DEADLOCK.
   heavyOnResources = false; // (resourceLimitedScore > 2.0);
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (2, 
         fprintf (stderr, "PathId %s, Final: Dependency lower bound is %d; resources lower bound is %d)\n",
             pathId.Name(), globalLbCycles, totalResources);
         fprintf (stderr, "Resource limited score = %f\n",
             resourceLimitedScore);
         fprintf (stderr, "Longest cycle = %d; longest path root to leaf = %d\n",
             longestCycle, maxPathRootToLeaf);
      )
#endif

   // each instruction has its templates sorted by latency (since we computed
   // the latency of each edge - an edge may have a different latency if
   // a different template is used). Anyway, I wanted to make another point
   // which is that probably it would be better if we sorted the templates
   // again based on the global info we have: sorted by latency, or sorted
   // by resource usage (using the global usage to compute a score). 
   // On the other hand it looks like a hassle, so maybe do this optimization
   // later if it looks necessary. Currently, each instruction has its 
   // templates ordered based on their length.

   int numUnits = mach->TotalNumberOfUnits();
   // allocate an array of numUnits size BitSets, with schedule_length entries
   // I need to map the templates of all instructions in the path onto this
   // array.
   bool success = false;
   int lastUnit = -12345;
   Node *lastFNode = NULL;
   Edge *lastFEdge = NULL;
   int lastSegmentId = NO_CYCLE_ID;
   int lastNumTries = 0;
   int *lastUnitUse = new int[numUnits];
   int *unitUse = 0;
   int streak = 0;
   // save the scheduling length attempted before we went on the
   // 3 attempts streak that lead to a deadlock;
   int preDeadlockLength = schedule_length;
//ozgurS adding return variable for cpu latency
   int preDeadlockLengthCPU = cpuLatency;
//ozgurE
   TimeAccount savedTimeStats;
   unsigned int savedTotalResources = totalResources;
   
   for (i=0 ; i<numUnits ; ++i)
      lastUnitUse[i] = 0;

   char file_name[64];
   char file_name_ps[64];
   char title_draw[64];
   
   // Uncomment next two lines to test the fallback scheduling algorithm.
   // Set the targetPath appropriately.
//   if (pathId == targetPath)
//      limpModeScheduling = true;
   
   while (! success)
   {
      int unit = 0;
      Node *fNode = NULL;
      Edge *fEdge = NULL;
      int extraLength;
      int numTries;
      int segmentId;
      schedule_template = new BitSet*[schedule_length];
      for (i=0 ; i<schedule_length ; ++i)
         schedule_template[i] = new BitSet(numUnits);
      success = create_schedule_main (unit, fNode, fEdge, extraLength, 
                    numTries, segmentId);
      if (success)  // schedule succeeded, determine unit usage for this path
         computeUnitUsage(numUnits);
      else   // failure; compute a count of how many times each unit was used
      {
         if (!unitUse)
            unitUse = new int[numUnits];
         for (i=0 ; i<numUnits ; ++i)
            unitUse[i] = 0;
         for (i=0 ; i<schedule_length ; ++i)
         {
            // I must iterate over all the bits that are set and compute sums 
            // accross all template cycles
            BitSet::BitSetIterator bitit(*schedule_template[i]);
            while((bool)bitit)
            {
               unsigned int elem = (unsigned)bitit;
               unitUse[elem] += 1;
               ++ bitit;
            }
         }
      }
         
      for (i=0 ; i<schedule_length ; ++i)
         delete schedule_template[i];
      delete[] schedule_template;

      // in case of succes draw the graph with the complete schedule
      // otherwise, generate a debugging graph with the partial schedule
      // FIXME: Add a flag to decide when to draw these graphs, and for 
      // which paths
      bool make_draw = false;
      if ( (success && (DRAW_GRAPH_ON_SUCCESS & graph_level)) ||
           (!success && (DRAW_GRAPH_ON_FAILURE & graph_level)) )
         make_draw = true;
//      if (pathId==targetPath) make_draw = true;
      if (make_draw)
      {
         char* temp_name = (char*) malloc (64);
         if (! success)
            sprintf (temp_name, "fail_%s", pathId.Filename());
         else
         {
            sprintf (temp_name, "sched_%s", pathId.Filename());
            
            // if successful, adjust also the schedule times such that the 
            // last branch instruction is on the last cycle.
            if (lastBranch != NULL)
            {
               const ScheduleTime& stlb = lastBranch->getScheduleTime ();
               ScheduleTime lastCycle (schedule_length);
               lastCycle.setTime (0,schedule_length-1);
               int deltaTime = lastCycle - stlb;
               if (deltaTime != 0)  // need to adjust times for all nodes
               {
                  NodesIterator allnit(*this);
                  while ((bool)allnit)
                  {
                     Node *a_node = allnit;
                     if (a_node->isInstructionNode ())
                     {
                        assert (a_node->scheduled > 0);
                        a_node->schedTime += deltaTime;
                     }
                     ++ allnit;
                  }
               }
            }
         }
         sprintf(title_draw, "%.10s_%.10s_%s_%03d_%d", routine_name, 
               mach->MachineName(), temp_name, schedule_length, streak);
         sprintf(file_name, "%s.dot", title_draw);
         sprintf(file_name_ps, "%s.ps", title_draw);
         free(temp_name);
         
         ofstream fout(file_name, ios::out);
         assert(fout && fout.good());
         if (fNode)
            fNode->setNodeOfFailure();
         if (fEdge)
            fEdge->setEdgeOfFailure();
         draw_scheduling (fout, title_draw);
         if (fNode)
            fNode->resetNodeOfFailure();
         if (fEdge)
            fEdge->resetEdgeOfFailure();
         fout.close();
         assert(fout.good());
      }
      
      if (! success)
      {
         if (unit<0 || unit>1000)
         {
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (2,
               fprintf(stderr, "PROBLEM?: Found negative bottleneck unit %d / %x\n", unit, unit);
            )
#endif
         }
         // check if extraLength is positive, if not set it to 1
         if (extraLength < 1)
            extraLength = 1;
         // check if segmentId is set, and increase the length of the 
         // corresponding segment (if applicable)
         assert (fNode);
         // I should also check the usage count of all units. I found a case 
         // not inducing of deadlocks, where the same node failed due to the same units
         // more than once, after increasing the schedule length. The problem lies
         // with instructions scheduled before this node. They can use a template that
         // uses the same port as this instruction, or an alternate port. When the schedule
         // was short, they successeded using the alternate port. After the schedule increased,
         // they were scheduled using this instruction's port, negating the effect of the
         // schedule length increase on the port availability.
         // Keep track of how many times we've used each unit during a scheduling attempt.
         if (lastUnit==unit && lastFNode==fNode && lastFEdge==fEdge && 
               lastNumTries==numTries && lastSegmentId==segmentId && 
               !memcmp(lastUnitUse, unitUse, numUnits*sizeof(int))
            )
         {
            ++ streak;
            if (streak>2)   // >= 3
            {
               fflush(stdout);
               fflush(stderr);
               // always print the deadlock message. This is a critical error.
               fprintf (stderr, "Scheduling DEADLOCK for PathId %s: Scheduling failed %d times consecutively due to unit %d, node %p, edge %p, extra length %d, numTries %d, and same unit usage\n",
                      pathId.Name(), streak, unit, fNode, fEdge, extraLength, numTries);
               if (fNode)
                  cerr << "Node of failure: " << (*fNode) << endl;
               if (fEdge)
                  cerr << "Edge of failure: " << (*fEdge) << endl;
               fprintf (stderr, "** Going into limp mode scheduling (list scheduling) for path %s. Pre-deadlock schedule length=%d\n",
                       pathId.Name(), preDeadlockLength);
               limpModeScheduling = true;
               streak = 0;
               schedule_length = preDeadlockLength;
//ozgurS setting back cpu latency to original
               cpuLatency = preDeadlockLengthCPU;
//ozgurE
               timeStats = savedTimeStats;
               totalResources = savedTotalResources;
               /**** FIXME TODO *****/
               continue; // ???
//               exit (-5);
            }
         } else
         {
            streak = 0;
            lastUnit = unit;
            lastFNode = fNode;
            lastFEdge = fEdge;
            lastNumTries = numTries;
            lastSegmentId = segmentId;
            memcpy(lastUnitUse, unitUse, numUnits*sizeof(int));
            // save also the old schedule length
            preDeadlockLength = schedule_length;
            savedTimeStats = timeStats;
            savedTotalResources = totalResources;
         }
         if (unit!=DEFAULT_UNIT)
            totalResources += extraLength;
         resourceLimitedScore = (float)totalResources / (float)globalLbCycles;
         // 02/15/2012, gmarin: Disable heavyOnResources edge sorting. It leads to DEADLOCK.
         heavyOnResources = false; // (resourceLimitedScore > 2.0);
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (2,
         fprintf (stderr, "PathId %s, numScheduledNodes=%d, After increase: Dependency lower bound is %d; resources lower bound is %d)\n",
             pathId.Name(), nextScheduleId-1, globalLbCycles, totalResources);
         fprintf (stderr, "Resource limited score = %f\n",
             resourceLimitedScore);
         fprintf (stderr, "Longest cycle = %d; longest path root to leaf = %d\n",
             longestCycle, maxPathRootToLeaf);
      )
#endif
        //FIXME chech if it need to be increased ozgur 
//         Edge *superE = NULL;
         bool increased = false;
         int old_schedule_length = schedule_length;

         if (!increased)
         {
            // Make sure that we increase the length by at least
            // 3% of the old value. This makes convergence faster for
            // complex paths with large schedule lengths
            int alternate_extra = ALTERNATE_EXTRA_LENGTH (cpuLatency); //ozgur original was this(schedule_length);
            if (alternate_extra > extraLength)
               extraLength = alternate_extra;
            schedule_length += extraLength;
//ozgurS calculating CPU Latency
            cpuLatency += extraLength;
//ozgurE
         }
         timeStats.addSchedulingTime (unit, 
                   schedule_length - old_schedule_length, detailed);

#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (1, 
            char uname[128];
            if (unit == DEFAULT_UNIT)
               strcpy(uname, "DefaultUnit");
            else if (unit>=MACHINE_ASYNC_UNIT_BASE)
               snprintf(uname, sizeof(uname), "%s", 
                        mach->getNameForAsyncResource(unit-MACHINE_ASYNC_UNIT_BASE));
            else if (unit>=0)
            {
               const char *tname = 0;
               int unitIdx = -1;
               tname = mach->getNameForUnit(unit, unitIdx);
               snprintf(uname, sizeof(uname), "%s[%d]", tname, unitIdx);
            } else 
               snprintf(uname, sizeof(uname), "%s", 
                        mach->getNameForRestrictionWithId(-unit));
            fprintf (stderr, "Scheduling for path %s: need to increase target length by %d cyles to %d due to unit %d (%s). Longest cycle=%d\n",
                  pathId.Name(), schedule_length - old_schedule_length, 
                  schedule_length, unit, uname, longestCycle);
         )
#endif
      }
   }
   if (unitUse)
      delete[] unitUse;
   if (lastUnitUse)
      delete[] lastUnitUse;
   
   setCfgFlags (DG_SCHEDULE_COMPUTED);
   assert ((double)schedule_length >= vecBound);
   
   // Finally, iterate over all instructions and compute uops types
   countRetiredUops();
   
   return (schedule_result_t(schedule_length, nextScheduleId-1));
}


//---------------------------------------------------------------------------------------------------------------------



void
SchedDG::countRetiredUops()
{
   int i;
   int64_t uopTypes[UOP_NUM_TYPES];
   for (i=0 ; i<UOP_NUM_TYPES ; ++i)
      uopTypes[i] = 0;
      
   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      if (nn->isInstructionNode ())
      {
         RetiredUopType rut = UOP_INVALID;
         if (nn->is_nop_instruction())
            rut = UOP_NOP;
         else if (nn->is_scalar_stack_reference())
            rut = UOP_SPILL_UNSPILL;
         else if (nn->isLoopCondition())
            rut = UOP_LOOP_COND;
         else if (nn->isAddressCalculation())
            rut = UOP_ADDRESS_CALC;
         else
            rut = nn->getRetiredUopType();
         uopTypes[(int)rut] += 1;
      }
      ++ nit;
   }
   for (i=0 ; i<UOP_NUM_TYPES ; ++i)
      if (uopTypes[i] > 0)
         timeStats.addRetiredUops(i, uopTypes[i]);
}

int 
SchedDG::computeUnitUsage(int numUnits)
{
   int i;
   int* usage = new int[numUnits];
   for (i=0 ; i<numUnits ; ++i)
      usage[i] = 0;
   
   for (i=0 ; i<schedule_length ; ++i)
   {
      // I must iterate over all the bits that are set and compute sums 
      // accross all template cycles
      BitSet::BitSetIterator bitit(*schedule_template[i]);
      while((bool)bitit)
      {
         unsigned int elem = (unsigned)bitit;
         usage[elem] += 1;
         ++ bitit;
      }
   }
   
   unitUsage.addExecutionTime(schedule_length);
   for(i=0 ; i<numUnits ; ++i)
      if (usage[i]>0)
         unitUsage.addResourceUsage(i, usage[i]);
   return (0);
}

int
SchedDG::find_memory_parallelism (CliqueList &allCliques, 
           HashMapUI &refsDist2Use)
{
//   draw_debug_graphics ("x6_V");
   // I do not divide the nodes in explicit intervals. If I have multiple
   // barrier nodes then I should still have super-structures between 
   // consecutive barrier node. But I do not have an easy method of 
   // recognizing nodes part of a interval, since super-structures are 
   // hierarchical and the sub-structures have different ids.
   // Try without intervals and if it becomes too complex on some graphs,
   // change such that I can identify the intervals. I could check if a node
   // is part of a top interval by finding its top super-edge.
   // 03/22/2007 mgabi: I will reintroduce the method with intervals. Some
   // routines are really large and breaking them in these intervals makes
   // things more efficient. An easy way to identify nodes part of an interval
   // is to compare the node level with the levels of the two barrier nodes.
   if (!barrierNodes.empty() && barrierNodes.front()!=barrierNodes.back())
   {
      NodeList::iterator nlit = barrierNodes.begin();
      Node* firstNode = *nlit;
      Node* lNode = firstNode;   // lower node
      
      for ( ; nlit!=barrierNodes.end() ; )
      {
         ++ nlit;
         Node *uNode;
//         int iters = 0, level = 0;
         if (nlit!=barrierNodes.end())
            uNode = *nlit;   // upper node
         else
         {
            uNode = firstNode;
//            iters = level = 1;
         }
         cerr << "Search for interval from " << *lNode << ", LEVEL="
              << lNode->_level << " to " << *uNode << ", LEVEL=" 
              << uNode->_level << endl;

#if 0  // I am not using super edges for identification; Use levels instead
         // should I find a super edge even if I have a single barrier node 
         // in the entire path? I think not as I construct super edges now. 
         // That will be identified as a regular cycle. I will restrict this
         // method only to cases when I have at least two barrier nodes.
         Edge* superE = lNode->findOutgoingEdge (uNode, OUTPUT_DEPENDENCY,
             SUPER_EDGE_TYPE, iters, level);
         assert (superE != NULL);
#endif
         
         find_memory_parallelism_for_interval (lNode->_level, uNode->_level,
                    allCliques, refsDist2Use);
         lNode = uNode;
      }
   } else   // at most one barrier node; use only the global method
   {
      find_memory_parallelism_for_interval (0, 0, allCliques, refsDist2Use);
   }
   return (0);
}
//--------------------------------------------------------------------------------------------------------------------

// compute memory parallelism only for nodes with levels between minLev and
// maxLev, exclusive.
int
SchedDG::find_memory_parallelism_for_interval (int minLev, int maxLev, 
              CliqueList &allCliques, HashMapUI &refsDist2Use)
{
   int i;
   // assign an index to each memory reference; for this I need to build a
   // mapping from the index to the instruction's pc so I can recover their
   // identity later.
   NodeArray refMapping;
   // I need also mapping from Node to index
   NodeIntMap nodeIndex;
   
   // should I build also an array of lists, recording the memory instructions
   // issued in each cycle? Record only the index of each instruction?
   UIList **refsArray = new UIList* [schedule_length];
   int nextRefIndex = 0;
   memset (refsArray, 0, schedule_length*sizeof(UIList *) );
   
   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      if (nn->isInstructionNode () && nn->is_load_instruction () &&
           ( (!minLev && !maxLev) || (minLev<nn->_level && nn->_level<maxLev)
               || (minLev>maxLev && (nn->_level>minLev || nn->_level<maxLev))
           ))
//             (superEdge==NULL || superEdge==nn->superE) )
      {
         int crtIdx = nextRefIndex ++;
         refMapping.push_back (nn);
         // get the clock cycle of this instruction
         int schedCycle = nn->schedTime.ClockCycle();
         if (refsArray[schedCycle]==NULL)  // check if first ref at this cycle
            refsArray[schedCycle] = new UIList();
         refsArray[schedCycle]->push_back (crtIdx);
         // I should not have seen this node before
         assert (nodeIndex.find (nn) == nodeIndex.end ());
         nodeIndex.insert (NodeIntMap::value_type (nn, crtIdx));
      }
      ++ nit;
   }
   
   // if we did not find any load instruction, return right away
   if (! nextRefIndex)
   {
      cerr << "find_memory_parallelism: Did not find any load instructions "
           << "in this path." << endl;
      return (0);
   }
      
   // We parsed all the instructions. We have 'nextRefIndex' references in 
   // this scope. Now I should build the adjaicency matrix for the parallelism
   // graph and call the clique finding routine.
   int Nsquared = nextRefIndex * nextRefIndex;
   char *connected = new char[Nsquared];
   memset (connected, 0, Nsquared * sizeof(char));
   char *parGraph = new char[Nsquared];
   memset (parGraph, 0, Nsquared * sizeof(char));
   // initialize the diagonal with 1 (the algorithm expects that)
   int baseIdx = 0, stride = nextRefIndex+1;
   for (i=0 ; i<nextRefIndex ; ++i, baseIdx+=stride)
      parGraph[baseIdx] = 1;
   
   // Now, the more difficult part is to actually fill the connectivity 
   // matrix with the correct values for the non trivial cases
   UIList::iterator ulit;
   for (i=0 ; i<schedule_length ; ++i)
   {
      if (refsArray[i]!=NULL)
         for (ulit=refsArray[i]->begin() ; ulit!=refsArray[i]->end() ; ++ulit)
         {
            int idx1 = *ulit;
            assert (idx1>=0 && idx1<nextRefIndex);
            Node *node1 = refMapping[idx1];
            assert (node1->isInstructionNode() && node1->isScheduled() &&
                node1->is_load_instruction() );
            unsigned int nodePc = node1->address;
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (7, 
               cerr << "Processing " << *node1 << " at PC " << hex 
                    << nodePc << dec << endl;
            )
#endif
            unsigned int &dist2use = refsDist2Use [nodePc];
            
            // find the earliest instruction that depends on this load
            // For in-order machines, this defines the upper bound for 
            // what instructions can be in parallel with current load.
            OutgoingEdgesIterator oeit (node1);
            int minLen = -1;
            while ((bool) oeit)
            {
               if (!oeit->IsRemoved() && (oeit->getType()==GP_REGISTER_TYPE
                                || oeit->getType()==ADDR_REGISTER_TYPE))
               {
                  // a candidate dependency; check out its length
                  int edist = oeit->getDistance();
                  int diffCycles;
                  if (edist == 0)
                     diffCycles = oeit->sink()->schedTime - 
                                  node1->schedTime;
                  else
                     diffCycles = (oeit->sink()->schedTime % edist) - 
                                   node1->schedTime;
                  if (diffCycles < 0)
                  {
                     cerr << "ERROR: found an edge with the ends badly scheduled: "
                          << *((Edge*)oeit) << endl;
                     assert (diffCycles >= 0);
                  }
                  if (minLen<0 || minLen>diffCycles)
                     minLen = diffCycles;
               }
               ++ oeit;
            }
            // at this point minLen should positive. Is it possible to have
            // a load instructions without any outgoing register dependency?
            // Maybe for exit paths only, if the use of the load is in the 
            // loop's epilog. Assert for now and check if it fails.
            // Found another scenario. Loop with multiple iterations, but has 
            // an inner loop. The use of the load may be in the inner loop.
            // Argh, another case when the load instruction is a prefetch and
            // does not have a destination register.
            // Another case in a loop with multiple paths, where blocks are
            // skipped. The load is moved before the branch, but the use is 
            // not on all paths. How to test such a case? Remove the assert?
//            assert (minLen>=0 || avgNumIters<=1 || !barrierNodes.empty() ||
//               (node1->gpRegDest.empty() && node1->fpRegDest.empty()));
            if (minLen < 0)
            {
               minLen = 16;
            }
            // if this is a straight piece of code (no SWP), check if
            // we exceed the length of the scheduling.
            if (avgNumIters<=1 && (i+minLen) > schedule_length)
               // this can happen only when I did not find an actual register
               // dependency and I set an arbitrary value
               minLen = schedule_length - i;
            // Also, for simplicity, I should limit parallelism only to 
            // instructions that are less than one full iteration apart
            if (minLen > schedule_length)
               minLen = schedule_length;
            // if there are barrier nodes, we must not search further than the
            // next barrier node
            Node *nextBarrier = nextBarrierNode[node1->_level];
            if (nextBarrier != NULL)
            {
               assert (nextBarrier->isScheduled());
               int diffCycles = nextBarrier->schedTime.ClockCycle() - 
                       node1->schedTime.ClockCycle();
               if (diffCycles < 0)
                  diffCycles += schedule_length;
               assert (diffCycles >= 0);
               assert (diffCycles < schedule_length);
               // add '1' to diffCycles, because we consider also other
               // references in the current clock cycle
               diffCycles += 1;
               if (minLen > diffCycles)
                  minLen = diffCycles;
            }
            dist2use = minLen;
            
            // I need a limited DFS search to tell me which instructions are
            // dependent on a given instruction
            unsigned int mm = new_marker();
#if VERBOSE_DEBUG_SCHEDULER
            DEBUG_SCHED (7, 
               cerr << "Call markDependentRefs for " << *node1 << ". Num refs="
                    << nextRefIndex << "; upper bound=" << minLen << endl;
            )
#endif
            markDependentRefs (idx1, node1, connected, nextRefIndex, 
                      nodeIndex, minLen, 0, mm);
            
            int clk = i;
            int j;
            register int baseAddr = idx1 * nextRefIndex;
            UIList::iterator ulit2 = ulit;
            ++ ulit2;
            for (j=0 ; j<minLen ; ++j)
            {
               if (refsArray[clk]!=NULL)
                  for ( ; ulit2!=refsArray[clk]->end() ; ++ulit2)
                  {
                     int idx2 = *ulit2;
                     assert (idx2>=0 && idx2<nextRefIndex);
                     assert (idx2 != idx1);
                     Node *node2 = refMapping[idx2];
                     assert (node2->isInstructionNode() && 
                             node2->isScheduled() &&
                             node2->is_load_instruction() );
                     if (connected [baseAddr + idx2]==0)
                     {
                        parGraph [baseAddr + idx2] = 1;
                        parGraph [idx2*nextRefIndex + idx1] = 1;
                     } else
                        if (parGraph [baseAddr + idx2] == 1)
                        {
                           // they were found independent before (must be from
                           // a search starting at idx2), but now we found a
                           // dependency chain from idx1 to idx2
#if VERBOSE_DEBUG_SCHEDULER
                           DEBUG_SCHED (0, 
                              cerr << "Warning: in find_parallel_refs, found dependent refs Idx1="
                                   << idx1 << " - " << *node1 << "; and Idx2=" 
                                   << idx2 << " - " << *node2 
                                   << ", but they were independent before" << endl;
                           )
#endif
                           // should I consider them in parallel in such a case
                           // or should I think of them as dependent?
                           // Leave them marked as in parallel for now
                           // When determining the initiation interval between 
                           // two references, consider the shortest distance
                           // arround (if a SWP case), otherwise there is only
                           // one choice.
                        }
                  }
               clk += 1;
               if (clk >= schedule_length)
                  clk = 0;
               if (refsArray[clk] != NULL)
                  ulit2 = refsArray[clk]->begin ();
            }
         }
   }
   // print the parGraph vector
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (8, 
   {
      cerr << "parGraph matrix:" << endl;
      for (i=0 ; i<nextRefIndex ; ++i)
      {
         for (int j=0 ; j<nextRefIndex ; ++j)
            cerr << (int)parGraph[i*nextRefIndex+j] << " ";
         cerr << endl;
      }
   }
   )
#endif
   // parGraph is constructed; call the clique finding routine
   CliqueList cliques;
   FindCliques fc(nextRefIndex, parGraph);
#if VERBOSE_DEBUG_SCHEDULER
   int compcost = 
#endif
        fc.compute_cliques (cliques);

#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (1, 
   {
      fprintf (stderr, "In compute_cliques, function 'extend' was called %d times.\n",
              compcost);
      fprintf (stderr, "Found %ld cliques:\n", cliques.size());
   }
   )
#endif
   CliqueList::iterator cait = cliques.begin();
   int cnt = 0;
   for ( ; cait!=cliques.end() ; ++cait, ++cnt)
   {
      int size = (*cait)->num_nodes;
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (6, 
         fprintf (stderr, "%-3d) Size %d:", cnt, size);
      )
#endif
      for (i=0 ; i<size ; ++i)
      {
         int idx = (*cait)->vertices[i];
         assert (idx>=0 && idx<nextRefIndex);
         Node *nn = refMapping[idx];
#if VERBOSE_DEBUG_SCHEDULER
         DEBUG_SCHED (6, 
            fprintf (stderr, "  %d (Node %d, pc 0x%lx, type %s)",
                 idx, nn->id, nn->address, 
                 Convert_InstrBin_to_string ((InstrBin) (nn->type)));
         )
#endif
         // convert the local index to pc address
         (*cait)->vertices[i] = nn->address;
      }
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (6, 
         fprintf (stderr, "\n");
      )
#endif
   }
   // deallocate the cliques; I need to save them in a file together with 
   // the schedule info eventually
   allCliques.splice (allCliques.end(), cliques);
      
   for (i=0 ; i<schedule_length ; ++i)
      if (refsArray[i]!=NULL)
         delete refsArray[i];
   delete [] refsArray;
   delete [] connected;
   delete [] parGraph;
   return (0);
}
//--------------------------------------------------------------------------------------------------------------------

void
SchedDG::markDependentRefs (int refIdx, Node *node, char *connected, int N, 
        NodeIntMap &nodeIndex, int uBound, int crtDist, unsigned int mm)
{
   if (crtDist>=uBound) return;
   // check if we visited this node before
   if (node->visited (mm))
      return;
   node->visit (mm);
   int nidx = -1;
   if (node->isInstructionNode () && node->is_load_instruction ())
   {
      // a candidate instruction; check that it has an index, and mark it in
      // the connected array
      NodeIntMap::iterator nit = nodeIndex.find (node);
      // I should not check beyond the end of this segment (if we have 
      // segments). This means that current ref should have an assigned index.
      assert (nit != nodeIndex.end() );
      nidx = nit->second;
      assert (nidx >= 0);
      if (refIdx != nidx)
      {
         connected [refIdx*N+nidx] = 1;
         connected [nidx*N+refIdx] = 1;
      }
   }
   OutgoingEdgesIterator oeit(node);
   while ((bool) oeit)
   {
      Edge *ee = oeit;
      // if this dependency is not removed, is valid (any type for most 
      // instruction types, but inly register dependencies for references)
      if (!ee->IsRemoved() && ee->isSchedulingEdge() &&
            (nidx==(-1) || ee->getType()==GP_REGISTER_TYPE ||
             ee->getType()==ADDR_REGISTER_TYPE) )
      {
         // this a valid dependency to follow
         // compute how much distance it travels in out schedule
         Node *destNode = ee->sink();
         int dist = (destNode->schedTime % ee->getDistance()) - 
                       node->schedTime;
         markDependentRefs (refIdx, destNode, connected, N, nodeIndex, uBound,
                  crtDist + dist, mm);
      }
      ++ oeit;
   }
}
//--------------------------------------------------------------------------------------------------------------------

int
SchedDG::minSchedulingLengthDueToDependencies()
{
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout<<__func__<<" here"<<endl;
            )
#endif
   // compute several metrics for each node
   // 1. longest cycle (=1 if no cycle, or avgNumIters<=1)
   // 2. sum of the lengths of all cycles this node is part of (=1 if no cycle)
   // 3. longest distance from a root node
   // 4. longest distance to a leaf node
   // for each edge, compute the length of the longest path root to leaf that
   // it is part of
   assert (HasAllFlags (DG_CYCLES_COMPUTED));
   assert (HasNoFlags (DG_GRAPH_IS_MODIFIED));
   
   bool manyBarriers = false;
   if (!barrierNodes.empty() && barrierNodes.front()!=barrierNodes.back())
      manyBarriers = true;
   // to compute pathToLeaf, do a DFS traversal from all top nodes
   OutgoingEdgesIterator teit(cfg_top);
   unsigned int mm = new_marker();
   unsigned int maxPathToLeaf = 0;
   while ((bool)teit)
   {
      unsigned int pathLen = 
            teit->sink()->computePathToLeaf(mm, manyBarriers, ds);
      if (maxPathToLeaf < pathLen)
         maxPathToLeaf = pathLen;
      ++ teit;
   }
   
   // to compute pathFromRoot, do a DFS traversal from all bottom nodes
   IncomingEdgesIterator beit(cfg_bottom);
   mm = new_marker();
   unsigned int maxPathFromRoot = 0;
   while ((bool)beit)
   {
      unsigned int pathLen = 
            beit->source()->computePathFromRoot (mm, manyBarriers, ds);
      if (maxPathFromRoot < pathLen)
         maxPathFromRoot = pathLen;
      ++ beit;
   }

#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout<<__func__<<" "<<maxPathFromRoot<<" "<<maxPathToLeaf<<endl;
            )
#endif
   
   // a leaf instruction is at distance one from scheduling end, because it
   // takes one cycle to issue it; however, a root instruction is at distance
   // 0 from scheduling start because it can be scheduled immediately (on the
   // first cycle).
   if (maxPathToLeaf != maxPathFromRoot + 1)
   {
      fprintf(stderr, "Path %s: maxPathToLeaf %d, maxPathFromRoot %d\n",
            pathId.Name(), maxPathToLeaf, maxPathFromRoot);
      assert (maxPathToLeaf == maxPathFromRoot + 1);
   }
   maxPathRootToLeaf = maxPathToLeaf; 
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout<<__func__<<" "<<maxPathRootToLeaf<<" "<<maxPathToLeaf<<endl;
            )
#endif
   // For each forward loop-independent edge, set the length of the longest
   // path from root to leaf that includes it
   // Check also if it is part of a path between two cycles.
   UIPairSet cycleSetEdges;
   EdgesIterator edit(*this);
   while ((bool)edit)
   {
      if (! edit->IsRemoved() && edit->isSchedulingEdge() )
      {
         Node *srcNode = edit->source ();
         Node *sinkNode = edit->sink ();
         if (edit->getLevel()==0)
         {
            
            int leng = edit->getLatency();
            if (srcNode->isInstructionNode())
               leng += srcNode->pathFromRoot;
            if (sinkNode->isInstructionNode())
               leng += sinkNode->pathToLeaf;
            assert (leng <= maxPathRootToLeaf);
            edit->longestPath = leng;
            
            if ( edit->hasCycles==0 &&
                 (srcNode->hasCycles==3 || srcNode->edgesFromCycle>0) &&
                 (sinkNode->hasCycles==3 || sinkNode->edgesToCycle>0) )
            {
               edit->hasCycles = 2;
               if (srcNode->hasCycles==0)
                  srcNode->hasCycles = 2;
               if (sinkNode->hasCycles==0)
                  sinkNode->hasCycles = 2;
               // insert all pairs of (inCycleSets, outCycleSets) into a map
               // and then create the respective edges into the DAG
               UISet inputSet;
               UISet outputSet;
               if (srcNode->hasCycles == 3)
               {
                  unsigned int cycId = srcNode->getSccId(); // ds->Find (srcNode->id);
                  inputSet.insert (cycId);
               } else
                  inputSet = srcNode->inCycleSets;
               if (sinkNode->hasCycles == 3)
               {
                  unsigned int cycId = sinkNode->getSccId(); // ds->Find (sinkNode->id);
                  outputSet.insert (cycId);
               } else
                  outputSet = sinkNode->outCycleSets;
               UISet::iterator init = inputSet.begin ();
               UISet::iterator outit;
               for ( ; init!=inputSet.end() ; ++init)
                  for (outit=outputSet.begin() ; outit!=outputSet.end() ; ++outit)
                     cycleSetEdges.insert (UIPair (*init, *outit));
            }
         }
//         edit->longestCycle = 0;
//         edit->sumAllCycles = 0;
         
         // for each scheduling edge, set also an additional priority field,
         // which gives higher priority to edges entering load instructions, 
         // and a lower priority to edges leaving loads. In this way I want to
         // ensure a maximum overlap between cache misses and computation.
         edit->extraPriority = 5;
         if (sinkNode->is_load_instruction())
            edit->extraPriority += 5;
         if (srcNode->is_load_instruction())
            edit->extraPriority -= 5;
         // if an edge has load instructions at both its ends, then it will
         // have the "normal" priority.
      }
      ++ edit;
   }
   // traverse cycleSetEdges and insert the corresponding edges into the DAG
   UIPairSet::iterator upit = cycleSetEdges.begin ();
   for ( ; upit!=cycleSetEdges.end() ; ++upit)
      // because we use a set of pairs, we do not have duplicates, so we
      // can use the cheaper addEdge instead of addUniqueEdge
      cycleDag->addEdge (upit->first, upit->second);
   
   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      // 03/30/2007 mgabi: Try having maxRemaining for nodes from one
      // direction only. We use it only to select the starting node anyway.
      // But in general it makes more sense to schedule just from one end
      // when we do not have cycles. If we start from both sides, we may end
      // up with problems.
      if (nn->isInstructionNode ())
      {
         nn->maxRemaining = nn->pathFromRoot;
         nn->remainingEdges = nn->edgesFromRoot;
#if 0
         if (nn->pathFromRoot > nn->pathToLeaf)
         {
            nn->maxRemaining = nn->pathFromRoot;
            nn->remainingEdges = nn->edgesFromRoot;
         }
         else
         {
            nn->maxRemaining = nn->pathToLeaf;
            nn->remainingEdges = nn->edgesToLeaf;
         }
#endif
      }
      ++ nit;
   }

   // create also the path slack structures
   // FOR starters we'll have slack info only for cycles, included in Cycle
   // class. usedSlack is reset to 0 at the start of each scheduling attempt.
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout<<__func__<<" "<<maxPathRootToLeaf<<"ozgur: longestCyle "<<longestCycle<<endl;
            )
#endif
   if (avgNumIters>ONE_ITERATION_EPSILON || longestCycle>1)
   {
      return (longestCycle);
   }
   else
   {
      return (maxPathRootToLeaf);
   }
}
//ozgurS
//creating my minschedulinglengthduetodependencies function

retValues
SchedDG::myMinSchedulingLengthDueToDependencies(float &memLatency, float &cpuLatency)
{
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout<<__func__<<" here"<<endl;
            )
#endif
   // compute several metrics for each node
   // 1. longest cycle (=1 if no cycle, or avgNumIters<=1)
   // 2. sum of the lengths of all cycles this node is part of (=1 if no cycle)
   // 3. longest distance from a root node
   // 4. longest distance to a leaf node
   // for each edge, compute the length of the longest path root to leaf that
   // it is part of
   assert (HasAllFlags (DG_CYCLES_COMPUTED));
   assert (HasNoFlags (DG_GRAPH_IS_MODIFIED));
   
   bool manyBarriers = false;
   if (!barrierNodes.empty() && barrierNodes.front()!=barrierNodes.back())
      manyBarriers = true;
   // to compute pathToLeaf, do a DFS traversal from all top nodes
   OutgoingEdgesIterator teit(cfg_top);
   unsigned int mm = new_marker();
   unsigned int maxPathToLeaf = 0;
   while ((bool)teit)
   {
      retValues ret = 
            teit->sink()->myComputePathToLeaf(mm, manyBarriers, ds, memLatency, cpuLatency);
      unsigned int pathLen = ret.ret;
      int memLatencyTemp = ret.memret;
      int cpuLatencyTemp = ret.cpuret;
      if (maxPathToLeaf < pathLen)
      {
         memLatency = memLatencyTemp;
         cpuLatency = cpuLatencyTemp;
         maxPathToLeaf = pathLen;
      }
      ++ teit;
   }
   
   // to compute pathFromRoot, do a DFS traversal from all bottom nodes
   IncomingEdgesIterator beit(cfg_bottom);
   mm = new_marker();
   unsigned int maxPathFromRoot = 0;
   while ((bool)beit)
   {
      unsigned int pathLen = 
            beit->source()->computePathFromRoot (mm, manyBarriers, ds);
      if (maxPathFromRoot < pathLen)
         maxPathFromRoot = pathLen;
      ++ beit;
   }

#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout<<__func__<<" "<<maxPathFromRoot<<" "<<maxPathToLeaf<<endl;
            )
#endif
   
   // a leaf instruction is at distance one from scheduling end, because it
   // takes one cycle to issue it; however, a root instruction is at distance
   // 0 from scheduling start because it can be scheduled immediately (on the
   // first cycle).
   if (maxPathToLeaf != maxPathFromRoot + 1)
   {
      fprintf(stderr, "Path %s: maxPathToLeaf %d, maxPathFromRoot %d\n",
            pathId.Name(), maxPathToLeaf, maxPathFromRoot);
      assert (maxPathToLeaf == maxPathFromRoot + 1);
   }
   maxPathRootToLeaf = maxPathToLeaf; 
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout<<__func__<<" "<<maxPathRootToLeaf<<" "<<maxPathToLeaf<<endl;
            )
#endif
   // For each forward loop-independent edge, set the length of the longest
   // path from root to leaf that includes it
   // Check also if it is part of a path between two cycles.
   UIPairSet cycleSetEdges;
   EdgesIterator edit(*this);
   while ((bool)edit)
   {
      if (! edit->IsRemoved() && edit->isSchedulingEdge() )
      {
         Node *srcNode = edit->source ();
         Node *sinkNode = edit->sink ();
         if (edit->getLevel()==0)
         {
            
            int leng = edit->getLatency();
            if (srcNode->isInstructionNode())
               leng += srcNode->pathFromRoot;
            if (sinkNode->isInstructionNode())
               leng += sinkNode->pathToLeaf;
            assert (leng <= maxPathRootToLeaf);
            edit->longestPath = leng;
            
            if ( edit->hasCycles==0 &&
                 (srcNode->hasCycles==3 || srcNode->edgesFromCycle>0) &&
                 (sinkNode->hasCycles==3 || sinkNode->edgesToCycle>0) )
            {
               edit->hasCycles = 2;
               if (srcNode->hasCycles==0)
                  srcNode->hasCycles = 2;
               if (sinkNode->hasCycles==0)
                  sinkNode->hasCycles = 2;
               // insert all pairs of (inCycleSets, outCycleSets) into a map
               // and then create the respective edges into the DAG
               UISet inputSet;
               UISet outputSet;
               if (srcNode->hasCycles == 3)
               {
                  unsigned int cycId = srcNode->getSccId(); // ds->Find (srcNode->id);
                  inputSet.insert (cycId);
               } else
                  inputSet = srcNode->inCycleSets;
               if (sinkNode->hasCycles == 3)
               {
                  unsigned int cycId = sinkNode->getSccId(); // ds->Find (sinkNode->id);
                  outputSet.insert (cycId);
               } else
                  outputSet = sinkNode->outCycleSets;
               UISet::iterator init = inputSet.begin ();
               UISet::iterator outit;
               for ( ; init!=inputSet.end() ; ++init)
                  for (outit=outputSet.begin() ; outit!=outputSet.end() ; ++outit)
                     cycleSetEdges.insert (UIPair (*init, *outit));
            }
         }
//         edit->longestCycle = 0;
//         edit->sumAllCycles = 0;
         
         // for each scheduling edge, set also an additional priority field,
         // which gives higher priority to edges entering load instructions, 
         // and a lower priority to edges leaving loads. In this way I want to
         // ensure a maximum overlap between cache misses and computation.
         edit->extraPriority = 5;
         if (sinkNode->is_load_instruction())
            edit->extraPriority += 5;
         if (srcNode->is_load_instruction())
            edit->extraPriority -= 5;
         // if an edge has load instructions at both its ends, then it will
         // have the "normal" priority.
      }
      ++ edit;
   }
   // traverse cycleSetEdges and insert the corresponding edges into the DAG
   UIPairSet::iterator upit = cycleSetEdges.begin ();
   for ( ; upit!=cycleSetEdges.end() ; ++upit)
      // because we use a set of pairs, we do not have duplicates, so we
      // can use the cheaper addEdge instead of addUniqueEdge
      cycleDag->addEdge (upit->first, upit->second);
   
   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      // 03/30/2007 mgabi: Try having maxRemaining for nodes from one
      // direction only. We use it only to select the starting node anyway.
      // But in general it makes more sense to schedule just from one end
      // when we do not have cycles. If we start from both sides, we may end
      // up with problems.
      if (nn->isInstructionNode ())
      {
         nn->maxRemaining = nn->pathFromRoot;
         nn->remainingEdges = nn->edgesFromRoot;
#if 0
         if (nn->pathFromRoot > nn->pathToLeaf)
         {
            nn->maxRemaining = nn->pathFromRoot;
            nn->remainingEdges = nn->edgesFromRoot;
         }
         else
         {
            nn->maxRemaining = nn->pathToLeaf;
            nn->remainingEdges = nn->edgesToLeaf;
         }
#endif
      }
      ++ nit;
   }

   // create also the path slack structures
   // FOR starters we'll have slack info only for cycles, included in Cycle
   // class. usedSlack is reset to 0 at the start of each scheduling attempt.
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout<<__func__<<" "<<maxPathRootToLeaf<<"ozgur: longestCyle "<<longestCycle<<endl;
            )
#endif
   retValues returnThis;
   returnThis.memret=memLatency;
   returnThis.cpuret=cpuLatency;
   if (avgNumIters>ONE_ITERATION_EPSILON || longestCycle>1)
   {
      returnThis.ret = (longestCycle);
      returnThis.memret = longestCycleMem;
      returnThis.cpuret = longestCycleCPU;
   }
   else
   {
      returnThis.ret= (maxPathRootToLeaf);
   }


   return returnThis;
}
//ozgurE
//--------------------------------------------------------------------------------------------------------------------

int
SchedDG::Node::computePathToLeaf (unsigned int marker, bool manyBarriers, 
             DisjointSet *ds)
{
   // because I follow only loop independent edges, I cannot have cycles
   if (marker == _visited)  // I saw this node before
   {
      return (pathToLeaf);
   }
   
   // I see this node for the first time
   _visited = marker;
   pathToLeaf = 1;
   edgesToLeaf = 0;   // num edges associated with the longest path
   edgesToCycle = 0;
   pathToCycle = 0;
   outCycleSets.clear ();
   
   OutgoingEdgesIterator oeit(this);
   while ((bool) oeit)
   {
      // follow only loop independent dependencies; also, follow all
      // memory dependencies, but only true dependencies of other types.
      // For example anti- and output- register dependencies are removed
      // by register renaming; control dependencies are only true-
      if (! oeit->IsRemoved() && oeit->getLevel()==0 && 
              oeit->isSchedulingEdge() )
      {
         Node *sinkNode = oeit->sink ();
         int edgeLat = oeit->getLatency ();
#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout <<__func__<<" "<<(unsigned int*)oeit->source()->getAddress()<<" "<<(unsigned int*)oeit->sink()->getAddress()<< edgeLat<<endl;
            )
#endif
         int rez = sinkNode->computePathToLeaf (marker, 
                      manyBarriers, ds);
         int rezEdges = sinkNode->edgesToLeaf;
         if ((edgeLat+rez > pathToLeaf) || edgesToLeaf==0)
         {
            pathToLeaf = edgeLat+rez;
//            edgesToLeaf = sinkNode->edgesToLeaf + 1;
         }
         if (edgesToLeaf==0 || edgesToLeaf<=rezEdges)
            edgesToLeaf = rezEdges + 1;
         
         // when computing segments that connect two different cycles, I 
         // should avoid cycles formed by super edges (when there are at least
         // two barrier nodes). I also do not care for paths connecting cycles
         // that are in different segments (barrier intervals).
         if (hasCycles==0 && (sinkNode->edgesToCycle>0 || 
                  (sinkNode->hasCycles==3 && 
                        (!sinkNode->isBarrierNode() || !manyBarriers))) )
         {
            // if the sink node is a member of a cycle, add only the
            // id of that cycle and do not test the outCycleSet of the sink.
            if (sinkNode->hasCycles == 3)
            {
               unsigned int sinkCycId = sinkNode->getSccId(); // ds->Find (sinkNode->id);
               outCycleSets.insert (sinkCycId);
            } else
            {
               UISet &sinkSet = sinkNode->outCycleSets;
               outCycleSets.insert (sinkSet.begin(), sinkSet.end());
            }
            if ((edgeLat+sinkNode->pathToCycle>pathToCycle) || 
                       edgesToCycle==0)
            {
               pathToCycle = edgeLat + sinkNode->pathToCycle;
               edgesToCycle = sinkNode->edgesToCycle + 1;
            }
         }
      }
      ++ oeit;
   }
   return (pathToLeaf);
}
//ozgurS
//adding my computePathToLeaf
//

retValues
SchedDG::Node::myComputePathToLeaf (unsigned int marker, bool manyBarriers, 
             DisjointSet *ds, float &memLatency, float &cpuLatency)
{
   // because I follow only loop independent edges, I cannot have cycles
   if (marker == _visited)  // I saw this node before
   {
//      memLatency = pathToLeafMem;
//      cpuLatency = pathToLeafCPU;
      retValues ret;
      ret.ret = pathToLeaf;
      ret.memret = pathToLeafMem;
      ret.cpuret = pathToLeafCPU;
      return (ret);
   }
   
   // I see this node for the first time
   _visited = marker;
   pathToLeaf = 1;
   edgesToLeaf = 0;   // num edges associated with the longest path
   edgesToCycle = 0;
   pathToCycle = 0;
   pathToLeafCPU = 1;
   pathToLeafMem = 0;
   outCycleSets.clear ();
   
   OutgoingEdgesIterator oeit(this);
   while ((bool) oeit)
   {
      // follow only loop independent dependencies; also, follow all
      // memory dependencies, but only true dependencies of other types.
      // For example anti- and output- register dependencies are removed
      // by register renaming; control dependencies are only true-
      if (! oeit->IsRemoved() && oeit->getLevel()==0 && 
              oeit->isSchedulingEdge() )
      {
         Node *sinkNode = oeit->sink ();
         int edgeLat = oeit->getLatency ();
         int edgeCPULat = oeit->getCPULatency ();
         int edgeMemLat = oeit->getMemLatency ();

#if VERBOSE_DEBUG_PALM
      DEBUG_PALM(1,
      cout <<__func__<<" "<<(unsigned int*)oeit->source()->getAddress()<<" "<<(unsigned int*)oeit->sink()->getAddress()<< edgeLat<<endl;
            )
#endif
         retValues rez = sinkNode->myComputePathToLeaf (marker, 
                      manyBarriers, ds, memLatency, cpuLatency );
         int rezEdges = sinkNode->edgesToLeaf;
         if ((edgeLat+rez.ret > pathToLeaf) || edgesToLeaf==0)
         {
    //        memLatency = edgeMemLat+rez.memret;
    //        cpuLatency = edgeCPULat+rez.cpuret;
    //        pathToLeafCPU = cpuLatency;
    //        pathToLeafMem = memLatency;
            pathToLeafCPU = edgeCPULat+rez.cpuret;
            pathToLeafMem = edgeMemLat+rez.memret; 
            pathToLeaf = edgeLat+rez.ret;
//            edgesToLeaf = sinkNode->edgesToLeaf + 1;
         }
         if (edgesToLeaf==0 || edgesToLeaf<=rezEdges)
            edgesToLeaf = rezEdges + 1;
         
         // when computing segments that connect two different cycles, I 
         // should avoid cycles formed by super edges (when there are at least
         // two barrier nodes). I also do not care for paths connecting cycles
         // that are in different segments (barrier intervals).
         if (hasCycles==0 && (sinkNode->edgesToCycle>0 || 
                  (sinkNode->hasCycles==3 && 
                        (!sinkNode->isBarrierNode() || !manyBarriers))) )
         {
            // if the sink node is a member of a cycle, add only the
            // id of that cycle and do not test the outCycleSet of the sink.
            if (sinkNode->hasCycles == 3)
            {
               unsigned int sinkCycId = sinkNode->getSccId(); // ds->Find (sinkNode->id);
               outCycleSets.insert (sinkCycId);
            } else
            {
               UISet &sinkSet = sinkNode->outCycleSets;
               outCycleSets.insert (sinkSet.begin(), sinkSet.end());
            }
            if ((edgeLat+sinkNode->pathToCycle>pathToCycle) || 
                       edgesToCycle==0)
            {
               pathToCycle = edgeLat + sinkNode->pathToCycle;
               edgesToCycle = sinkNode->edgesToCycle + 1;
            }
         }
      }
      ++ oeit;
   }
   retValues ret;
   ret.ret=pathToLeaf;
   ret.cpuret=pathToLeafCPU;
   ret.memret=pathToLeafMem;

   return (ret);
}

//--------------------------------------------------------------------------------------------------------------------

int
SchedDG::Node::computePathFromRoot (unsigned int marker, bool manyBarriers,
             DisjointSet *ds)
{
   // because I follow only loop independent edges, I cannot have cycles
   if (marker == _visited)  // I saw this node before
   {
      return (pathFromRoot);
   }
   
   // I see this node for the first time
   _visited = marker;
   pathFromRoot = 0;
   edgesFromRoot = 0;   // num edges associated with the longest path
   edgesFromCycle = 0;
   pathFromCycle = 0;
   inCycleSets.clear ();
   
   IncomingEdgesIterator ieit(this);
   while ((bool) ieit)
   {
      // follow only loop independent dependencies; also, follow all
      // memory dependencies, but only true dependencies of other types.
      // For example anti- and output- register dependencies are removed
      // by register renaming; control dependencies are only true-
      if (! ieit->IsRemoved() && ieit->getLevel()==0 && 
              ieit->isSchedulingEdge() )
      {
         Node *srcNode = ieit->source ();
         int edgeLat = ieit->getLatency ();
         int rez = ieit->source()->computePathFromRoot (marker,
                    manyBarriers, ds);
         int rezEdges = ieit->source()->edgesFromRoot;
         if (edgeLat+rez > pathFromRoot || edgesFromRoot==0)
         {
            pathFromRoot = edgeLat+rez;
//            edgesFromRoot = ieit->source()->edgesFromRoot + 1;
         }
         if (edgesFromRoot==0 || edgesFromRoot<=rezEdges)
            edgesFromRoot = rezEdges + 1;

         if (hasCycles==0 && (srcNode->edgesFromCycle>0 || 
                  (srcNode->hasCycles==3 && 
                       (!srcNode->isBarrierNode() || !manyBarriers))) )
         {
            // if the source node is a member of a cycle, add only the
            // id of that cycle and do not test the inCycleSets of the src.
            if (srcNode->hasCycles == 3)
            {
               unsigned int srcCycId = srcNode->getSccId(); // ds->Find (srcNode->id);
               inCycleSets.insert (srcCycId);
            } else
            {
               UISet &srcSet = srcNode->inCycleSets;
               inCycleSets.insert (srcSet.begin(), srcSet.end());
            }
            if ((edgeLat+srcNode->pathFromCycle>pathFromCycle) || 
                        edgesFromCycle==0)
            {
               pathFromCycle = edgeLat + srcNode->pathFromCycle;
               edgesFromCycle = srcNode->edgesFromCycle + 1;
            }
         }
      }
      ++ ieit;
   }
   return (pathFromRoot);
}
//--------------------------------------------------------------------------------------------------------------------
#if USE_SCC_PATHS
int
SchedDG::Node::computeSccPathToLeaf (unsigned int marker, int sccId)
{
   // because I follow only loop independent edges, I cannot have cycles
   if (marker == _visited)  // I saw this node before
   {
      return (sccPathToLeaf);
   }
   
   // I see this node for the first time
   _visited = marker;
   sccPathToLeaf = 1;
   sccEdgesToLeaf = 0;   // num edges associated with the longest path
   
   OutgoingEdgesIterator oeit(this);
   while ((bool) oeit)
   {
      // follow only loop independent dependencies; also, follow all
      // memory dependencies, but only true dependencies of other types.
      // For example anti- and output- register dependencies are removed
      // by register renaming; control dependencies are only true-
      if (! oeit->IsRemoved() && oeit->getLevel()==0 && 
              oeit->isSchedulingEdge() && oeit->sink()->getSccId()==sccId )
      {
         Node *sinkNode = oeit->sink();
         int edgeLat = oeit->getLatency();
         int rez = sinkNode->computeSccPathToLeaf (marker, sccId);
         int rezEdges = sinkNode->sccEdgesToLeaf;
         if ((edgeLat+rez > sccPathToLeaf) || sccEdgesToLeaf==0)
         {
            sccPathToLeaf = edgeLat+rez;
//            sccEdgesToLeaf = sinkNode->sccEdgesToLeaf + 1;
         }
         if (sccEdgesToLeaf==0 || sccEdgesToLeaf<=rezEdges)
            sccEdgesToLeaf = rezEdges + 1;
      }
      ++ oeit;
   }
   return (sccPathToLeaf);
}
//--------------------------------------------------------------------------------------------------------------------

int
SchedDG::Node::computeSccPathFromRoot (unsigned int marker, int sccId)
{
   // because I follow only loop independent edges, I cannot have cycles
   if (marker == _visited)  // I saw this node before
   {
      return (sccPathFromRoot);
   }
   
   // I see this node for the first time
   _visited = marker;
   sccPathFromRoot = 0;
   sccEdgesFromRoot = 0;   // num edges associated with the longest path
   
   IncomingEdgesIterator ieit(this);
   while ((bool) ieit)
   {
      // follow only loop independent dependencies; also, follow all
      // memory dependencies, but only true dependencies of other types.
      // For example anti- and output- register dependencies are removed
      // by register renaming; control dependencies are only true-
      if (! ieit->IsRemoved() && ieit->getLevel()==0 && 
              ieit->isSchedulingEdge() && ieit->source()->getSccId()==sccId)
      {
         Node *srcNode = ieit->source();
         int edgeLat = ieit->getLatency();
         int rez = srcNode->computeSccPathFromRoot(marker, sccId);
         int rezEdges = srcNode->sccEdgesFromRoot;
         if (edgeLat+rez > sccPathFromRoot || sccEdgesFromRoot==0)
         {
            sccPathFromRoot = edgeLat+rez;
//            sccEdgesFromRoot = ieit->source()->sccEdgesFromRoot + 1;
         }
         if (sccEdgesFromRoot==0 || sccEdgesFromRoot<=rezEdges)
            sccEdgesFromRoot = rezEdges + 1;
      }
      ++ ieit;
   }
   return (sccPathFromRoot);
}
#endif  // USE_SCC_PATHS
//--------------------------------------------------------------------------------------------------------------------

typedef std::pair<InstructionClass, int> ICIntPair;
class ICIntPairCompare
{
public:
   bool operator() (const ICIntPair &ic1, const ICIntPair &ic2) const
   {
      if (ic1.first.equals(ic2.first))
         return (ic1.second < ic2.second);
      else
         return (icc(ic1.first, ic2.first));
   }
private:
   InstructionClassCompare icc;
};
typedef std::map<ICIntPair, int64_t, ICIntPairCompare> ICVecMap;
typedef std::map<int, double> IntDoubleMap;

// bottleneckUnit will receive the index of the unit that causes the lower
// bound. If the index is >= 0, this is the index of a CPU unit. If the 
// value is < 0, this is the negated value of a units restriction rule.
int
SchedDG::minSchedulingLengthDueToResources(int& bottleneckUnit, 
          unsigned int marker, Edge* superEdge, double& vecBound, int& vecUnit)
{
   assert (HasAllFlags (DG_MACHINE_DEFINED));
   int i, j;
   int numInstClasses = mach->NumberOfInstructionClasses();
//   IB_TOP_MARKER;
   int numUnits = mach->TotalNumberOfUnits();
   int numAsyncUnits = mach->NumberOfAsyncResources();
   double maxValue = 0., maxVecValue = 0.;
   const RestrictionList* restrictions = mach->Restrictions();
   unsigned int lbDueToResources; // holds the lower bound due to resources
   int mostUsedUnit, mostUsedVecUnit; // indicates which cpu unit causes the bound

  /** HERE: Change iCounts into a map; Count how many instructions
   ** of a given type are in the path
   ** Anything else that must be changed?
   **/
   // count how many instructions of each type are in this path
   ICIMap iCounts;
   
   // we must compute also a bound if we could achieve perfect vectorization
   // for this machine
   // Keep separate counts for vectorizable instructions, and for instructions
   // that should not be vectorized and that should not be duplicated if we
   // unroll the loop (address calculations, instructions asociated
   // with the loop condition and index increment).
   ICVecMap vecCounts;
   ICIMap nonVecCounts;
   IntDoubleMap instMap;  // I need this map to estimate the number of instruction after vectorization
   double boilerplateInsts = 0.0;
   int maxVecFactor = 1;
   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      if (nn->isInstructionNode() &&  // cfg_top has type == -1
             (superEdge==NULL || superEdge==nn->superE) )
      {
         int vwidth = 0;
         if (nn->exec_unit == ExecUnit_VECTOR)
            vwidth = nn->vec_len * nn->bit_width;
         Instruction *inst = mach->InstructionOfType((InstrBin)nn->type, nn->exec_unit, vwidth,
                   nn->exec_unit_type, nn->bit_width);
         if (inst == NULL)
         {
            fprintf (stderr, "Error: Could not find any machine description instruction type that matches the native instruction at address 0x%" PRIxaddr " of type: %s-%s-%s-%" PRIwidth "-veclen=%d\n",
                nn->address,
                Convert_InstrBin_to_string((InstrBin)nn->type), ExecUnitToString(nn->exec_unit),
                ExecUnitTypeToString(nn->exec_unit_type), nn->bit_width, nn->vec_len);
            assert (inst != NULL);
         }
         const InstructionClass &nic = inst->getIClass();
         ICIMap::iterator imit = iCounts.find (nic);
         if (imit != iCounts.end())
            imit->second += 1;
         else
            iCounts.insert(ICIMap::value_type(nic, 1));

         // obtain also the template of the longest vector length, to compute the
         // improvement potential from perfect vectorization
         // TODO improvements: test which nodes correspond to address calculations,
         // loop end condition (branch, compare, index increment), and mark them as
         // "vectorization agnostic". I should not duplicate these instructions
         // if I pretend that I unroll the loop to support vectorization.
         if (nn->bit_width==0 || nn->isLoopBoilerplate() || nn->is_nop_instruction())
         {
            boilerplateInsts += 1./nn->getNumUopsInInstruction();
            
            imit = nonVecCounts.find (nic);
            if (imit != nonVecCounts.end())
               imit->second += 1;
            else
               nonVecCounts.insert(ICIMap::value_type(nic, 1));
         } else   // check if we have a longer vector template
         {
            Instruction *vecInst = mach->InstructionOfLongestVectorType((InstrBin)nn->type, 
                      nn->exec_unit, vwidth, nn->exec_unit_type, nn->bit_width);
            // we hould never get a NULL template here, because I return the
            // normal template if I cannot find a vector one, and above I  
            // handle the NULL template case with an abort
            assert (vecInst != NULL);
            if (nn->exec_unit == ExecUnit_SCALAR)
               vwidth = nn->bit_width;
            const InstructionClass &vic = vecInst->getIClass();
            // did we find a vector longer than the width defined by the instructio type
            int vec_factor = 1;  // how many instructions can I pack in a vector?
            if (vwidth < vic.vec_width)
            {
               vec_factor = vic.vec_width / vwidth;
               // it should divide exactly, and the result should be at least 1
               // gmarin, 2013/10/15: Hmm, it doesn't always divide exactly.
               // Consider the case of x87 arithmetic. Fix the machine model 
               // to specify the element size for the vector templates.
               // Also, print a more informative error message here.
               if (vec_factor<1 ||
                       (vwidth*vec_factor)!=vic.vec_width)
               {
                  cerr << "Vectorization: Instruction of type " << nic.ToString()
                       << " -> found vector template " << vic.ToString()
                       << " but vector length " << vic.vec_width
                       << " does not divide by element width " << vwidth
                       << " or vec_factor=" << vec_factor << " is < 1" << endl;
                  assert (vec_factor>=1 && 
                          (vwidth*vec_factor)==vic.vec_width);
               }
            }
            if (vec_factor > maxVecFactor)
               maxVecFactor = vec_factor;
            ICIntPair vPair(vic, vec_factor);
            ICVecMap::iterator icvit = vecCounts.find (vPair);
            if (icvit != vecCounts.end())
               icvit->second += 1;
            else
               vecCounts.insert(ICVecMap::value_type(vPair, 1));
            // keep track of how many instructions we've seen
            instMap[vec_factor] += 1./nn->getNumUopsInInstruction();
         }
      }
      ++ nit;
   }
   float *resourceUsage = new float[numUnits];
   float *tempRU = new float[numUnits];
   float *vecResourceUsage = new float[numUnits];
   float *asyncUsage=0, *vecAsyncUsage=0;
   
   for (i=0 ; i<numUnits; ++i)
   {
      resourceUsage[i] = 0.f;
      tempRU[i] = 0.f;
      vecResourceUsage[i] = 0.f;
   }

   if (numAsyncUnits)
   { 
      asyncUsage = new float[numAsyncUnits];
      vecAsyncUsage = new float[numAsyncUnits];
      for (i=0 ; i<numAsyncUnits; ++i)
      {
         asyncUsage[i] = vecAsyncUsage[i] = 0.f;
      }
   }
   
   const InstructionClass* instIndex = mach->getInstructionsIndex();
   const int* unitsIndex = mach->getUnitsIndex();
   IntDoubleMap::iterator idit = instMap.begin();
   for ( ; idit!=instMap.end() ; ++idit)
      boilerplateInsts += idit->second * maxVecFactor / idit->first;
   int numVecInstructions = (int)round(boilerplateInsts);
      
   for (i=0 ; i<numInstClasses ; ++i)
   {
      if (instIndex[i].isValid())
      {
         ICIMap::iterator imit = iCounts.find(instIndex[i]);
         if (imit!=iCounts.end() && imit->second>0)  // should always be > 0 if found, just a sanity check
         {
            float* rez = mach->InstructionOfType(instIndex[i])->
                       getBalancedResourceUsage (resourceUsage, numUnits, 
                            unitsIndex, imit->second, restrictions, 
                            numAsyncUnits, asyncUsage);
            assert (rez != NULL || !"Instruction::getBalancedResourceUsage returned NULL");
         }
         
         // compute resource usage for the ideal vectorization case
         ICIntPair vPair(instIndex[i], 0);
         ICVecMap::iterator icvit = vecCounts.lower_bound(vPair);
         for ( ; icvit!=vecCounts.end() && icvit->first.first.equals(instIndex[i]) ; ++icvit)
         {
            assert (icvit->first.second>=1);
            assert (icvit->second > 0);
            assert (maxVecFactor % icvit->first.second == 0);
            int numInstances = icvit->second * maxVecFactor / icvit->first.second;
//            numVecInstructions += numInstances;
            float* rez = mach->InstructionOfType(instIndex[i])->
                       getBalancedResourceUsage (vecResourceUsage, numUnits, 
                            unitsIndex, numInstances, restrictions,
                            numAsyncUnits, vecAsyncUsage);
            assert (rez != NULL || !"Instruction::getBalancedResourceUsage returned NULL");
         }
         // now add the cost of "unvectorizable" instructions
         imit = nonVecCounts.find(instIndex[i]);
         if (imit!=nonVecCounts.end() && imit->second>0)  // should always be > 0 if found, just a sanity check
         {
//            numVecInstructions += imit->second;
            float* rez = mach->InstructionOfType(instIndex[i])->
                       getBalancedResourceUsage (vecResourceUsage, numUnits, 
                            unitsIndex, imit->second, restrictions,
                            numAsyncUnits, vecAsyncUsage);
            assert (rez != NULL || !"Instruction::getBalancedResourceUsage returned NULL");
         }
      }
   }
   // end of use for iCounts

#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (7, 
      fprintf (stderr, "Resource Low Bound for path %s: resource usage of all units:\n",
             pathId.Name());
      for (i=0 ; i<numUnits ; ++i)
         fprintf (stderr, "(%d) %5.3f, ", i, resourceUsage[i]);
      fprintf (stderr, "\n");
      for (i=0 ; i<numAsyncUnits ; ++i)
         fprintf (stderr, "Async(%d) %5.3f, ", i, asyncUsage[i]);
      fprintf (stderr, "\n");

      fprintf (stderr, "Resource Low Bound for path %s with Ideal Vectorization and unrolled %d times: resource usage of all units:\n",
             pathId.Name(), maxVecFactor);
      for (i=0 ; i<numUnits ; ++i)
         fprintf (stderr, "(%d) %5.3f, ", i, vecResourceUsage[i]);
      fprintf (stderr, "\n");
      for (i=0 ; i<numAsyncUnits ; ++i)
         fprintf (stderr, "Async(%d) %5.3f, ", i, vecAsyncUsage[i]);
      fprintf (stderr, "\n");
   )
#endif

   // I have computed resource usage for entire path at this point
   // Compute a score for each node (higher means more restrictive - high prio)
   nit.Reset();
   int countInst = 0;
   while ((bool)nit)
   {
      Node *nn = nit;
      if (nn->isInstructionNode() &&   // cfg_top has type == -1
              (superEdge==NULL || superEdge==nn->superE) )
      {
         float score = 0;
         ++ countInst;
         int vwidth = 0;
         if (nn->exec_unit == ExecUnit_VECTOR)
            vwidth = nn->vec_len * nn->bit_width;
         Instruction *inst = mach->InstructionOfType((InstrBin)nn->type, nn->exec_unit, vwidth,
                   nn->exec_unit_type, nn->bit_width);
         assert (inst != NULL);  // this should not happen. I have a similar check two loops up
         // which should assert first on such a condition with a more informative error message
         float* rez = inst->getResourceUsage (tempRU, numUnits, unitsIndex);
         assert (rez != NULL || !"Instruction::getResourceUsage returned NULL");
         for (j=0 ; j<numUnits ; ++j)
            if (rez[j] != 0)
               score += (resourceUsage[j]*rez[j]*rez[j]);
         nn->resourceScore = score;
         assert (!nn->visited(marker));
         nn->visit (marker);
      }
      ++ nit;
   }


#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (5, 
      fprintf (stderr, "There are %d instructions in path %s.\n", 
             countInst, pathId.Name());
   )
#endif
   delete[] tempRU;
   
   // find most used resource
   mostUsedUnit = mostUsedVecUnit = 0;
   for (i=0 ; i<numUnits ; ++i)
   {
      if (resourceUsage[i]>maxValue)
      {
         maxValue = resourceUsage[i];
         mostUsedUnit = i;
      }
      
      if (vecResourceUsage[i]>maxVecValue)
      {
         maxVecValue = vecResourceUsage[i];
         mostUsedVecUnit = i;
      }
   }
   // check also the asynchronous resources
   for (i=0 ; i<numAsyncUnits ; ++i)
   {
      // add usage of asynchronous resource to the database.
      // Async resources do not depend on the final instruction schedule
      if (asyncUsage[i]>0)
         unitUsage.addResourceUsage(i+MACHINE_ASYNC_UNIT_BASE, asyncUsage[i]);
      
      int asyncCount = mach->getMultiplicityOfAsyncResource(i);
      if (asyncUsage[i]>maxValue*asyncCount)
      {
         maxValue = asyncUsage[i] / asyncCount;
         mostUsedUnit = i + MACHINE_ASYNC_UNIT_BASE;
      }

      if (vecAsyncUsage[i]>maxVecValue*asyncCount)
      {
         maxVecValue = vecAsyncUsage[i] / asyncCount;
         mostUsedVecUnit = i + MACHINE_ASYNC_UNIT_BASE;
      }
   }
   // for all restriction that are active, check if they restrict the 
   // lower bound even more
   // A retirement restriction (optional) is marked as not active because it must 
   // only be checked here. We can detect it by checking the unit list is NULL;
   int max_retire = 0;
   int retire_rule_id = 0;
   RestrictionList::const_iterator rlit = restrictions->begin();
   for ( ; rlit!=restrictions->end() ; ++rlit)
   {
      if ((*rlit)->UnitsList() == 0)  // retirement restriction
      {
         max_retire = (*rlit)->MaxCount();
         retire_rule_id = (*rlit)->getRuleId();
      }
         
      // if the rule is not really restrictive do not check it
      if ( ! (*rlit)->IsActive())
         continue;
      double sumUsage = 0., vecSumUsage = 0.;
      TEUList *uList = (*rlit)->UnitsList();
      TEUList::iterator teuit = uList->begin();
      for ( ; teuit!=uList->end() ; ++teuit)
      {
         TemplateExecutionUnit *teu = *teuit;
         int baseIdx = unitsIndex[teu->index];
         BitSet::BitSetIterator bitit(*(teu->unit_mask));
         while ((bool)bitit)
         {
            int elem = bitit;
            sumUsage += resourceUsage[baseIdx+elem];
            vecSumUsage += vecResourceUsage[baseIdx+elem];
            ++ bitit;
         }
      }
      int maxC = (*rlit)->MaxCount();
      assert (maxC > 0);
      sumUsage /= maxC;
      vecSumUsage /= maxC;
      if (sumUsage > maxValue)
      {
         maxValue = sumUsage;
         mostUsedUnit = - (*rlit)->getRuleId();
      }
      if (vecSumUsage > maxVecValue)
      {
         maxVecValue = vecSumUsage;
         mostUsedVecUnit = - (*rlit)->getRuleId();
      }
   }
   
   // Do not apply retirement limit if this is a short loop with few average iterations
   // FIXME: use a function of window size instead of the constant 100 or whatever
   float numIters = avgNumIters;
   if (numIters>1.0f) numIters -= 1.0f;
   if (max_retire>0 && num_instructions>0 && num_instructions*numIters>80)
   {
      double retire_limit = (double)num_instructions / max_retire;
      if (retire_limit > maxValue)
      {
         maxValue = retire_limit;
         mostUsedUnit = -retire_rule_id;
      }

      // compute retirement limit for vectorized case
      retire_limit = (double)numVecInstructions / max_retire;
      if (retire_limit > maxVecValue)
      {
         maxVecValue = retire_limit;
         mostUsedVecUnit = -retire_rule_id;
      }
   }
   
   // Paranoia check. Note that 0.01 is not part of the ceil. It is used to make sure that
   // the floor value given by (int) conversion gives me the correct value if the value returned
   // by ceil has fp precision errors (not sure if possible in this case).
   lbDueToResources = (int)(ceil(maxValue)+0.01);
   bottleneckUnit = mostUsedUnit;
   vecUnit = mostUsedVecUnit;
   // account for the fact that the computed vectorization limit corresponds
   // to the loop being unrolled maxVecFactor times
   vecBound = maxVecValue / maxVecFactor;
   if (vecBound > lbDueToResources)
      vecBound = lbDueToResources;
   delete[] resourceUsage;
   delete[] vecResourceUsage;
   delete[] asyncUsage;
   delete[] vecAsyncUsage;
   return (lbDueToResources);
}
//--------------------------------------------------------------------------------------------------------------------

/* Check if an instruction is a memory reference to the stack
 * with all strides zero, and no indirect or irregular stride.
 */
bool 
SchedDG::Node::is_scalar_stack_reference()
{
   if (! is_memory_reference())
      return (false);
   int opidx = memoryOpIndex();
   if (opidx < 0) {
      fprintf(stderr, "Error: SchedDG::Node::is_scalar_stack_reference: Instruction at 0x%" PRIxaddr ", uop of type %s, has memory opidx %d\n",
            address, Convert_InstrBin_to_string((MIAMI::InstrBin)type), opidx);
      assert(!"Negative memopidx. Why??");
   }
   RefFormulas *refF = in_cfg()->refFormulas.hasFormulasFor(address, opidx);
   if (refF == NULL)
      return (false);
   
   GFSliceVal oform = refF->base;
   coeff_t soffset;
   if (oform.is_uninitialized() || !FormulaIsStackReference(oform, soffset))
      return (false);
   
   // is stack reference. Now check that all strides are zero and are not 
   // marked as indirect or irregular
   int numStrides = refF->NumberOfStrides();
   for (int i=0 ; i<numStrides ; ++i)
   {
      GFSliceVal& stride = refF->strides[i];
      if (stride.is_uninitialized() || stride.has_irregular_access() || stride.has_indirect_access())
         return (false);
      coeff_t valueNum;
      ucoeff_t valueDen;
      if (!IsConstantFormula(stride, valueNum, valueDen) || valueNum!=0)
         return (false);
   }
   return (true);
}

void 
SchedDG::markLoopBoilerplateNodes()
{
   // iterate over all the edges
   // mark as boilerplate all the sources of ADDRESS edges
   // and then propagate the marking backwards along TRUE register 
   // dependencies
   EdgesIterator eit(*this);
   while ((bool)eit)
   {
      if (!eit->IsRemoved() && eit->getDirection()==TRUE_DEPENDENCY &&
           eit->getType()==ADDR_REGISTER_TYPE)
      {
         Node *nn = eit->source();
         nn->propagateAddressCalculation();
      }
      ++ eit;
   }
   
   // we should have a lastBranch; assert??
   // anyway, if we have one, start from the branch and propagate
   // backwards along register dependencies
   if (lastBranch)
   {
      lastBranch->propagateLoopCondition();
   }
}

void
SchedDG::Node::propagateAddressCalculation()
{
   // check if we've processed this node before; termination check
   if (isAddressCalculation())
      return;
   
   // the node wasn't marked before
   setAddressCalculation();
   // mark as address calculation all the nodes that are reachable 
   // through register true dependencies.
   // I should consider also spill-unspill cases.
   // If instruction is a Load and we have a true memory dependency from a 
   // Save instruction, where neither is indirect/irregular, keep propagating.
   // Sometimes we actually want to save a computed value into an array to 
   // reuse later. Should I propagate through these?? Perhaps I should 
   // only propagate through scalar stack references ...
   IncomingEdgesIterator ieit(this);
   while ((bool)ieit)
   {
      if (!ieit->IsRemoved() && ieit->getDirection()==TRUE_DEPENDENCY &&
           (ieit->getType()==ADDR_REGISTER_TYPE || ieit->getType()==GP_REGISTER_TYPE ||
            (ieit->getType()==MEMORY_TYPE && is_load_instruction() && is_scalar_stack_reference()
              && ieit->source()->is_store_instruction() && ieit->source()->is_scalar_stack_reference())
           )
         )
         ieit->source()->propagateAddressCalculation();
         
      ++ ieit;
   }
}

void
SchedDG::Node::propagateLoopCondition()
{
   // check if we've processed this node before; termination check
   if (isLoopCondition())
      return;
   
   // the node wasn't marked before
   setLoopCondition();
   // mark as address calculation all the nodes that are reachable 
   // through register true dependencies.
   IncomingEdgesIterator ieit(this);
   while ((bool)ieit)
   {
      if (!ieit->IsRemoved() && ieit->getDirection()==TRUE_DEPENDENCY &&
           (ieit->getType()==GP_REGISTER_TYPE ||
            (ieit->getType()==MEMORY_TYPE && is_load_instruction() && is_scalar_stack_reference()
              && ieit->source()->is_store_instruction() && ieit->source()->is_scalar_stack_reference())
           )
         )
         ieit->source()->propagateLoopCondition();
         
      ++ ieit;
   }
}

void
SchedDG::Edge::PrintObject (ostream& os) const
{
   os << "Edge " << id << " of type " << dependencyTypeToString(dtype) 
      << ", direction " << dependencyDirectionToString(ddirection) 
      << ", distance " << dependencyDistanceToString(ddistance) 
      << ", removed? " << (IsRemoved()?"YES":"NO") << ", minLatency=" 
      << minLatency;
   if (realLatency != NO_LATENCY)
      os << ", actualLatency=" << getActualLatency();
   if (dtype==SUPER_EDGE_TYPE)
      os << ", num diff paths=" << numPaths;
   os << ", hasCycles=" << hasCycles << ", longestCycle " << longestCycle 
      << ", maxRemaining " << maxRemaining << ", remainingEdges "
      << remainingEdges << ", resourceScore " 
      << resourceScore << " with source {" << *(source()) 
      << "} and sink {" << *(sink()) << "}.";
   if (isScheduled())
      os << " Edge uses " << usedSlack << " cycles of slack.";
//   os << endl;
}
//--------------------------------------------------------------------------------------------------------------------

void
SchedDG::Node::PrintObject (ostream& os) const
{
   os << "Node " << id << " of type " << Convert_InstrBin_to_string((InstrBin)type)
      << ", longestCycle " << longestCycle << ", resourceScore " << resourceScore
      << ", sccId " << sccId;
   if (scheduled>0)
      os << " with scheduleId " << scheduled << " scheduled at " 
         << schedTime << " using template " << templIdx;
   os << ".";   //endl;
}
//--------------------------------------------------------------------------------------------------------------------

// make a graphical representation of the scheduling
// I need to sort the nodes by schedule time (first cycle, then iteration),
// and by level. Use a priority queue.
class SortNodesBySchedTime : public std::binary_function<SchedDG::Node*, 
           SchedDG::Node*, bool>
{
public:
   bool operator () (SchedDG::Node*& n1, SchedDG::Node*& n2) const
   {
      const ScheduleTime &s1 = n1->getScheduleTime ();
      const ScheduleTime &s2 = n2->getScheduleTime ();
      if (s1.ClockCycle() > s2.ClockCycle())
         return (true);
      if (s1.ClockCycle() < s2.ClockCycle())
         return (false);
      if (s1.Iteration() > s2.Iteration())
         return (true);
      if (s1.Iteration() < s2.Iteration())
         return (false);
      if (n1->getLevel() > n2->getLevel())
         return (true);
      return (false);
   }
};

typedef std::priority_queue <SchedDG::Node*, std::vector<SchedDG::Node*>,
            SortNodesBySchedTime > SchedNodePQ;

void
SchedDG::draw_cycle_header (std::ostream& os, int cycle)
{ 
   os << "subgraph { rank=same;\n";
   os << "Cyc" << cycle << " [label=\"Cycle " << cycle << "\"];\n";
#if 0
   os << "C" << cycle << " [label=< <TABLE BORDER=\"0\" CELLSPACING=\"0\" "
      << "CELLBORDER=\"0\">\n"
      << "<TR><TD PORT=\"label\">Cycle " << cycle << "</TD></TR>\n";
   os << "</TABLE>>];\n";
#endif
   
}

void
SchedDG::draw_cycle_footer (std::ostream& os, int cycle, NodeList &nl)
{ 
//   os << "</TABLE>>];\n";
   // write invisible edges to hold this cluster together
   if (!nl.empty ())
   {
      os << "Cyc" << cycle;
      NodeList::iterator nlit = nl.begin ();
      for ( ; nlit!=nl.end() ; ++nlit )
         os << " -> IS" << (*nlit)->scheduleId();
      os << ";\n";
   }
   os << "}\n";
}

void
SchedDG::Node::draw_scheduling (std::ostream& os)
{
//   os << "<TR><TD>";
   os << "IS" << scheduled << " [label=< \n";

   os << "<TABLE BORDER=\"1\" CELLSPACING=\"0\">\n";
   // PORT=\"s" << scheduled << "\">\n";

   // first row
   os << "<TR><TD>";
   if (CreatedByReplace())     // this is a node created by a replacement
      os << "* ";              // mark them with a star;
   if (NodeIsReplicated())     // this is a node replicated during a replacement
      os << "+ ";              // mark them with a cross;
   os << scheduled << "</TD><TD>"
      << schedTime.Iteration() << "</TD><TD>"
      << templIdx << "</TD></TR>\n";

   // node label
   os << "<TR><TD COLSPAN=\"3\">";
   write_label(os, 0, true);
   os << "</TD></TR>\n";

   // machine units
   os << "<TR><TD COLSPAN=\"3\">";
   _in_cfg->write_machine_units(os, allocatedUnits, 0);
   os << "</TD></TR>\n";
   
   os << "</TABLE>";

   os << ">";

   os << "];\n";
//   os <<  "</TD></TR>\n";
}

void
SchedDG::write_machine_units(std::ostream& os, PairList& selectedUnits, 
			     int offset)
{
   PairList::iterator plit = selectedUnits.begin();
   while (plit!=selectedUnits.end() && (int)plit->second<offset)
      ++plit;
   bool first = true;
   while (plit!=selectedUnits.end() && (int)plit->second==offset)
   {
      int unitIdx = 0;
      const char* uName = mach->getNameForUnit(plit->first, unitIdx);
      if (first)
      {
         os << uName << "[" << unitIdx << "]";
         first = false;
      }
      else
         os << "+" << uName << "[" << unitIdx << "]";
      ++ plit;
   }
}

void
SchedDG::draw_time_account (std::ostream& os, int minCycIndex, 
           int lastNodeMaxCyc, bool before)
{
   int num_ta_nodes = 2;
   int i;

   os << "subgraph cluster_TimeAccount {\n";
   os << "node[shape=point,style=\"invis\",margin=\"0.0\",width=\"0.0\",height=\"0.0\"];\n";
//   os << "ta0";
//   for (i=1 ; i<num_ta_nodes ; ++i)
//      os << "  ta" << i;
   os << "\n";
   os << "ta0";
   for (i=1 ; i<num_ta_nodes ; ++i)
      os << " -> ta" << i;
   if (num_ta_nodes>1)
      os << " [style=\"invis\",weight=99];\n";

   os << "label=< <TABLE BORDER=\"0\" CELLSPACING=\"0\" "
      << "CELLBORDER=\"0\">\n"
      << "<TR><TD ALIGN=\"CENTER\" COLSPAN=\"2\"><FONT POINT-SIZE=\"18\">"
      << "Cost per iteration:</FONT></TD></TR>\n";
      
   const UiToDoubleMap &timeStatsMap = timeStats.getDataConst();
   UiToDoubleMap::const_iterator tait = timeStatsMap.begin ();
   for ( ; tait!=timeStatsMap.end(); ++tait)
   {
      os << "<TR><TD ALIGN=\"LEFT\">" 
         << getTANameForKey (tait->first, mach) << ":</TD>\n"
         << "<TD ALIGN=\"CENTER\">" 
         << timeStats.getDisplayValue (tait->first, mach) << "</TD></TR>\n";
   }
   os << "</TABLE> >;\n";
   os << "}\n";
   if (minCycIndex>=0)
      os << "rank=same { Cyc" << minCycIndex << "  ta0 }\n";
   if (before)
      os << "IS" << lastNodeMaxCyc 
         << " -> ta0 [style=\"invis\",weight=\"1.0\"];\n";
   else
      os << "ta" << (num_ta_nodes-1) << " -> IS" << lastNodeMaxCyc
         << " [style=\"invis\",weight=\"1.0\"];\n";
}

void 
SchedDG::draw_scheduling (std::ostream& os, const char* name)
{
   int i;
   // Make sure the graph is not modified
   if (HasAllFlags (DG_GRAPH_IS_MODIFIED))
   {
      os << "SchedDG::draw_scheduling: graph is modified and level information" << endl
         << "is not current. Call this routine only on graphs fully or partially" << endl
         << "scheduled." << endl;
      return;
   }
   if (schedule_length < 1)
   {
      os << "SchedDG::draw_scheduling: length of scheduling information is" << endl
         << "invalid:" << schedule_length << "." << endl;
      return;
   }
   
   os << "digraph \"" << name << "\"{\n";
   os << "size=\"7,10\";\n";
   os << "rankdir=\"LR\";\n";
   os << "center=\"true\";\n";
//   os << "ratio=\"fill\";\n";
   os << "ranksep=0.5;\n";
   os << "nodesep=0.3;\n";
   os << "node[color=black,fontcolor=black,shape=none,font=Helvetica,fontsize=14,height=.3,rank=same,penwidth=3];\n";
   os << "edge[style=\"invis\",weight=499,penwidth=3];\n";

   SchedNodePQ snpq;
   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      if (nn->isInstructionNode ())
      {
         if (nn->isScheduled ())
            snpq.push (nn);
         else
            nn->draw(os);
      }
      ++ nit;
   }

   int* numNodes = new int[schedule_length];
   int* lastNode = new int[schedule_length];
   
   // I need to read instructions on turn, and display into a single node 
   // all instructions scheduled on the same cycle.
   int crtCyc = 0;
   int numNodesPerCyc = 0;
   
   NodeList nodesThisClock;
   draw_cycle_header (os, crtCyc);
   while (!snpq.empty())
   {
      Node* nextNode = snpq.top();
      const ScheduleTime &st = nextNode->getScheduleTime ();
      if (st.ClockCycle() == crtCyc)  // still on the same cycle
      {
         snpq.pop ();
         nextNode->draw_scheduling (os);
         nodesThisClock.push_back (nextNode);
         lastNode[crtCyc] = nextNode->scheduled;
         ++ numNodesPerCyc;
      } else    // change of cycle
      {
         draw_cycle_footer (os, crtCyc, nodesThisClock);
         numNodes[crtCyc] = numNodesPerCyc;
         ++ crtCyc;
         nodesThisClock.clear ();
         numNodesPerCyc = 0;
         draw_cycle_header (os, crtCyc);
      }
   }
   draw_cycle_footer (os, crtCyc, nodesThisClock);
   nodesThisClock.clear ();
   numNodes[crtCyc] = numNodesPerCyc;
   assert (crtCyc < schedule_length);
   for (i=crtCyc+1 ; i<schedule_length ; ++i)
   {
      draw_cycle_header (os, i);
      draw_cycle_footer (os, i, nodesThisClock);
      numNodes[i] = 0;
   }

   // I have added code for all nodes. Now connect nodes in correct order with
   // heavy and invisible edges.
   if (schedule_length > 1)
   {
      os << "edge[style=\"invis\",weight=999];\n";
      for (i=0 ; i<schedule_length-1 ; ++i)
         os << "Cyc" << i << " -> ";
      os << "Cyc" << (schedule_length-1) << ";\n";
   }
   
   // Prepare to draw dependency edges
   os << "edge[font=Helvetica,fontsize=14,dir=forward,weight=\"1.0\"];\n";
   EdgesIterator egit(*this);
   while ((bool)egit)
   {
      Edge *ee = egit;
      // I should include control edges as well
      if (! ee->IsRemoved() && ee->isSchedulingEdge())
//               ee->getType()!=CONTROL_TYPE)
      {
         ee->draw_scheduling (os, draw_all, 0);
      }
      ++ egit;
   }
   
   int minCycIndex = -1;
   int lastNodeMaxCyc = -1;
   int minNumNodes = 0;
   bool before = true;
   for (i=0 ; i<schedule_length-1 ; ++i)
   {
      int temp_max = max (numNodes[i], numNodes[i+1]);
      if (minCycIndex<0 || temp_max<minNumNodes)
      {
         minCycIndex = i;
         minNumNodes = temp_max;
      }
   }
   // if schedule_length == 1 (minCycIndex == -1), then draw the stats to the right
   if (minCycIndex < 0)
   {
      assert (schedule_length>0);
      assert (numNodes[0]>0);
   }
   // find the cycle with the largest number of nodes
   int maxNumNodes = -1;
   for (i=0 ; i<schedule_length ; ++i)
      if (numNodes[i]>maxNumNodes || maxNumNodes<0)
      {
         maxNumNodes = numNodes[i];
         if (maxNumNodes>0)
            lastNodeMaxCyc = lastNode[i];
         if (minCycIndex>=0 && i>minCycIndex)
            before = false;
      }
   assert(lastNodeMaxCyc>0);
   
   draw_time_account (os, minCycIndex, lastNodeMaxCyc, before);
   delete[] numNodes;
   delete[] lastNode;

   os << "}\n";
}
//--------------------------------------------------------------------------------------------------------------------

void
SchedDG::draw_cluster (ostream& os, int idx, int indent)
{
   int i;
   if (idx)
   {
      os << setw(indent) << "" << "subgraph cluster" << idx << " {\n";
      if (listOfNodes[idx])
      {
         Edge *supE = NULL;
         NodeList::iterator lit = listOfNodes[idx]->begin();
         for ( ; lit!=listOfNodes[idx]->end() ; ++lit)
         {
            os << setw(indent) << "";
            (*lit)->draw (os);
            if (!supE)
               supE = (*lit)->superE;
         }
         
         os << setw(indent) << "" << "Lbl" << idx 
            << " [fontcolor=purple,shape=none,label=\"Struct " << idx;
         if (supE)
            os << "\\nSE" << supE->id << ",L=" << supE->minLatency
               << ",D=" << supE->ddistance;
         os << "\"];\n";
      }
   }
   for (i=childIdx[idx] ; i ; i=next_sibling[i])
      draw_cluster (os, i, indent+3);

   if (idx)
      os << setw(indent) << "" << "}\n";
}

void
SchedDG::draw (ostream& os, const char* name, DependencyFilterType f, 
        int maxCycle)
{
   // I have to compute levels for nodes
   if (HasAllFlags (DG_GRAPH_IS_MODIFIED))
      ComputeNodeLevels();

   os << "digraph \"" << name << "\"{\n";
   os << "size=\"7.5,10\";\n";
   os << "center=\"true\";\n";
   os << "ratio=\"compress\";\n";
   os << "ranksep=.3;\n";
   os << "node[color=black,fontcolor=black,shape=ellipse,font=Helvetica,fontsize=14,height=.3,penwidth=3];\n";
   os << "edge[font=Helvetica,fontsize=14,dir=forward,penwidth=3];\n";

//   os << "N0[fontsize=20,width=.5,height=.25,shape=plaintext,"
//      << "label=\"" << name << "\"];\n";

   // specify the clusters first (if any exists)
   if (childIdx && next_sibling && listOfNodes)
      draw_cluster (os, 0, 0);
   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      if (nn->isInstructionNode() && (! nn->structureId))
      {
         nn->draw (os);
      }
#if 0    // IB_inner_loop is considered an instruction node now
       else 
         if (nn->type==IB_inner_loop)  //DG_LOOP_ENTRY_TYPE)
         {
            os << "B" << nn->id << "[label=\"Inner loop\",shape=diamond];\n";
         }
#endif
      ++ nit;
   }
   
   EdgesIterator egit(*this);
   while ((bool)egit)
   {
      Edge *ee = egit;
      if (!ee->IsRemoved() && ee->isSchedulingEdge())
//                 ee->isSuperEdge()) )
      {
         ee->draw(os, f, maxCycle);
      }
      ++ egit;
   }
   
   os << "}\n";
}

typedef std::set <SchedDG::Node*> NodeSet;
void
SchedDG::draw_min (ostream& os, const char* name, DependencyFilterType f)
{
   // I have to compute levels for nodes
   if (HasAllFlags (DG_GRAPH_IS_MODIFIED))
      ComputeNodeLevels();

   os << "digraph \"" << name << "\"{\n";
   os << "size=\"7.5,10\";\n";
   os << "center=\"true\";\n";
   os << "ratio=\"compress\";\n";
   os << "ranksep=.3;\n";
   os << "node[color=black,fontcolor=black,shape=ellipse,font=Helvetica,fontsize=14,height=.3];\n";
   os << "edge[font=Helvetica,fontsize=14,dir=forward];\n";

//   os << "N0[fontsize=20,width=.5,height=.25,shape=plaintext,"
//      << "label=\"" << name << "\"];\n";

   // clusters will be replaced by super-edges
   NodeSet myNodes;

   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      if (nn->isInstructionNode() && (! nn->structureId))
      {
         myNodes.insert (nn);
      }
      ++ nit;
   }
   
   EdgesIterator egit(*this);
   while ((bool)egit)
   {
      Edge *ee = egit;
      if (!ee->IsRemoved() && (ee->isSchedulingEdge() ||
                 ee->isSuperEdge()) && !ee->superE)
      {
         ee->draw (os, f, 0);
         myNodes.insert (ee->source());
         myNodes.insert (ee->sink());
      }
      ++ egit;
   }
   NodeSet::iterator nsit = myNodes.begin();
   for ( ; nsit!=myNodes.end() ; ++nsit)
   {
      Node *nn = *nsit;
      if (nn->structureId)
         fprintf (stderr, "Error: found node %d part of structure %d that is connected by an edge with no superE\n",
             nn->id, nn->structureId);
      nn->draw (os);
   }
   
   myNodes.clear();
   os << "}\n";
}

void
SchedDG::Node::draw(ostream& os, int info)
{
   if (_flags & NODE_DONOT_DRAW)
      return;
   
   os << "B" << id << "[level=" << _level << ",";
   if (isLoopCondition())
      os << "color=\"blue\",";  // draw them in blue
   else if (isAddressCalculation())
      os << "color=\"green\",";  // draw them in green
   else if (CreatedByReplace())     // this is a node created by a replacement
      os << "color=\"red\",";  // draw them all red for now;
      
   if (NodeIsReplicated())     // this is a node replicated during a replacement
      os << "style=\"filled\",fillcolor=\"gold\",";  
   os << "label=\"";
   write_label(os, info);
   if (NodeOfFailure())
      os << "\",shape=doubleoctagon];\n";
   else
      if (type==IB_inner_loop)
         os << "\",shape=diamond];\n";
      else
         os << "\",shape=ellipse];\n";
}

void
SchedDG::Node::write_label(ostream& os, int info, bool html_escape)
{
//   os << hex << address << dec << ":"
   os << id;
   if (info)
      os << "(" << info << ")";
   os << "::" << Convert_InstrBin_to_string((InstrBin)type)
      << "{" << bit_width << "}" << ":"
      << ExecUnitTypeToString(exec_unit_type);
   if (exec_unit==ExecUnit_VECTOR)
      os << ",vec{" << vec_width << "} ";
   else
      os << " ";
   RInfoList::iterator rit = srcRegs.begin();
   for( ; rit!=srcRegs.end() ; ++rit )
   {
      os << regTypeToString(rit->type) << rit->name 
         << "[" << rit->lsb << ":" << rit->msb << "] ";
   }
   for (int i=0 ; i<num_imm_values ; ++i)
   {
      if (immVals[i].is_signed)
         os << immVals[i].value.s << "/" << hex << immVals[i].value.s << dec << " ";
      else
         os << immVals[i].value.u << "/" << hex << immVals[i].value.u << dec << " ";
   }
   if (html_escape)
      os << "-&gt;";
   else
      os << "->";
   rit = destRegs.begin();
   for( ; rit!=destRegs.end() ; ++rit )
   {
      os << regTypeToString(rit->type) << rit->name 
         << "[" << rit->lsb << ":" << rit->msb << "] ";
   }
}

void
SchedDG::Edge::draw(ostream& os, DependencyFilterType f, int maxCycle)
{
   if (source()->getLevel() <= sink()->getLevel())
      os << "B" << source()->getId() << "->B" << sink()->getId() << "[";
   else
   {
      os << "B" << sink()->getId() << "->B" << source()->getId() 
         << "[dir=back,";
   }
   write_label (os);
   write_style (os, f, maxCycle);
   os << "];\n";
}

void
SchedDG::Edge::draw_scheduling(ostream& os, DependencyFilterType f, 
          int maxCycle)
{
   // allocate the two name buffers static just so they are not allocated 
   // and deallocated on every call; Make sure to turn them back to 
   // automatic if this routine becomes recursive or in a multi-threaded 
   // environment.
   static char sourceName[64];
   static char sinkName[64];
   
   bool direct = true;
   int srcSchedId = source()->scheduleId();
   int sinkSchedId = sink()->scheduleId();
   int srcCycle = 0;
   int sinkCycle = 0;
   int score = 0;

   if (srcSchedId>0)
   {
      score += 2;
      srcCycle = source()->getScheduleTime().ClockCycle();
      sprintf (sourceName, "IS%u", srcSchedId);
   } else
      sprintf (sourceName, "B%d", source()->getId());

   if (sinkSchedId>0)
   {
      score += 1;
      sinkCycle = sink()->getScheduleTime().ClockCycle();
      sprintf (sinkName, "IS%u", sinkSchedId);
   } else
      sprintf (sinkName, "B%d", sink()->getId());
   
   // I need to break the cycles; I need a partial ordering for nodes.
   // Not to break the plot of scheduled nodes, I will always consider that
   // a scheduled node is before a not-scheduled one. In addition, I have a
   // partial ordering between nodes of the same category.
   switch (score)
   {
      case 0:   // neither scheduled
         if (source()->getLevel() > sink()->getLevel())
            direct = false;
         break;
         
      case 1:   // source not scheduled, sink scheduled
         direct = false;
         strcat (sinkName, ":e");
         break;
         
      case 2:   // source scheduled, sink not scheduled
         strcat (sourceName, ":e");
         break;
         
      case 3:   // both scheduled
         if (srcCycle==sinkCycle)  // if part of the same node
         {
            // place the edge on the right (east)
//            strcat (sourceName, ":e");
//            strcat (sinkName, ":e");
         } else
            if (srcCycle<sinkCycle)
            {
//               strcat (sourceName, ":e");
//               strcat (sinkName, ":w");
            } else
            {
               direct = false;
//               strcat (sourceName, ":w");
//               strcat (sinkName, ":e");
            }
         break;
   }
   
   if (direct)
      os << sourceName << "->" << sinkName << "[";
   else
   {
      os << sinkName << "->" << sourceName << "[dir=back,";
   }
   write_label(os);
   write_style(os, f, maxCycle);
   os << "];\n";
}

void
SchedDG::Edge::write_label (ostream& os)
{
   if (dtype!=CONTROL_TYPE) {
      os << "label=\"dep(" << ddirection << "," << ddistance << ")";
   }
   else { // print probability for control dependencies
      char tempbuf[16];
      sprintf(tempbuf, "%5.3f", _prob);
      os << "label=\"prob=" << tempbuf;
   }
   if (source()->isScheduled()){
      os << ",lat=" << realLatency;
//ozgurS
      os << ",realMemlat=" << realMemLatency;
      os << ",realCPUlat=" << realCPULatency;
//ozgurE
   }
   else{
      if (_flags & LATENCY_COMPUTED_EDGE){
         os << ",minlat=" << minLatency;
//ozgurS
         os << ",memlat=" << minMemLatency;
         os << ",CPUlat=" << minCPULatency;
//ozgurE
      }
   }
   os << "\\nedg" << id << "\",";
}

void
SchedDG::Edge::write_style (ostream& os, DependencyFilterType f, unsigned int maxCycle)
{
   os << "style=";
   if (f(this) && !IsRemoved())  // edge was not pruned
      os << dependencyTypeStyle(dtype);
   else
      os << "invis";
   os << ",color=\"" << dependencyDirectionColor(ddirection, ddistance);
   if (EdgeOfFailure())
      os << ":magenta";
   UISet::iterator sit;
   for( sit=cyclesIds.begin() ; sit!=cyclesIds.end() && (*sit)<=maxCycle ; 
              ++sit )
   {
      os << ":" << cycleColor[(*sit)-1];
   }
   os << "\",fontcolor=" << dependencyDirectionColor(ddirection, ddistance);
   if (ddirection==CHANGING_DEPENDENCY)
      os << ",dir=both";
}

void 
SchedDG::draw_debug_graphics(const char* prefix, bool draw_reg_only, 
        int max_cycles)
{
   int i;
   char file_name[64];
   char file_name_ps[64];
   char title_draw[64];
   
   if (draw_reg_only)
   {
      sprintf(file_name, "%s_regonly.dot", prefix);
      sprintf(file_name_ps, "%s_regonly.ps", prefix);
      sprintf(title_draw, "%s_regonly", prefix);

      ofstream fout(file_name, ios::out);
      assert(fout && fout.good());
      draw(fout, title_draw, draw_regonly, 0);
      fout.close();
      assert(fout.good());

/*
      stringstream dot_cmd;
      dot_cmd << "dot -Tps2 -o " << file_name_ps << " "
              << file_name << '\0';
      system(dot_cmd.str().c_str());
 */
   }
   for (i=0 ; i<=numberOfCycles() && i<=max_cycles ; ++i)
   {
      sprintf(file_name, "%s_deps_%d.dot", prefix, i);
      sprintf(file_name_ps, "%s_deps_%d.ps", prefix, i);
      sprintf(title_draw, "%s_deps_%d", prefix, i);

      ofstream fout(file_name, ios::out);
      assert(fout && fout.good());
      draw(fout, title_draw, draw_all, i);
      fout.close();
      assert(fout.good());

/*
      stringstream dot_cmd;
      dot_cmd << "dot -Tps2 -o " << file_name_ps << " "
              << file_name << '\0';
      system(dot_cmd.str().c_str());
 */
   }
#if 0
   sprintf(file_name, "%s_all_deps.dot", prefix);
   sprintf(file_name_ps, "%s_all_deps.ps", prefix);
   sprintf(title_draw, "%s_all_deps", prefix);

   ofstream fout3 (file_name, ios::out);
   assert(fout3 && fout3.good());
   draw(fout3, title_draw, draw_all, 0);
   fout3.close();
   assert(fout3.good());

   stringstream dot_cmd3;
   dot_cmd3 << "dot -Tps2 -o " << file_name_ps << " "
            << file_name << '\0';
   system(dot_cmd3.str().c_str());
//   delete dot_cmd3.str();
#endif
}

void 
SchedDG::draw_debug_min_graphics (const char* prefix)
{
   char file_name[64];
   char file_name_ps[64];
   char title_draw[64];
   
   sprintf (file_name, "%s_min.dot", prefix);
   sprintf (file_name_ps, "%s_min.ps", prefix);
   sprintf (title_draw, "%s_min", prefix);

   ofstream fout (file_name, ios::out);
   assert (fout && fout.good());
   draw_min (fout, title_draw, draw_all);
   fout.close ();
   assert (fout.good());

/*
   stringstream dot_cmd;
   dot_cmd << "dot -Tps2 -o " << file_name_ps << " "
           << file_name << '\0';
   system(dot_cmd.str().c_str());
//   delete dot_cmd.str();
*/
}

void
SchedDG::printTimeAccount(){
  printf("PALM: timestats\n");
  timeStats.printStats(mach);
  printf("PALM: unitusage\n");
  unitUsage.printStats(mach);

}
void
SchedDG::draw_legend (ostream& os)
{
  os << "digraph \"Scheduling Graph Legend\"{\n";
  os << "subgraph legend{\n";
  os << "node[width=.5,height=.25,shape=plaintext,style=\"setlinewidth(2)\",fontsize=14,font=Helvetica];\n";
  os << "N0[fontcolor=black,fontsize=20,label=\"Legend\"];\n";
  os << "N1[color=black,fontcolor=black,fontsize=16,label=\"Color Map for Dependency Direction\"];\n";
  os << "N2[color=black,fontcolor=black,shape=ellipse,label=\"Instruction Node\"];\n";
  os << "N3[color=black,fontcolor=black,fontsize=16,label=\"Style Map for Dependency Type\"];\n";
  os << "N4[color=black,fontcolor=black,shape=ellipse,label=\"Instruction Node\"];\n";
  os << "EN2a[height=.1,width=.1,style=invis];\n";
  os << "EN2b[height=.1,width=.1,style=invis];\n";
  os << "EN2c[height=.1,width=.1,style=invis];\n";
  os << "EN2d[height=.1,width=.1,style=invis];\n";
  os << "EN4a[height=.1,width=.1,style=invis];\n";
  os << "EN4b[height=.1,width=.1,style=invis];\n";
  os << "EN4c[height=.1,width=.1,style=invis];\n";

  os << "N0->N1->N2->EN2b->N3->N4[style=invis];\n";
  os << "edge[style=\"setlinewidth(2),solid\"];\n";
  os << "N2->EN2a[color=" << dependencyDirectionColor(TRUE_DEPENDENCY, 0) 
      << ",fontcolor=" << dependencyDirectionColor(TRUE_DEPENDENCY, 0) 
      << ",label=\"True Dependency\"];\n";
  os << "N2->EN2b[color=" << dependencyDirectionColor(OUTPUT_DEPENDENCY, 0) 
      << ",fontcolor=" << dependencyDirectionColor(OUTPUT_DEPENDENCY, 0) 
      << ",label=\"Output Dependency\"];\n";
  os << "N2->EN2c[color=" << dependencyDirectionColor(ANTI_DEPENDENCY, 0) 
      << ",fontcolor=" << dependencyDirectionColor(ANTI_DEPENDENCY, 0) 
      << ",label=\"Anti Dependency\"];\n";
  os << "N2->EN2d[color=" << dependencyDirectionColor(CHANGING_DEPENDENCY, 0) 
      << ",fontcolor=" << dependencyDirectionColor(CHANGING_DEPENDENCY, 0) 
      << ",label=\"Changing Dependency\",dir=both];\n";

  os << "edge[color=blue,fontcolor=blue];\n";
  os << "N4->EN4a[style=" << dependencyTypeStyle(GP_REGISTER_TYPE) 
      << ",label=\"Register Dependency\"];\n";
  os << "N4->EN4b[style=" << dependencyTypeStyle(MEMORY_TYPE) 
      << ",label=\"Memory Dependency\"];\n";
  os << "N4->EN4c[style=" << dependencyTypeStyle(CONTROL_TYPE) 
      << ",label=\"Control Dependency\"];\n";
  os << "};\n";
  os << "}\n";
}

//--------------------------------------------------------------------------------------------------------------------
void
SchedDG::Node::longdump (SchedDG *currcfg, ostream& os)
{
  // print the node ID
  os << "NodeId: " << getId() << ", pc=" << hex << address << dec 
     << ", type=" << type << endl;
     
  if (this!=currcfg->CfgTop())
     os << hex << address << dec << ": " 
        << Convert_InstrBin_to_string((InstrBin)type) << " ";
  
   RInfoList::iterator rit = srcRegs.begin();
   for( ; rit!=srcRegs.end() ; ++rit )
   {
      os << regTypeToString(rit->type) << rit->name 
         << "[" << rit->lsb << ":" << rit->msb << "] ";
   }
   os << "--> ";
   rit = destRegs.begin();
   for( ; rit!=destRegs.end() ; ++rit )
   {
      os << regTypeToString(rit->type) << rit->name 
         << "[" << rit->lsb << ":" << rit->msb << "] ";
   }
   os << endl;
   
  if (num_incoming() == 0)
    os << " <-- [TopNode]";
  // print the source(s)
  SourceNodesIterator srcIt(this);
  if ((bool)srcIt) {
    os << " <-- (" << ((DGraph::Node*)srcIt)->getId();
    ++srcIt;
    while ((bool)srcIt) {
      os << ", " << ((DGraph::Node*)srcIt)->getId();
      ++srcIt;
    }
    os << ")";
  };
  os << endl;

  // print the sink(s)
  OutgoingEdgesIterator out_iter(this);
  if ((bool)out_iter) {
    DGraph::Edge* e = (DGraph::Edge*)out_iter;
    os << " --> (" << (e->sink())->getId();
    os << " [";
    e->dump(os);
    os << "]";
    ++out_iter;
    while ((bool)out_iter) {
      e = (DGraph::Edge*)out_iter;
      os << ", " << (e->sink())->getId();
      os << " [";
      e->dump(os);
      os << "]";
      ++out_iter;
    }
    os << ")" << endl;
  }
}
//--------------------------------------------------------------------------------------------------------------------


bool 
SchedDG::SortNodesByDegree::operator () (Node*& n1, Node*& n2) const
{
   return (n1->Degree() < n2->Degree());
}
//--------------------------------------------------------------------------------------------------------------------


bool 
SchedDG::SortNodesByDependencies::operator () (Node*& n1, Node*& n2) const
{
   if (limpMode)
   {
      if (n1->pathToLeaf != n2->pathToLeaf)
         return (n1->pathToLeaf < n2->pathToLeaf);
      else
         return (n1->resourceScore < n2->resourceScore);
   } else  //if (1)  //!resourceLimited)
   {
#if VERBOSE_DEBUG_SCHEDULER
           DEBUG_SCHED (7, 
              fprintf (stderr, "In Sort nodes by dependencies: n1 cycle len=%d, n2 cycle len=%d\n",
                   n1->longestCycle, n2->longestCycle);
           )
#endif
       if (! heavyOnResources)
       {
           if (n1->longestCycle < n2->longestCycle)
              return (true);
           if (n1->longestCycle > n2->longestCycle)
              return (false);
           if (n1->sumAllCycles < n2->sumAllCycles)
              return (true);
           if (n1->sumAllCycles > n2->sumAllCycles)
              return (false);
       }
       
#if VERBOSE_DEBUG_SCHEDULER
           DEBUG_SCHED (8, 
              fprintf (stderr, "In Sort nodes by dependencies: n1 max remain latency=%d , n2 max remain=%d\n",
                 n1->maxRemaining, n2->maxRemaining);
           )
#endif
           // if we are here, either both nodes are part of equal cycles,
           // or both are part of no cycle
           // if these nodes are not part of any cycle, then
           // maxRemaining will keep remaining path left (in fact
           // it will hold the length of path to root or 
           // the length of path to leaf, depending which is larger
           if (n1->maxRemaining < n2->maxRemaining)
              return (true);
           if (n1->maxRemaining > n2->maxRemaining)
              return (false);
           if (n1->remainingEdges < n2->remainingEdges)
              return (true);
           if (n1->remainingEdges > n2->remainingEdges)
              return (false);
           // if everything is equal, use the resource score
           return (n1->resourceScore < n2->resourceScore);
   }
}
//--------------------------------------------------------------------------------------------------------------------

bool 
SchedDG::SortEdgesByDependencies::operator () (Edge*& e1, Edge*& e2) const
{
   if (1)
   {
           register int hasCyc1 = e1->hasCycles;
           register int hasCyc2 = e2->hasCycles;
#if VERBOSE_DEBUG_SCHEDULER
           DEBUG_SCHED (7, 
              fprintf (stderr, "In Sort edges by dependencies: e1 edge %d remain latency (edges,hasCyc)=%d (%d,%d), e2 edge %d remain=%d (%d,%d)\n",
                 e1->id, e1->maxRemaining, e1->remainingEdges, hasCyc1, 
                 e2->id, e2->maxRemaining, e2->remainingEdges, hasCyc2);
           )
#endif
      if (!heavyOnResources)
      {
           if (hasCyc1 < hasCyc2)
              return (true);
           if (hasCyc1 > hasCyc2)
              return (false);
      } else
      {
           // if heavy on resources, favor the loop independent edges
           if (e1->dlevel > e2->dlevel)
              return (true);
           if (e1->dlevel < e2->dlevel)
              return (false);
      }
      
      // gmarin, 09/05/2013:
      // If heavy on resources, and the edges are not part of cycles, then consider the
      // resource score of the unscheduled end
      // This could lead to DEADLOCK with the right diamond-like structure!!
      // Better comment out, and change machine file templates for Move, Shuffle, Blend
      // so that they try first the pipelines that do not have an Add or Multiply unit.
      if (0 && resourceLimitedScore>1.5f && !hasCyc1 && !hasCyc2)
      {
         float res1 = (e1->source()->isScheduled() ? e1->sink()->resourceScore :
                         e1->source()->resourceScore) * e1->maxRemaining;
         float res2 = (e2->source()->isScheduled() ? e2->sink()->resourceScore :
                         e2->source()->resourceScore) * e2->maxRemaining;
         if (res1 != res2)
            return (res1 < res2);
      }
           // if we are here, either both edges are part of cycles,
           // or both are part of no cycle
           
           // if these edges are not part of any cycle, then
           // maxRemaining will keep remaining path left (in fact
           // it will hold the length of path to root or 
           // the length of path to leaf, depending what end is
           // already scheduled).
           if (e1->maxRemaining < e2->maxRemaining)
              return (true);
           if (e1->maxRemaining > e2->maxRemaining)
              return (false);
           if (e1->remainingEdges < e2->remainingEdges)
              return (true);
           if (e1->remainingEdges > e2->remainingEdges)
              return (false);

           // it means they have equal remaining latencies
           // if we talk about cycles, then I want to give higher
           // priority to edges that are part of a longer cycle
           if (!heavyOnResources && hasCyc1==3)
           {
              if (e1->longestCycle < e2->longestCycle)
                 return (true);
              if (e1->longestCycle > e2->longestCycle)
                 return (false);
           }
           
           // Next criteria is EXPERIMENTAL
           // I would like to give higher priority to loop independent edges.
           // Only cycles can have loop carried dependencies because I
           // convert all the other edges to loop independent.
           // 03/23/2007 mgabi: I found a case where this produced bad results
        if (! heavyOnResources)
        {
           if (e1->dlevel > e2->dlevel)
              return (true);
           if (e1->dlevel < e2->dlevel)
              return (false);
        }

           // all things being equal, for all cases except real cycle,
           // I want to give higher priority to edges where the scheduled
           // node is in a position that lengthens the overall span more
           if (heavyOnResources || hasCyc1<3)
           {
              register int sink1Sched = e1->sink()->isScheduled();
              register int sink2Sched = e2->sink()->isScheduled();
              if (sink1Sched < sink2Sched)
                 return (true);
              if (sink1Sched > sink2Sched)
                 return (false);
              // both edges have the same end scheduled
              if (sink1Sched)
              {
                 if (e1->sink()->schedTime > e2->sink()->schedTime)
                    return (true);
                 if (e1->sink()->schedTime < e2->sink()->schedTime)
                    return (false);
              } else
              {
                 if (e1->source()->schedTime < e2->source()->schedTime)
                    return (true);
                 if (e1->source()->schedTime > e2->source()->schedTime)
                    return (false);
              }
           }

           if (hiResourceScore)
           {
              if (e1->resourceScore < e2->resourceScore)
                 return (true);
              if (e1->resourceScore > e2->resourceScore)
                 return (false);
           }
           if (SWP_enabled && lowResourceScore)
           {
              if (e1->extraPriority < e2->extraPriority)
                 return (true);
              if (e1->extraPriority > e2->extraPriority)
                 return (false);
           }
           if (e1->sumAllCycles < e2->sumAllCycles)
              return (true);
           if (e1->sumAllCycles > e2->sumAllCycles)
              return (false);

#if 0
           if (!hasCyc1 && !hasCyc2)
           {
              // if these edges are not part of any cycle, then
              // maxRemaining will keep remaining path left (in fact
              // it will hold the length of path to root or 
              // the length of path to leaf, depending what end is
              // already scheduled).
              
              if (e1->maxRemaining < e2->maxRemaining)
                 return (true);
              if (e1->maxRemaining > e2->maxRemaining)
                 return (false);
              // it means they have equal remaining latencies
              if (e1->remainingEdges < e2->remainingEdges)
                 return (true);
              if (e1->remainingEdges > e2->remainingEdges)
                 return (false);
           }
#endif
#if 0
           if (e1->longestPath < e2->longestPath)
              return (true);
           if (e1->longestPath > e2->longestPath)
              return (false);
#endif
           if (!SWP_enabled && lowResourceScore)
           {
              if (e1->extraPriority < e2->extraPriority)
                 return (true);
              if (e1->extraPriority > e2->extraPriority)
                 return (false);
           }
           if (!hiResourceScore)
           {
              if (e1->resourceScore < e2->resourceScore)
                 return (true);
              if (e1->resourceScore > e2->resourceScore)
                 return (false);
           }
           if (!lowResourceScore)
           {
              if (e1->extraPriority < e2->extraPriority)
                 return (true);
              if (e1->extraPriority > e2->extraPriority)
                 return (false);
           }
           // finally, check the number of neighbours
           if (e1->neighbors < e2->neighbors)
              return (true);
           return (false);
   } else   // path is resource limited
   {
           fprintf (stdout, "In Sort edges by resources: e1 res score=%g, e2 res score=%g\n",
               e1->resourceScore, e2->resourceScore);
           if (e1->resourceScore < e2->resourceScore)
              return (true);
           if (e1->resourceScore > e2->resourceScore)
              return (false);
           if (e1->maxRemaining < e2->maxRemaining)
              return (true);
           if (e1->maxRemaining > e2->maxRemaining)
              return (false);
#if 0
           if (e1->longestCycle < e2->longestCycle)
              return (true);
           if (e1->longestCycle > e2->longestCycle)
              return (false);
#endif
           if (SWP_enabled)
           {
              if (e1->extraPriority < e2->extraPriority)
                 return (true);
              if (e1->extraPriority > e2->extraPriority)
                 return (false);
           }
           if (e1->sumAllCycles < e2->sumAllCycles)
              return (true);
           if (e1->sumAllCycles > e2->sumAllCycles)
              return (false);
           if (e1->longestPath < e2->longestPath)
              return (true);
           if (e1->longestPath > e2->longestPath)
              return (false);
           if (!SWP_enabled)
           {
              if (e1->extraPriority < e2->extraPriority)
                 return (true);
              if (e1->extraPriority > e2->extraPriority)
                 return (false);
           }
           // finally, check the number of neighbours
           if (e1->neighbors < e2->neighbors)
              return (true);
           return (false);
   }
}

}  /* namespace MIAMI_DG */

