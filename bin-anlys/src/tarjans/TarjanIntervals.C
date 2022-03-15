/*! \file
  
  \brief This file contains code to determine the Tarjan intervals
         (nested strongly connected regions) of a flow graph.
         Tarjan's original algorithm is described in the following
         article:
           Tarjan, R.E. "Testing flow graph reducibility"
           J. of Computer & System Sciences 9, 355-365, 1974. 
  
         Extensions to Tarjan's algorithm were made here to handle
         reducible SCRs surrounded by or containing irreducible ones.
         The extensions are described in the following article:
           Havlak, P. "Nesting of reducible and irreducible loops"
           ACM TOPLAS, July, 1997.
  
         The result is returned as a tree of SCRs, where a parent SCR
         contains its children.  A leaf SCR is a single node, which is
         not really a loop unless the isCyclic flag is set.
  
         Preconditions:
         1. The underlying directed graph has a unique start node,
            dominating all others in the graph and a unique exit node,
            postdominating all others.
        
  \authors Original DSystem code by phh (Paul Havlak?)
           Port to 'Classic OpenAnalysis' by John Mellor-Crummey,
             Jason Eckhardt
           Port to OpenAnalysis by Nathan Tallent (renamed NestedSCR
	     from TarjanIntervals)
  \version $Id$

  Copyright (c) 2002-2005, Rice University <br>
  Copyright (c) 2004-2005, University of Chicago <br>
  Copyright (c) 2006, Contributors <br>
  All rights reserved. <br>
  See ../../../Copyright.txt for details. <br>
*/

//************************** System Include Files ***************************

#include <iostream>
using std::cout;
#include <iomanip>
#include <algorithm>     // for max
#include <map>
#include <list>

#ifdef NO_STD_CHEADERS
# include <string.h>
# include <stdio.h>
# include <limits.h>
#else
# include <cstring>
# include <cstdio>
# include <climits>
using namespace std; // For compatibility with non-std C headers
#endif

#include <unistd.h>
#include <assert.h>

//*************************** User Include Files ****************************

#include "TarjanIntervals.h"

//*************************** Forward Declarations **************************

const TarjanIntervals::DFNUM_t DFNUM_ROOT = 0;
const TarjanIntervals::DFNUM_t DFNUM_NIL  = UINT_MAX;

//*************************** Forward Declarations **************************

#define DEBUG_TARJANS 0

//------------------------------------------------------------------------
// TarjWork: There will be an array of TarjWorks, one for each node.
// These are indexed by the DFS number of the node.  This way, there
// is no dependence on the node numbering of the underlying graph.
//------------------------------------------------------------------------

class TarjWork {
public:
  TarjWork();
  ~TarjWork();

  RIFGNodeId wk_vertex;	// map from DFS# of vertex to RIFGNodeId
  TarjanIntervals::DFNUM_t wk_last;   // DFS # of vertex's last descendant
  TarjanIntervals::DFNUM_t wk_header; // header of the vertex's interval - HIGHPT
  TarjanIntervals::DFNUM_t wk_nextP;  // next member of P set == reachunder
  TarjanIntervals::DFNUM_t wk_nextQ;  // next member of Q set == worklist
  bool wk_inP;			// test for membership in P == reachunder
  bool wk_isCyclic;		// has backedges -- to id singles
  bool wk_reducible;		// true if cyclic scr is reducible
  std::list<int> backPreds;	// List of preds that are backedges.
  std::list<int> nonBackPreds;  // List of preds that are non-backedges.
};


TarjWork::TarjWork() 
{
  wk_vertex = RIFG_NIL;
  wk_last   = DFNUM_NIL;
  wk_header = DFNUM_ROOT;
  wk_nextP  = DFNUM_NIL;
  wk_nextQ  = DFNUM_NIL;
  wk_inP = false;
  wk_isCyclic = false;
  wk_reducible = true;
}


TarjWork::~TarjWork() 
{
  backPreds.clear();
  nonBackPreds.clear();
}


//------------------------------------------------------------------------
// Interval tree will consist of an array of TarjTreeNodes. These
// are indexed (internally) by the DFS number of the node.  This way,
// there is no dependence on the node numbering of the underlying graph. 
//
// There is also a map from RIFGNodeIds to DFS numbers, so
// that tree nodes can be accessed (by the client) through their
// node ids. 
//------------------------------------------------------------------------

class TarjTreeNode {
public:
  TarjTreeNode();
  RIFGNodeId nodeid;	// Associated RIFGNodeId.
  short level;		// nesting depth -- outermost loop is 1 
  short  depth;		// nesting depth -- innermost loop is 1 
  TarjanIntervals::Node_t type; // acyclic, interval or irreducible 
  TarjanIntervals::DFNUM_t outer;	 // DFS number of header of containing interval
  TarjanIntervals::DFNUM_t inners; // DFS number of header of first nested interval
  TarjanIntervals::DFNUM_t next;	// DFS number of next nested header
  int prenum;			// preorder number
  TarjanIntervals::DFNUM_t last;	// number of last descendent
  RIFGNodeId last_id;		// id of last descendent
  short loopIndex;		// unique id for intervals
  short numExits;
  short numEntries;
};


TarjTreeNode::TarjTreeNode()
{
  nodeid = RIFG_NIL;
  level  = 0;
  depth  = 0;
  type   = TarjanIntervals::NODE_NOTHING;
  outer  = DFNUM_NIL;
  inners = DFNUM_NIL;
  next   = DFNUM_NIL;
  prenum = -1;
  last   = DFNUM_NIL;	
  last_id = RIFG_NIL;
  loopIndex = -1;
  numExits = 0;
  numEntries = 0;
}


//***************************************************************************
// TarjanIntervals
//***************************************************************************

  // Refers to members in TarjTreeNode
#define TARJ_nodeid(name)	(tarj[name].nodeid)
#define TARJ_outer(name)	(tarj[name].outer)
#define TARJ_inners(name)	(tarj[name].inners)
#define TARJ_next(name)		(tarj[name].next)
#define TARJ_level(name)	(tarj[name].level)
#define TARJ_depth(name)	(tarj[name].depth)
#define TARJ_type(name)		(tarj[name].type)
#define TARJ_last(name)		(tarj[name].last)
#define TARJ_last_id(name)	(tarj[name].last_id)
#define TARJ_loopIndex(name)	(tarj[name].loopIndex)
#define TARJ_numExits(name)	(tarj[name].numExits)
#define TARJ_numEntries(name)	(tarj[name].numEntries)
#define TARJ_contains(a,b)	\
    ( ( tarj[a].prenum <= tarj[b].prenum ) && \
      ( tarj[b].prenum <= tarj[tarj[a].last].prenum ) \
    )

  // Refers to members in TarjWork
#define vertex(x)	(wk[x].wk_vertex)
#define TLast(x)	(wk[x].wk_last)
#define header(x)	(wk[x].wk_header)
#define nextP(x)	(wk[x].wk_nextP)
#define nextQ(x)	(wk[x].wk_nextQ)
#define inP(x)		(wk[x].wk_inP)
#define isCyclic(x)	(wk[x].wk_isCyclic)
#define reducible(x)	(wk[x].wk_reducible)
#define backPreds(x)	(wk[x].backPreds)
#define nonBackPreds(x)	(wk[x].nonBackPreds)

static TarjanIntervals::DFNUM_t DFS_n;	// next DFS preorder number
static RIFGNodeId DFS_last_id;	// RIFGNodeId whose DFS preorder number is DFS_n

//
// is_backedge(a,b) returns true if a is a descendant of b in DFST
// (a and b are the DFS numbers of the corresponding vertices). 
//
// Note that this is in the local structures; corresponds somewhat
// to Contains(tarj,b,a) for the public structures.
//
//
// use this one after the tree has been build and Work freed.
// On second thought, use Contains(b,a) instead. This one might
// not capture all backedges.
//#define is_backedge(a,b) ((b <= a) && (a <= TARJ_last(b)))

// use this one while the tree is being built
#define is_wk_backedge(a,b) ((b <= a) && (a <= TLast(b)))

#define dfnum(v)		(nodeid_to_dfnum_map[v])


TarjanIntervals::TarjanIntervals(RIFG &rifg_)
  : rifg(&rifg_)
{
  Create();
}


TarjanIntervals::~TarjanIntervals()
{
  if (tarj)
    delete[] tarj;
  if (uf)
    delete uf;
  if (wk)
    delete[] wk;
  rev_top_list.clear();
  nodeid_to_dfnum_map.clear();
}


// Construct tree of Tarjan intervals for the graph
void 
TarjanIntervals::Create()
{
  RIFGNodeId root = rifg->GetRootNode();
  
  Init();
  DFS(root);
  FillPredLists();
  GetTarjans();
  Build();
  Sort();
  ComputeIntervalIndex();
  FreeWork();
}


//
// Sort the Tarjan Tree in topological ordering
//
// Feb. 2002 (jle) - Removed the call to GetTopologicalMap which computes
// a (breadth-first) topological ordering of the CFG. This is completely
// unnecessary (and wasteful) since by virtue of performing a DFS as part of
// interval building, we have a topological ordering available as a by-product.
// Further, the breadth-first version in GetTopologicalMap is clumsy (in this
// context) since it has to make call-backs to the tarjan code to determine
// if the current edge being examined is a back-edge (to avoid cycling).
//
void 
TarjanIntervals::Sort()
{
  RIFGNodeId gId;
  DFNUM_t parent;		// Tarj parent (loop header)

  // Now disconnect all "next" fields for all tarj nodes

  RIFGNodeIterator* ni = rifg->GetNodeIterator();
  for (gId = ni->Current(); gId != RIFG_NIL; gId = (*ni)++) 
  {
    DFNUM_t gdfn = dfnum(gId);
    // gdfn will be invalid if the node gId was unreachable.
    if (gdfn != DFNUM_NIL) {
      TARJ_inners(gdfn) = DFNUM_NIL;
      TARJ_next(gdfn) = DFNUM_NIL;
    }
  }
  delete ni;

  // The list is sorted in reverse topological order since each child
  // in the loop below is prepended to the list of children.
  std::list<RIFGNodeId>::iterator i;
  for (i = rev_top_list.begin(); i != rev_top_list.end(); i++) {
    gId = *i;
    if (gId != RIFG_NIL) {
      // Add new kid
      DFNUM_t gdfn = dfnum(gId);
      parent = TARJ_outer(gdfn);
      if (parent != DFNUM_NIL) {
        TARJ_next(gdfn) = TARJ_inners(parent);
        TARJ_inners(parent) = gdfn;
      }
    }
  }

  Renumber();
} // end of tarj_sort()


TarjTreeNode* 
TarjanIntervals::getTree()
{
  return tarj;
}



//
// Allocate and initialize structures for algorithm, and UNION-FIND
//
void 
TarjanIntervals::Init()
{
  unsigned int g_size = rifg->HighWaterMarkNodeId() + 1;

  DFS_n = DFNUM_ROOT;
  number_exits_computed = false;

  //
  // Local work space
  //
  wk = new TarjWork[g_size];
  uf = new UnionFindUniverse(g_size);
  tarj = new TarjTreeNode[g_size];
  InitArrays();
}


//
// Initialize only nodes in current instance g
//
void 
TarjanIntervals::InitArrays()
{
  RIFGNodeId gId;
  RIFGNodeIterator* ni = rifg->GetNodeIterator();
  for (gId = ni->Current(); gId != RIFG_NIL; gId = (*ni)++) 
  {
    dfnum(gId) = DFNUM_NIL;
  }
  delete ni;
}


//
// Do depth first search on control flow graph to 
// initialize vertex[], dfnum[], last[]
//
// gmarin, 08/08/2013: DFS numbering is obviously not unique.
// This is not an issue in graphs without irreducible intervals, because
// any eventual loop has only one entry node (by definition of not being 
// irreducible). However, in the presence of irreducible intervals, different
// traversals can create different loop structures. Sometimes, when you have 
// very complicated CFGs, with nested irreducible intervals, the scope trees 
// may look very different depending on the DFS traversal.
// Try to maintain some sanity, by creating a more deterministic traversal.
// Implement a SortedEdgeIterator, that attempts to sort the edges by any
// given criteria, and use this iterator during the DFS traversal. It will 
// ensure more consistency between different analysis steps.
void 
TarjanIntervals::DFS(RIFGNodeId v)
{
  if (!rifg->IsValid(v)) return;

  vertex(DFS_n) = v;
  dfnum(v)  = DFS_n++;
 
  RIFGEdgeId succ;
  RIFGEdgeIterator *ei = rifg->GetSortedEdgeIterator(v, ED_OUTGOING);
  for (succ = ei->Current(); succ != RIFG_NIL; succ = (*ei)++)
  {
     int son = rifg->GetEdgeSink (succ);
#if DEBUG_TARJANS
     fprintf(stderr, "-- Found outgoing edge E%d from B%d to B%d\n", succ, v, son);
#endif
     if (dfnum(son) == DFNUM_NIL) 
        DFS(son);
  }
  delete ei;

  //
  // Equivalent to # of descendants -- number of last descendant
  //
  TLast(dfnum(v)) = DFS_n - 1;
#if DEBUG_TARJANS
  fprintf(stderr, "== Vertex B%d has DFS num %d and last DFS desc=%d\n", v, dfnum(v), DFS_n-1);
#endif
  rev_top_list.push_back(v);
}


// Fill in the backPreds and nonBackPreds lists for each node.
void 
TarjanIntervals::FillPredLists()
{
  for (DFNUM_t i = DFNUM_ROOT; i < DFS_n; ++i) 
  {
    RIFGEdgeId eId;
    RIFGEdgeIterator *ei = rifg->GetEdgeIterator(vertex(i), ED_INCOMING);
    for (eId = ei->Current(); eId != RIFG_NIL; eId = (*ei)++)
    {
      DFNUM_t prednum = dfnum(rifg->GetEdgeSrc(eId));
      if (is_wk_backedge(prednum, i)) {
        backPreds(i).push_back(prednum); 
#if DEBUG_TARJANS
        fprintf(stderr, "++ Found back edge E%d from DFS %d (B%d) to DFS %d (B%d)\n",
                eId, prednum, vertex(prednum), i, vertex(i));
#endif
      } else {
        nonBackPreds(i).push_back(prednum); 
#if DEBUG_TARJANS
        fprintf(stderr, "+- Found non-back edge E%d from DFS %d (B%d) to DFS %d (B%d)\n",
                eId, prednum, vertex(prednum), i, vertex(i));
#endif
      }
    } // for preds
    delete ei;
  } // for nodes
}


//
// In bottom-up traversal of depth-first spanning tree, 
// determine nested strongly connected regions and flag irreducible regions.
//                                                    phh, 4 Mar 91
//
void 
TarjanIntervals::GetTarjans()
{
  DFNUM_t w;			// DFS number of current vertex
  DFNUM_t firstP, firstQ;	// set and worklist

  //
  // Following loop should skip root (prenumbered as 0)
  //
  for (w = DFS_n - 1; w != DFNUM_ROOT; w--) // loop c
    //
    // skip any nodes freed or not reachable
    //
    if (w != DFNUM_NIL && rifg->IsValid(vertex(w))) {
      firstP = firstQ = DFNUM_NIL;
      //
      // Add sources of cycle arcs to P -- and to worklist Q
      //
      std::list<int>::iterator prednum; 
      for (prednum = backPreds(w).begin(); prednum != backPreds(w).end();
           prednum++) { // loop d
        DFNUM_t u,v;			// vertex names
        v = *prednum;
        // ignore predecessors not reachable
        if (v != DFNUM_NIL)
        {
          if (v == w) {
            //
            // Don't add w to its own P and Q sets
            //
            isCyclic(w) = true;
#if DEBUG_TARJANS
            fprintf(stderr, "-> Mark w=%d as cyclic1. vertex(w)=%d\n", w, vertex(w));
#endif
          } else {
            //
            // Add FIND(v) to P and Q
            //
            u = FIND(v);
            if (!inP(u)) {
              nextP(u) = nextQ(u) = firstP;
              firstP   = firstQ   = u;
              inP(u)   = true;
            } // if (!inP(u))
          } // if (v == w)
        } // if 
      } // for preds

      //
      // P nonempty -> w is header of a loop
      //
      if (firstP != DFNUM_NIL) {
        isCyclic(w) = true;
#if DEBUG_TARJANS
        fprintf(stderr, "-> Mark w=%d as cyclic2. vertex(w)=%d\n", w, vertex(w));
#endif
      }

      while (firstQ != DFNUM_NIL) {
        DFNUM_t x, y, yy;		// DFS nums of vertices

        x = firstQ;
        firstQ = nextQ(x);	// remove x from worklist

        //
        // Now look at non-cycle arcs
        //
        std::list<int>::iterator prednum;
        for (prednum = nonBackPreds(x).begin();
             prednum != nonBackPreds(x).end();
             prednum++) { // loop d
          y = *prednum;

          //
          // ignore predecessors not reachable
          //
          if (y != DFNUM_NIL)
          {
            //
            // Add FIND(y) to P and Q
            //
            yy = FIND(y);

            if (is_wk_backedge(yy, w)) {
              if ((!inP(yy)) & (yy != w)) {
                nextP(yy) = firstP;
                nextQ(yy) = firstQ;
                firstP = firstQ = yy;
                inP(yy) = true;
              }
              //
              // Slight change to published alg'm...
              // moved setting of header (HIGHPT)
              // from here to after the union.
              //
            } else {
              //
              // Irreducible region!
              //
              reducible(w) = false;
#if DEBUG_TARJANS
              fprintf(stderr, ">> Set w=%d as irreducible1. vertex(w)=%d\n", w, vertex(w));
#endif
#if 1
              // FIXME:old The DSystem version of the code did not have the
              // following line.  However, this line IS in the 1997 article,
              // and I believe it is necessary (jle, 03-02-2002).
              nonBackPreds(w).push_back(yy);
#endif
            }
          }
        }
      }
      //
      // now P = P(w) as in Tarjan's paper
      //
      while (firstP != DFNUM_NIL) {
        //
        // First line is a change to published algorithm;
        // Want sources of cycle edges to have header w
        // and w itself not to have header w.
        //
        if ((header(firstP) == DFNUM_ROOT) & (firstP != w))
          header(firstP) = w;
        UNION(firstP, w, w);
        inP(firstP) = false;
        firstP = nextP(firstP);
      } // while
    } // if
}


//
// Traverse df spanning tree, building tree of Tarjan intervals
//
void 
TarjanIntervals::Build()
{
  DFNUM_t w;		// DFS number of current vertex
  DFNUM_t outer;	// DFS number header of surrounding loop

  TARJ_nodeid(DFNUM_ROOT) = rifg->GetRootNode();
  //
  // Let the root of the tree be the root of the instance...
  // Following loop can skip the root (prenumbered 0)
  //
  for (w = DFNUM_ROOT + 1; w < DFS_n; w++) {
    RIFGNodeId wnode = vertex(w);
    //
    // skip any nodes not in current instance g
    //
    if (wnode != RIFG_NIL) {
      outer = header(w);
      TARJ_outer(w) = outer;
      TARJ_nodeid(w) = wnode;
      //
      // tarj[w].inners = DFNUM_NIL;  % done in InitArrays
      //
      if (isCyclic(w)) {
        //
        // Level is deeper than outer if this is a loop
        //
        if (reducible(w)) {
          TARJ_type(w) = NODE_INTERVAL;
          TARJ_level(w) = TARJ_level(outer) + 1;
        } 
	else {
          TARJ_type(w) = NODE_IRREDUCIBLE;
          TARJ_level(w) = TARJ_level(outer) + 1;
        }
      } 
      else {
	TARJ_type(w) = NODE_ACYCLIC;
        TARJ_level(w) = TARJ_level(outer);
      }
      TARJ_next(w) = TARJ_inners(outer);
      TARJ_inners(outer) = w;
    }
  }
  ComputeDepth(DFNUM_ROOT);
  DFS_n = 0;
  Prenumber(DFNUM_ROOT);
}

void 
TarjanIntervals::ComputeDepth(DFNUM_t v)
{
    DFNUM_t inner;
    int max_depth = -1;
    
    // first, go into recurssion as deep as possible, and record the
    // maximum depth among all children
    for (inner = TARJ_inners(v); inner != DFNUM_NIL; 
         inner = TARJ_next(inner)) 
    {
       if (isCyclic(inner)) {
          ComputeDepth(inner);
          if (max_depth<TARJ_depth(inner)) 
             max_depth = TARJ_depth(inner);
       }
    }
    max_depth += 1;
    // on the way back, after iterating through all sub-loops, update the depth
    // for all nodes at this level
    
    TARJ_depth(v) = max_depth;
    for (inner = TARJ_inners(v); inner != DFNUM_NIL; inner = TARJ_next(inner)) 
    {
       if (!isCyclic(inner))
          TARJ_depth(inner) = max_depth;
    }
}


void 
TarjanIntervals::Prenumber(int v)
{
  DFNUM_t inner;

  tarj[v].prenum = ++DFS_n;
  DFS_last_id = TARJ_nodeid(v);
    
  for (inner = TARJ_inners(v); inner != DFNUM_NIL; 
       inner = TARJ_next(inner)) {
    Prenumber(inner);
  }

  /* tarj[v].last = DFS_n;  // 3/18/93 RvH: switch to RIFGNodeId DFS_last_id */
  tarj[v].last_id = DFS_last_id;
  tarj[v].last = dfnum(DFS_last_id);
}


void 
TarjanIntervals::Renumber()
{
  tarj = tarj;
  DFS_n = 0;
  Prenumber(DFNUM_ROOT);
}


//  Free all space used in computation of 
//  Tarjan interval tree (but not tree itself)
void 
TarjanIntervals::FreeWork()
{
  delete[] wk;
  wk = 0;

  delete uf;
  uf = 0;
}


static int loopIndex;
void 
TarjanIntervals::ComputeIntervalIndexSubTree(DFNUM_t node)
{
  DFNUM_t kid;
    
  TARJ_loopIndex(node) = ++loopIndex;

  for (kid = TARJ_inners(node); kid != DFNUM_NIL; kid = TARJ_next(kid))
  {
    if (isCyclic(kid))
       ComputeIntervalIndexSubTree(kid);
    else
       TARJ_loopIndex(kid) = TARJ_loopIndex(node);
  }
}


void 
TarjanIntervals::ComputeIntervalIndex()
{
  loopIndex = 0;
  ComputeIntervalIndexSubTree(DFNUM_ROOT);
}


void 
TarjanIntervals::Dump(std::ostream& os)
{
//  os << "RIFG for Tarjan intervals\n";
//  rifg->dump(os);
  os << "Tarjan interval tree: node-id (level,type) : IntervalIndex\n";
  os << std::setfill(' ');
  DumpSubTree(os, DFNUM_ROOT, 0 /* Indent */);
}


void 
TarjanIntervals::DumpSubTree(std::ostream& os, int node, int indent)
{
  static const char *NodeType[] = {"NOTHING", "Acyclic",
				   "Interval", "Irreducible"};
  if (indent < 72) {
    indent += 2;
  }
  
  os << std::setw(indent) << " "
     << TARJ_nodeid(node) 
     << " (" << TARJ_level(node) << ", " << NodeType[TARJ_type(node)] << ")"
     << " : " << TARJ_loopIndex(node) << endl;
  
  for (DFNUM_t kid = TARJ_inners(node); kid != DFNUM_NIL; kid = TARJ_next(kid)) {
    DumpSubTree(os, kid, indent);
  }
  
  if (indent >= 3) {
    indent -= 2;
  }
}


int 
TarjanIntervals::FIND(int v)
{
  return uf->Find(v);
}


void 
TarjanIntervals::UNION(int i, int j, int k)
{
  uf->Union(i,j,k);
}




//
// Return type of g edge from src to sink, one of
//   RI_TARJ_NORMAL
//   RI_TARJ_LOOP_ENTRY
//   RI_TARJ_IRRED_ENTRY
//   RI_TARJ_ITERATE (back edge)
//
TarjanIntervals::Edge_t 
TarjanIntervals::getEdgeType(RIFGNodeId src, RIFGNodeId sink)
{
  RIFGNodeId anc = LCA(src, sink);
  DFNUM_t dfn_sink = dfnum(sink);
  DFNUM_t dfn_anc = dfnum(anc);

  if (dfn_anc == dfn_sink) {
    return EDGE_ITERATE;
  } 
  else if (TARJ_outer(dfn_sink) != DFNUM_NIL
           && (TARJ_type(TARJ_outer(dfn_sink)) == NODE_IRREDUCIBLE)
           && (dfn_anc != TARJ_outer(dfn_sink))) {
    //
    // Entering irreducible region not through the "header"
    //
    return EDGE_IRRED_ENTRY;
  } 
  else {
    switch (TARJ_type(dfn_sink)) {
    case NODE_INTERVAL:
      return EDGE_LOOP_ENTRY;
    case NODE_IRREDUCIBLE:
      //
      // Entering irreducible region through the "header"
      //
      return EDGE_IRRED_ENTRY;
    case NODE_ACYCLIC:
    case NODE_NOTHING:
    default:
      return EDGE_NORMAL;
    }
  }
}


//
// LCA = least common ancestor
//
RIFGNodeId 
TarjanIntervals::LCA(RIFGNodeId a, RIFGNodeId b)
{
  if ((a == RIFG_NIL) || (b == RIFG_NIL))
    return RIFG_NIL;

  if (Contains(a,b))
    return a;

  DFNUM_t dfn_b = dfnum(b);
  while ((dfn_b != DFNUM_NIL) && !Contains(b,a)) {
    dfn_b = TARJ_outer(dfn_b);
    b = TARJ_nodeid(dfn_b);
  }

  return b;
}


//
// Return true if the CFG edge passed in is an backedge.
//
//   Precondition: Tarjan tree must be built already.
//
bool 
TarjanIntervals::IsBackEdge(RIFGEdgeId edgeId)
{
  RIFGNodeId src, dest;
  src = rifg->GetEdgeSrc(edgeId);
  dest = rifg->GetEdgeSink(edgeId);
  return (Contains(dest, src));
/*
  DFNUM_t dfn_src = dfnum(src);
  DFNUM_t dfn_dest = dfnum(dest);
  return (is_backedge(dfn_src, dfn_dest));
*/
} 


RIFGNodeId 
TarjanIntervals::TarjInners(RIFGNodeId id)
{
  DFNUM_t dfn_inners = TARJ_inners(dfnum(id));
  return (dfn_inners == DFNUM_NIL ? RIFG_NIL : TARJ_nodeid(dfn_inners));
}


RIFGNodeId 
TarjanIntervals::TarjInnersLast(RIFGNodeId id)
{
  int dfn_last_id = TARJ_last_id(dfnum(id));
  return TARJ_nodeid(dfn_last_id);
}


RIFGNodeId
TarjanIntervals::TarjOuter(RIFGNodeId id)
{
  DFNUM_t dfn_outer = TARJ_outer(dfnum(id));
  return (dfn_outer == DFNUM_NIL ? RIFG_NIL : TARJ_nodeid(dfn_outer));
}


RIFGNodeId 
TarjanIntervals::TarjNext(RIFGNodeId id)
{
  DFNUM_t dfn_next = TARJ_next(dfnum(id));
  return (dfn_next == DFNUM_NIL ? RIFG_NIL : TARJ_nodeid(dfn_next));
}


/* This method is different than IsHeader. This is a better way to check
 * for a loop because there are intervals (loops) that have only the
 * header node and no inners nodes.
 */
bool 
TarjanIntervals::NodeIsLoopHeader(RIFGNodeId id)
{
    DFNUM_t dfn_id = dfnum(id);
    return (TARJ_type(dfn_id)==NODE_INTERVAL || 
            TARJ_type(dfn_id)==NODE_IRREDUCIBLE);
}


bool 
TarjanIntervals::IsHeader(RIFGNodeId id)
{
  return (TarjInners(id) != RIFG_NIL);
}


bool 
TarjanIntervals::IsFirst(RIFGNodeId id)
{
  if (TarjOuter(id) == RIFG_NIL)
    return true;

  return (TarjInners(TarjOuter(id)) == id);
}


bool 
TarjanIntervals::IsLast(RIFGNodeId id)
{
  return (TarjNext(id) == RIFG_NIL);
}


int 
TarjanIntervals::GetLevel(RIFGNodeId id)
{
  return TARJ_level(dfnum(id));
}


TarjanIntervals::Node_t 
TarjanIntervals::getNodeType(RIFGNodeId id)
{
  DFNUM_t dfn = dfnum(id);
  if (dfn == DFNUM_NIL) {
    return TarjanIntervals::NODE_NOTHING;
  }
  else {
    return TARJ_type(dfn);
  }
}


bool 
TarjanIntervals::Contains(RIFGNodeId a, RIFGNodeId b)
{
  return (TARJ_contains(dfnum(a), dfnum(b)));
}


int 
TarjanIntervals::LoopIndex(RIFGNodeId id)
{
  return TARJ_loopIndex(dfnum(id));
}

void
TarjanIntervals::find_entry_exit_edges_for_interval(RIFGNodeId rNode)
{
    RIFGNodeId kid;
    RIFGEdgeId outEdge;
    int loopsEntered =0, loopsExited = 0;
    
    RIFGEdgeIterator* ei = rifg->GetEdgeIterator(rNode, ED_OUTGOING);
    for (outEdge = ei->Current(); outEdge != RIFG_NIL; outEdge = (*ei)++) 
    {
       RIFGNodeId sinkId = rifg->GetEdgeSink(outEdge);
       tarj_entries_exits (rNode, sinkId, loopsEntered, loopsExited);
       RIFGNodeId iter = rNode;
       while (loopsExited>0)
       {
          assert (iter != RIFG_NIL);
          if (NodeIsLoopHeader(iter))
          {
             TARJ_numExits(dfnum(iter)) ++;
             loopsExited --;
          }
          iter = TarjOuter(iter);  //tarj[iter].outer;
       }
       iter = sinkId;
       while (loopsEntered>0)
       {
          assert (iter != RIFG_NIL);
          if (NodeIsLoopHeader(iter))
          {
             TARJ_numEntries(dfnum(iter)) ++;
             loopsEntered --;
          }
          iter = TarjOuter(iter);
       }
    }
    delete ei;

    for (kid = TarjInners(rNode) ; (kid != RIFG_NIL); kid = TarjNext(kid))
    {
       if (NodeIsLoopHeader(kid))
       {
          find_entry_exit_edges_for_interval (kid);
       }
       else
       {
          ei = rifg->GetEdgeIterator (kid, ED_OUTGOING);
          for (outEdge = ei->Current(); outEdge != RIFG_NIL; outEdge = (*ei)++) 
          {
             RIFGNodeId sinkId = rifg->GetEdgeSink(outEdge);
             tarj_entries_exits (kid, sinkId, loopsEntered, loopsExited);
             RIFGNodeId iter = rNode;
             while (loopsExited>0)
             {
                assert (iter != RIFG_NIL);
                if (NodeIsLoopHeader(iter))
                {
                   TARJ_numExits(dfnum(iter)) ++;
                   loopsExited --;
                }
                iter = TarjOuter(iter);
             }
             iter = sinkId;
             while (loopsEntered>0)
             {
                assert (iter != RIFG_NIL);
                if (NodeIsLoopHeader(iter))
                {
                   TARJ_numEntries(dfnum(iter)) ++;
                   loopsEntered --;
                }
                iter = TarjOuter(iter);
             }
          }
          delete ei;
       }
    }
}

void
TarjanIntervals::compute_number_of_entries_exits ()
{
    RIFGNodeId root = rifg->GetRootNode();
    find_entry_exit_edges_for_interval(root);
    number_exits_computed = true;
}


/*
 * return number of exits from the innermost loop that node is part of
 */
int 
TarjanIntervals::numExitEdges (RIFGNodeId node)
{
   if (!number_exits_computed)
      compute_number_of_entries_exits ();
   if (! NodeIsLoopHeader(node))
      node = TarjOuter(node);
   return (TARJ_numExits(dfnum(node)));
}

/*
 * return number of entries into the innermost loop that node is part of
 */
int 
TarjanIntervals::numEntryEdges (RIFGNodeId node)
{
   if (!number_exits_computed)
      compute_number_of_entries_exits ();
   if (! NodeIsLoopHeader(node))
      node = TarjOuter(node);
   return (TARJ_numEntries(dfnum(node)));
}

//
// Return number of loops exited in traversing g edge from src to sink
// (0 is normal).  a.k.a. getExits
//
int 
TarjanIntervals::tarj_exits(RIFGNodeId src, RIFGNodeId sink)
{
  RIFGNodeId lca = LCA(src, sink);
  if (lca == RIFG_NIL)
    return 0;

  DFNUM_t dfn_src = dfnum(src);
  DFNUM_t dfn_lca = dfnum(lca);
  return std::max(0, TARJ_level(dfn_src) - TARJ_level(dfn_lca));
}

/*
 *  Return number of loops entered in traversing g edge from src to sink
 *  (0 is normal).
 */
int 
TarjanIntervals::tarj_enters (RIFGNodeId src, RIFGNodeId sink)
{
  RIFGNodeId lca = LCA (src, sink);
  if (lca == RIFG_NIL) 
    return 0;

  DFNUM_t dfn_sink = dfnum(sink);
  DFNUM_t dfn_lca = dfnum(lca);
  return std::max(0, TARJ_level(dfn_sink) - TARJ_level(dfn_lca));
}

/*
 * compute both how many loops are entered, as well as how many loops are
 * exited by this edge; 
 * Return total (entered + exited); num entered/exited are stored in the
 * last two output arguments.
 */
int 
TarjanIntervals::tarj_entries_exits (RIFGNodeId src, RIFGNodeId sink, 
        int& numEntries, int& numExits)
{
    RIFGNodeId lca = LCA (src, sink);
    if (lca == RIFG_NIL) 
    {  
       numEntries = numExits = 0; 
    } else
    {
       DFNUM_t dfn_src = dfnum(src);
       DFNUM_t dfn_sink = dfnum(sink);
       DFNUM_t dfn_lca = dfnum(lca);
       numExits = std::max(0, TARJ_level(dfn_src) - TARJ_level(dfn_lca));
       numEntries = std::max(0, TARJ_level(dfn_sink) - TARJ_level(dfn_lca));
    }
    return (numEntries + numExits);
}

#if 0
//
// Return outermost loop exited in traversing g edge from src to sink
// (RIFG_NIL if no loops exited).
//
RIFGNodeId 
TarjanIntervals::getLoopExited(RIFGNodeId src, RIFGNodeId sink)
{
  RIFGNodeId lca = LCA(src, sink);
  if (lca == RIFG_NIL)
    return RIFG_NIL;

  if (lca == src)
    return RIFG_NIL;

  DFNUM_t dfn_src = dfnum(src);
  while (!Contains(TARJ_nodeid(TARJ_outer(dfn_src)), sink))
    dfn_src = TARJ_outer(dfn_src);
    
  if (TARJ_type(dfn_src) == NODE_INTERVAL
      || TARJ_type(dfn_src) == NODE_IRREDUCIBLE)
    return TARJ_nodeid(dfn_src);

  return RIFG_NIL;
}
#endif

/*
 *  Return outermost loop exited in traversing g edge from src to sink
 *  (RIFG_NIL if no loops exited).
 */
RIFGNodeId 
TarjanIntervals::tarj_loop_exited(RIFGNodeId src, RIFGNodeId sink)
{
    int loopsExited = tarj_exits(src, sink);
    if (loopsExited == 0)
       return RIFG_NIL;
       
    RIFGNodeId iter;
    if (NodeIsLoopHeader(src))
       iter = src;
    else
    {
       assert (TarjInners(src) == RIFG_NIL);
       iter = TarjOuter(src);
    }
    
    while(loopsExited > 0)
    {
       assert(iter != RIFG_NIL);
       assert(NodeIsLoopHeader(iter));
       if (loopsExited == 1)
          return (iter);
       
       loopsExited --;
       iter = TarjOuter(iter);
    }
    
    return RIFG_NIL;
}

/*
 *  Return outermost loop entered in traversing g edge from src to sink
 *  (RIFG_NIL if no loops entered).
 */
RIFGNodeId 
TarjanIntervals::tarj_loop_entered(RIFGNodeId src, RIFGNodeId sink)
{
    int loopsEntered = tarj_enters (src, sink);
//    printf ("Tarjans::tarj_loop_entered, src=%d, sink=%d, loopsEntered=%d\n", 
//         src, sink, loopsEntered);
    if (loopsEntered == 0)
       return RIFG_NIL;
       
    RIFGNodeId iter;
    if (NodeIsLoopHeader(sink))
       iter = sink;
    else
    {
       assert (TarjInners(sink) == RIFG_NIL);
       iter = TarjOuter(sink);
    }
    
    while(loopsEntered > 0)
    {
//       printf (" -> tarj_loop_entered, src=%d, sink=%d, loopsEntered=%d, iter=%d\n", 
//              src, sink, loopsEntered, iter);
       assert(iter != RIFG_NIL);
       assert (NodeIsLoopHeader(iter));
       if (loopsEntered == 1)
          return (iter);
       
       loopsEntered --;
       iter = TarjOuter(iter);
    }
    
    return RIFG_NIL;
}

