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
           Port to OpenAnalysis by Nathan Tallent (and renamed NestedSCR
	     from TarjanIntervals)
  \version $Id$

  Copyright (c) 2002-2005, Rice University <br>
  Copyright (c) 2004-2005, University of Chicago <br>
  Copyright (c) 2006, Contributors <br>
  All rights reserved. <br>
  See ../../../Copyright.txt for details. <br>
*/

/* 
   NOTES: 
     - A clean re-write in C++ would greatly enhance the style and
       clarity of the code.
     - Nathan Tallent: Added the DGraph-node -> SCRId map to get
       around the unmet assumption that DGraph-node-ids go from 0-n.
*/

#ifndef _TARJAN_INTERVALS_H
#define _TARJAN_INTERVALS_H

#include <list>
#include <iostream>
#include "RIFG.h"
#include "UnionFindUniverse.h"


//***************************************************************************
// Class: TarjanIntervals
//
// Description:
//    A Tree of strongly connected regions
//
//***************************************************************************

class TarjWork;
class TarjTreeNode;

  
class TarjanIntervals {
public:
  
  enum Node_t { NODE_NOTHING, 
		NODE_ACYCLIC, 
		NODE_INTERVAL, 
		NODE_IRREDUCIBLE };

  enum Edge_t { EDGE_NORMAL, 
		EDGE_LOOP_ENTRY, 
		EDGE_IRRED_ENTRY, 
		EDGE_ITERATE };

  typedef unsigned int DFNUM_t;

public:
  // Construct the Tarjan interval tree for the graph.
  TarjanIntervals(RIFG &_g);
  ~TarjanIntervals();
  
  RIFG* getRIFG() { return rifg; }


  // Obtain a pointer to tree. [FIXME:old Do we want to allow this?]
  TarjTreeNode* getTree();


  bool IsFirst(RIFGNodeId id);
  bool IsLast(RIFGNodeId id);
  bool IsHeader(RIFGNodeId id);

  /* check if a node is a reducible or irreducible interval
   */
  bool NodeIsLoopHeader(RIFGNodeId id);

  // Given an interval header, return its first nested interval.
  RIFGNodeId TarjInners(RIFGNodeId id);

  // Given an interval header, return its last nested interval.
  RIFGNodeId TarjInnersLast(RIFGNodeId id);

  // Given an interval, return its parent (header) interval.
  RIFGNodeId TarjOuter(RIFGNodeId id);

  // Given an interval, return its next sibling interval. 
  RIFGNodeId TarjNext(RIFGNodeId id);


  // Given an interval, return its nesting level.
  int GetLevel(RIFGNodeId id);

  // Given an interval, return its unique index.
  int LoopIndex(RIFGNodeId id);

  // Given a node, return its type (acyclic, interval, irreducible).
  Node_t getNodeType(RIFGNodeId id);
  Edge_t getEdgeType(RIFGNodeId src, RIFGNodeId sink);

  // Return true if the edge is a backedge, false otherwise.
  bool IsBackEdge(RIFGEdgeId e);

/*
  // Return number of loops exited in traversing g edge from src to sink.
  int getExits(RIFGNodeId src, RIFGNodeId sink);

  // Return outermost loop exited in traversing g edge from src to sink
  // (RIFG_NIL if no loops exited).
  RIFGNodeId getLoopExited(RIFGNodeId src, RIFGNodeId sink);
*/
  int numExitEdges(RIFGNodeId node);  // return number of exits from the 
                                      // innermost loop this node is part of
  int numEntryEdges(RIFGNodeId node); // return number of entries into the 
                                      // innermost loop this node is part of
  
  int tarj_exits (RIFGNodeId src, RIFGNodeId sink);
  RIFGNodeId tarj_loop_exited (RIFGNodeId src, RIFGNodeId sink);

  int tarj_enters (RIFGNodeId src, RIFGNodeId sink);
  RIFGNodeId tarj_loop_entered (RIFGNodeId src, RIFGNodeId sink);

  // compute both how many loops are entered, as well as how many loops are
  // exited by this edge; return total; num entries/exits are stored in the
  // last two arguments on exit
  int tarj_entries_exits (RIFGNodeId src, RIFGNodeId sink, int& numEntries,
               int& numExits);
  
  
  bool Contains(RIFGNodeId a, RIFGNodeId b);

  // Return the least-common-ancestor of two nodes in the tree.
  RIFGNodeId LCA(RIFGNodeId a, RIFGNodeId b);

  
  // Updates the prenumbering of the tree.  Useful if some change
  // has been made to the cfg where one knows how to update the
  // interval tree (as when adding pre-header nodes).
  void Renumber();
  void Prenumber(int n);


  // Pretty-print the interval tree. [N.B.: ignores 'os' at the moment]
  void Dump(std::ostream& os);
  
private: // methods
  void Create();

  void Init();
  void InitArrays();
  void DFS(RIFGNodeId n);
  void FillPredLists();
  void GetTarjans();
  void Build();
  void Sort();
  void ComputeIntervalIndex();
  void ComputeIntervalIndexSubTree(DFNUM_t node);
  void FreeWork();

  void ComputeDepth(DFNUM_t v);

  void compute_number_of_entries_exits ();
  void find_entry_exit_edges_for_interval (RIFGNodeId node);

  void DumpSubTree(std::ostream& os, int node, int indent);

  int FIND(int v);
  void UNION(int i, int j, int k);

private: // data
  // The graph to analyze.
  RIFG* rifg;

  // Work data
  UnionFindUniverse *uf;
  TarjWork *wk;       // maps RIFGNodeId to TarjWork
  TarjTreeNode *tarj; // maps RIFGNodeId to TarjTreeNode
  bool number_exits_computed;
  
  // List of nodes in reverse topological order.
  std::list<RIFGNodeId> rev_top_list;

  // A map from RIFGNodeId to DFS number.
  std::map<RIFGNodeId, DFNUM_t> nodeid_to_dfnum_map;
};


#endif
