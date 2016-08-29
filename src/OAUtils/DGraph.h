// $Id: DGraph.h,v 1.3 2004/02/19 21:42:41 eraxxon Exp $
// -*-C++-*-
// * BeginRiceCopyright *****************************************************
// 
// Copyright ((c)) 2002, Rice University 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// 
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
// 
// * Neither the name of Rice University (RICE) nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
// 
// This software is provided by RICE and contributors "as is" and any
// express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular
// purpose are disclaimed. In no event shall RICE or contributors be
// liable for any direct, indirect, incidental, special, exemplary, or
// consequential damages (including, but not limited to, procurement of
// substitute goods or services; loss of use, data, or profits; or
// business interruption) however caused and on any theory of liability,
// whether in contract, strict liability, or tort (including negligence
// or otherwise) arising in any way out of the use of this software, even
// if advised of the possibility of such damage. 
// 
// ******************************************************* EndRiceCopyright *

// Best seen in 120-column wide window (or print in landscape mode).
//--------------------------------------------------------------------------------------------------------------------
// This file is part of Mint.
// Arun Chauhan (achauhan@cs.rice.edu), Dept of Computer Science, Rice University, 2001.
//--------------------------------------------------------------------------------------------------------------------

#ifndef DGraph_H
#define DGraph_H

// Mint headers
#include "BaseGraph.h"

#include <fstream>

//--------------------------------------------------------------------------------------------------------------------
// DGraph
/** DGraph is the base class for a general directed graph (DGraph) that is in turn derived from BaseGraph.  Algorithms
    that operate upon abstract directed graphs should, normally, use only this base DGraph class for maximum
    portability.

    No extra restrictions are placed on nodes and edges in addition to those imposed by BaseGraph.  This means that
    self-edges, and multiple edges between two nodes, are allowed.

    A directed graph, DGraph, extends BaseGraph by adding DFS and BFS iterators, as well as iterators to enumerate
    source nodes, sink nodes, incoming edges, and outgoing edges for a node.

    NOTE ON friend CLASSES: Many classes (especially DGraph, DGraph::Node, and DGraph::Edge) have many friend classes.
    This is *not* a kludge.  It is simulating "package" visiblity in Java.  We want a limited public interface to Node
    and Edge and yet give more permissions to methods within the Graph class.  */
//--------------------------------------------------------------------------------------------------------------------
class DGraph : public BaseGraph {
public:
  class DFSIterator;
  class BFSIterator;
  class IncomingEdgesIterator;
  class OutgoingEdgesIterator;
  class SourceNodesIterator;
  class SinkNodesIterator;
  class Edge;
  class Node;
  friend class DFSIterator;
  friend class BFSIterator;
  //------------------------------------------------------------------------------------------------------------------
  /** An node in an undirected graph has a list of neighboring nodes and a list of incident edges. */
  class Node : public BaseGraph::Node {
  public:
    Node () {}
    virtual ~Node () {
      incoming_edges.clear();
      outgoing_edges.clear();
    }
    int num_incoming () const { return incoming_edges.size(); }
    int num_outgoing () const { return outgoing_edges.size(); }
    virtual void draw (std::ostream& os, int info = 0) { }
    
  protected:
    std::list<Edge*> incoming_edges;
    std::list<Edge*> outgoing_edges;
  private:
    friend class DGraph;
    friend class DGraph::DFSIterator;
    friend class DGraph::BFSIterator;
    friend class DGraph::IncomingEdgesIterator;
    friend class DGraph::OutgoingEdgesIterator;
    friend class DGraph::SourceNodesIterator;
    friend class DGraph::SinkNodesIterator;
  };
  //------------------------------------------------------------------------------------------------------------------
  class Edge : public BaseGraph::Edge {
  public:
    Edge (Node* _source, Node* _sink) : BaseGraph::Edge(_source, _sink) {}
    virtual ~Edge () {}
    Node* source () const { return dynamic_cast<Node*>(n1); }
    Node* sink () const { return dynamic_cast<Node*>(n2); }
    Node* head () const { return sink(); }
    Node* tail () const { return source(); }
    friend class DGraph;
  };
  
  // lt_Edge: function object that compares by source and sink id.
  // Useful for sorting.
  class lt_Edge {
  public:
    // return true if e1 < e2; false otherwise
    bool operator()(const Edge* e1, const Edge* e2) const {
      unsigned int src1 = e1->source()->getId();
      unsigned int src2 = e2->source()->getId();
      if (src1 == src2) { 
	return (e1->sink()->getId() < e2->sink()->getId()); 
      } else {
	return (src1 < src2);
      }
    }
  };
  //------------------------------------------------------------------------------------------------------------------
  /** The DFSIterator here is just an extension of BaseGraph::DFSIterator to allow proper casting. */
  class DFSIterator : public BaseGraph::DFSIterator {
  public:
    DFSIterator (DGraph& g) : BaseGraph::DFSIterator(g) {}
    virtual ~DFSIterator () {}
    operator Node* () const { return dynamic_cast<Node*>(p); }
    Node* operator -> () const { return dynamic_cast<Node*>(p); }
   };
  //------------------------------------------------------------------------------------------------------------------
  /** The BFSIterator here is just an extension of BaseGraph::DFSIterator to allow proper casting. */
  class BFSIterator : public BaseGraph::BFSIterator {
  public:
    BFSIterator (DGraph& g) : BaseGraph::BFSIterator(g) {}
    virtual ~BFSIterator () {}
    operator Node* () const { return dynamic_cast<Node*>(p); }
    Node* operator -> () const { return dynamic_cast<Node*>(p); }
   };
  //------------------------------------------------------------------------------------------------------------------
  /** Iterator to enumerate all the incoming edges into a node. */
  class IncomingEdgesIterator : public BaseGraph::BaseEdgesIterator {
  public:
    IncomingEdgesIterator (const Node* n) { center = n;  iter = center->incoming_edges.begin(); }
    ~IncomingEdgesIterator () {}
    void operator++ () { ++iter; }
    operator bool () const { return (iter != center->incoming_edges.end()); }
    //    operator std::list<Edge*>::iterator& () { return iter; }
    Edge* operator-> () const { return *iter; }
    operator Edge* () const { return *iter; }
    Edge* Object() const { return *iter; }
    void Reset()        { iter = center->incoming_edges.begin(); }
  private:
    const Node*                      center;
  protected:
    std::list<Edge*>::const_iterator iter;
  };
  //------------------------------------------------------------------------------------------------------------------
  /** Iterator to enumerate all the outgoing edges from a node. */
  class OutgoingEdgesIterator : public BaseGraph::BaseEdgesIterator {
  public:
    OutgoingEdgesIterator (const Node* n) { center = n;  iter = center->outgoing_edges.begin(); }
    ~OutgoingEdgesIterator () {}
    void operator++ () { ++iter; }
    operator bool () const { return (iter != center->outgoing_edges.end()); }
    //    operator std::list<Edge*>::iterator& () { return iter; }
    Edge* operator-> () const { return *iter; }
    operator Edge* () const { return *iter; }
    Edge* Object() const { return *iter; }
    void Reset()        { iter = center->outgoing_edges.begin(); }
  private:
    const Node*                      center;
  protected:
    std::list<Edge*>::const_iterator iter;
  };
  //------------------------------------------------------------------------------------------------------------------
  /** Iterator to enumerate all the source nodes. */
  class SourceNodesIterator : public BaseGraph::BaseNodesIterator {
  public:
    SourceNodesIterator (const Node* n) { center = n; iter = center->incoming_edges.begin(); }
    ~SourceNodesIterator () {}
    void operator++ () { ++iter; }
    operator bool () const { return (iter != center->incoming_edges.end()); }
    Node* operator-> () const { Edge* e = *iter; return e->source(); }
    operator Node* () const { Edge* e = *iter; return e->source(); }
  private:
    const Node*                      center;
    std::list<Edge*>::const_iterator iter;
  };
  //------------------------------------------------------------------------------------------------------------------
  /** Iterator to enumerate all the sink nodes. */
  class SinkNodesIterator : public BaseGraph::BaseNodesIterator {
  public:
    SinkNodesIterator (const Node* n) { center = n; iter = center->outgoing_edges.begin(); }
    ~SinkNodesIterator () {}
    void operator++ () { ++iter; }
    operator bool () const { return (iter != center->outgoing_edges.end()); }
    Node* operator-> () const { Edge* e = *iter; return e->sink(); }
    operator Node* () const { Edge* e = *iter; return e->sink(); }
  private:
    const Node*                      center;
    std::list<Edge*>::const_iterator iter;
  };
  //------------------------------------------------------------------------------------------------------------------
  /** The NodesIterator is just and extension of BaseGraph::NodesIterator to provide access to DGraph nodes. */
  class NodesIterator : public BaseGraph::NodesIterator {
  public:
    NodesIterator (const DGraph& g) : BaseGraph::NodesIterator(g) {}
    virtual ~NodesIterator () {}
    operator Node* () const { return dynamic_cast<Node*>(*iter); }
    Node* operator -> () const { return dynamic_cast<Node*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------
  /** The BiDirNodesIterator is just an extension of BaseGraph::BiDirNodesIterator to provide access to DGraph nodes. */
  class BiDirNodesIterator : public BaseGraph::BiDirNodesIterator {
  public:
    BiDirNodesIterator (const DGraph& g) : BaseGraph::BiDirNodesIterator(g) {}
    BiDirNodesIterator (const DGraph& g, dirType d) : BaseGraph::BiDirNodesIterator(g, d) {}
    virtual ~BiDirNodesIterator () {}
    operator Node* () const { return dynamic_cast<Node*>(*iter); }
    Node* operator -> () const { return dynamic_cast<Node*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------
  /** The EdgesIterator is just and extension of BaseGraph::EdgesIterator to provide access to DGraph edges. */
  class EdgesIterator : public BaseGraph::EdgesIterator {
  public:
    EdgesIterator (const DGraph& g) : BaseGraph::EdgesIterator(g) {}
    virtual ~EdgesIterator () {}
    operator Edge* () const { return dynamic_cast<Edge*>(*iter); }
    Edge* operator -> () const { return dynamic_cast<Edge*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------
  DGraph () : BaseGraph() {}
  DGraph (Node* root) : BaseGraph (root) {}
  virtual ~DGraph () {}
  virtual void add (DGraph::Edge* e);
  virtual void add (DGraph::Node* n);
  virtual void remove (DGraph::Edge* e);
  virtual void remove (DGraph::Node* n);

private:
  BaseGraph::Node* create_DFS_links (BaseGraph::Node* start_node);
  BaseGraph::Node* create_BFS_links (BaseGraph::Node* start_node);
};
//--------------------------------------------------------------------------------------------------------------------

#endif
