/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.  
 */
/* 
 * File: CFG.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Implements the Control Flow Graph (CFG) logic for a routine. It extends 
 * the base PrivateCFG class implementation with analysis specifically 
 * needed for data reuse analysis.
 */

#ifndef _MIAMI_MRDTOOL_CFG_H
#define _MIAMI_MRDTOOL_CFG_H

//--------------------------------------------------------------------------------------------------------------------
#include "PrivateCFG.h"
#include "load_module.h"

namespace MIAMI 
{

// temporary declaration of Routine
class Routine;

//--------------------------------------------------------------------------------------------------------------------
/** CFG is a DGraph (directed graph) with enhanced nodes and edges.  
    Each node in the CFG represents a block of machine instructions, 
    and an edge represents a control flow between blocks. 
*/
class CFG : public PrivateCFG {
public:
  class Node;
  friend class Node;
  class Edge;
  friend class Edge;
  
  CFG (Routine* _r, const std::string& func_name);
  virtual ~CFG ();
  
  //------------------------------------------------------------------------------------------------------------------
  class Node : public PrivateCFG::Node {
  public:
    Node (CFG *_g, addrtype _start_addr, addrtype _end_addr, NodeType _type = MIAMI_CODE_BLOCK) 
         : PrivateCFG::Node(_g, _start_addr, _end_addr, _type)
    { 
    }
    
    virtual ~Node () {  }
    
    friend class CFG;
    
    CFG* inCfg() const { return (static_cast<CFG*>(_in_cfg)); }
    void record_scopeid_for_memory_references(LoadModule *img, uint32_t scopeId);

  private:
  };
  //------------------------------------------------------------------------------------------------------------------
  
  class Edge : public PrivateCFG::Edge {
  public:
    Edge (Node* n1, Node* n2, EdgeType _type = MIAMI_FALLTHRU_EDGE) 
         : PrivateCFG::Edge(n1, n2, _type)
    {
    }
    virtual ~Edge () {
    }

    Node* source() const { return dynamic_cast<Node*>(n1); }
    Node* sink() const   { return dynamic_cast<Node*>(n2); }
    
    friend class CFG;
    
  private:
    
  };
  //------------------------------------------------------------------------------------------------------------------
  
  virtual Node* MakeNode(addrtype _start_addr, addrtype _end_addr, NodeType _type = MIAMI_CODE_BLOCK)
  {
     return (new Node(this, _start_addr, _end_addr, _type));
  }
  virtual Edge* MakeEdge(PrivateCFG::Node* n1, PrivateCFG::Node* n2, EdgeType _type = MIAMI_FALLTHRU_EDGE)
  {
     return (new Edge(static_cast<Node*>(n1), static_cast<Node*>(n2), _type));
  }
  
  typedef std::list <Node*> NodeList;
  typedef std::list <Edge*> EdgeList;
  typedef std::vector <Node*> NodeArray;
  typedef std::map <Node*, int> NodeIntMap;
  typedef std::set <Node*> NodeSet;
  typedef std::map <unsigned int, NodeSet> Ui2NSMap;
  
  class NodeListIterator : public Iterator {
  public:
    NodeListIterator (NodeList* nl) { list = nl; iter = list->begin(); }
    virtual ~NodeListIterator () {}
    void operator ++ () { if (iter != list->end()) ++iter; }
    operator bool () const { return (iter != list->end()); }
    operator Node* () const { return *iter; }
  private:
    NodeList* list;
    std::list<Node*>::iterator iter;
  };
  //------------------------------------------------------------------------------------------------------------------
  class EdgeListIterator : public Iterator {
  public:
    EdgeListIterator (EdgeList* el) { list = el; iter = list->begin(); }
    virtual ~EdgeListIterator () {}
    void operator ++ () { if (iter != list->end()) ++iter; }
    operator bool () const { return (iter != list->end()); }
    operator Edge* () const { return *iter; }
  private:
    EdgeList* list;
    std::list<Edge*>::iterator iter;
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------
  // enumerate nodes in DFS order
  //------------------------------------
  class DFSIterator : public DGraph::DFSIterator {
  public:
    DFSIterator (CFG  &g) : DGraph::DFSIterator(g) {}
    virtual ~DFSIterator () {}
    operator Node* () const { return dynamic_cast<CFG::Node*>(p); }
    void operator++ () { ++p; }
    operator bool () const { return (bool)p; }
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------
  // enumerate incoming edges
  //------------------------------------
  class IncomingEdgesIterator : public PrivateCFG::IncomingEdgesIterator {
  public:
    IncomingEdgesIterator (Node  *n) : PrivateCFG::IncomingEdgesIterator(n) {}
    virtual ~IncomingEdgesIterator () {}
    Edge* operator-> () const { return dynamic_cast<CFG::Edge*>(*iter); }
    operator Edge* () const { return dynamic_cast<CFG::Edge*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------
  // enumerate outgoing edges
  //------------------------------------
  class OutgoingEdgesIterator : public PrivateCFG::OutgoingEdgesIterator {
  public:
    OutgoingEdgesIterator (Node  *n) : PrivateCFG::OutgoingEdgesIterator(n) {}
    virtual ~OutgoingEdgesIterator () {}
    Edge* operator-> () const { return dynamic_cast<CFG::Edge*>(*iter); }
    operator Edge* () const { return dynamic_cast<CFG::Edge*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------
  // enumerate predecessor nodes (only over scheduling edges)
  //------------------------------------
  class PredecessorNodesIterator {
  public:
    PredecessorNodesIterator (Node* n) { 
       iter = new IncomingEdgesIterator (n);
    }
    virtual ~PredecessorNodesIterator () {
       delete iter;
    }
    void operator++ () { 
       if ((bool)(*iter))
          ++ (*iter);
    }
    operator bool () const { return (bool)(*iter); }
    Node* operator-> () const { 
       return dynamic_cast<CFG::Node*>((*iter)->source()); 
    }
    operator Node* () const { 
       return dynamic_cast<CFG::Node*>((*iter)->source()); 
    }
  private:
    IncomingEdgesIterator *iter;
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------
  // enumerate succesor nodes (only over scheduling edges)
  //------------------------------------
  class SuccessorNodesIterator {
  public:
    SuccessorNodesIterator (Node* n) { 
       iter = new OutgoingEdgesIterator (n);
    }
    virtual ~SuccessorNodesIterator () {
       delete iter;
    }
    void operator++ () { 
       if ((bool)(*iter))
          ++ (*iter);
    }
    operator bool () const { return (bool)(*iter); }
    Node* operator-> () const { 
       return dynamic_cast<CFG::Node*>((*iter)->sink()); 
    }
    operator Node* () const { 
       return dynamic_cast<CFG::Node*>((*iter)->sink()); 
    }
  private:
    OutgoingEdgesIterator *iter;
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------
  // enumerate nodes in no particular order
  //------------------------------------
  class NodesIterator : public PrivateCFG::NodesIterator {
  public:
    NodesIterator (const CFG &g) : PrivateCFG::NodesIterator(g) {}
    virtual ~NodesIterator () {}
    Node* operator-> () const { return dynamic_cast<CFG::Node*>(*iter); }
    operator Node* ()  const  { return dynamic_cast<CFG::Node*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------------------------------------------------------------------------------------
  /** The EdgesIterator is just an extension of PrivateCFG::EdgesIterator */
  class EdgesIterator : public PrivateCFG::EdgesIterator {
  public:
    EdgesIterator (const CFG& g) : PrivateCFG::EdgesIterator(g) {}
    virtual ~EdgesIterator () {}
    operator Edge* () const { return dynamic_cast<Edge*>(*iter); }
    Edge* operator -> () const { return dynamic_cast<Edge*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------

   using PrivateCFG::add;
   virtual void add (DGraph::Edge* e);
  
   Node* CfgEntry()    { return (static_cast<CFG::Node*>(cfg_entry)); }
   Node* CfgExit()     { return (static_cast<CFG::Node*>(cfg_exit)); }
   
   void saveToFile(FILE *fd, addrtype base_addr);

   void FindScopesRecursively(RIFGNodeId root, TarjanIntervals *tarj, MiamiRIFG *ecfg,
               LoadModule *img, uint32_t parentId);
   void FindLoopEntryExitEdges(RIFGNodeId root, TarjanIntervals *tarj, MiamiRIFG *ecfg,
               LoadModule *img);

private:
   
};
//--------------------------------------------------------------------------------------------------------------------

// a few EdgeFilterType functions, must be updated for a CFG
extern bool draw_all(CFG::Edge* e);

}  // namespace MIAMI

#endif
