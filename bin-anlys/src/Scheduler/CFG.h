/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: CFG.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Implements the Control Flow Graph (CFG) logic for a routine. 
 * It extends the base PrivateCFG class implementation with analysis 
 * needed by the MIAMI modeling engine.
 */

#ifndef _MIAMI_SCHEDTOOL_CFG_H
#define _MIAMI_SCHEDTOOL_CFG_H

//--------------------------------------------------------------------------------------------------------------------
#include <map>
#include "PrivateCFG.h"
#include "Dominator.h"
#include "private_routine.h"

namespace MIAMI 
{

// forward declaration of Routine
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
       fanin = 0;
       fanout = 0;
       
//       freq = 0;
       bcount = 0;
       ecount = 0;
       counter = 0;
       
       ninsts = 0;   // number of machine instructions in this block.
       inst_len = 0;
       in_use = 0;
    }
    
    virtual ~Node () {  }
    
    uint32_t Degree() const       { return (fanin+fanout); }
    uint32_t DegreeIn() const     { return (fanin); }
    uint32_t DegreeOut() const    { return (fanout); }
    inline bool hasCount() const  {
       return ((flags&(CFG_COUNTER_NODE|NODE_COUNT_COMPUTED))>0); 
    }
    inline bool hasPartialCount() const   { return (flags&(NODE_PARTIAL_COUNT)); }
    inline bool hasComputedCount() const  { return (flags&(NODE_COUNT_COMPUTED)); }

    inline uint64_t* Counter() { 
#if DEBUG_BLOCK_EDGE_COUNTS
       if (start_addr==debug_block)
          raise(SIGINT);
#endif
       return (&counter); 
    }

    friend class CFG;
    
    CFG* inCfg() const { return (static_cast<CFG*>(_in_cfg)); }
    
    Edge* firstOutgoing() const
    {
       return (static_cast<Edge*>(PrivateCFG::Node::firstOutgoing()));
    }
    Edge* firstIncoming() const
    {
       return (static_cast<Edge*>(PrivateCFG::Node::firstIncoming()));
    }

    // imported from the Sparc version of basic_block
    inline void setBlockCount(int64_t _bcount) { 
#if DEBUG_BLOCK_EDGE_COUNTS
       if (start_addr==debug_block)
       {
          fprintf(stderr, "Setting block count to %" PRId64 "\n", _bcount);
          raise(SIGINT);
       }
#endif
       bcount = _bcount; 
    }
    inline int64_t getBlockCount() { 
#if DEBUG_BLOCK_EDGE_COUNTS
       if (start_addr==debug_block)
       {
          fprintf(stderr, "Getting block count of %" PRId64 "\n", bcount);
          raise(SIGINT);
       }
#endif
       return bcount; 
    }

    inline int64_t ExecCount() const { 
#if DEBUG_BLOCK_EDGE_COUNTS
       if (start_addr==debug_block)
       {
          fprintf(stderr, "Getting block exec count of %" PRId64 "\n", ecount);
          raise(SIGINT);
       }
#endif
       return ecount; 
    }
    inline void setExecCount (int64_t _count) { 
#if DEBUG_BLOCK_EDGE_COUNTS
       if (start_addr==debug_block)
       {
          fprintf(stderr, "Setting block exec count to %" PRId64 "\n", _count);
          raise(SIGINT);
       }
#endif
       ecount = _count; 
    }
    
    inline void setInUse() { in_use = 1; }
    inline void unsetInUse() { in_use = 0; }
    inline int InUse() const { return (in_use); }

  protected:
    void write_label (std::ostream& os, addrtype offset, int info=0, bool html_escape=false);

  private:
    uint32_t fanin;  // number of incomming edges
    uint32_t fanout; // number of outgoing edges
    uint64_t counter;

//    int64_t freq;
    int64_t bcount;
    int64_t ecount;
    int in_use;
  };
  //------------------------------------------------------------------------------------------------------------------
  
  class Edge : public PrivateCFG::Edge {
  public:
    Edge (Node* n1, Node* n2, EdgeType _type = MIAMI_FALLTHRU_EDGE)
       : PrivateCFG::Edge(n1, n2, _type)
    {
       ecount = 0;
       ecount2 = 0;
       probab = 1.0;
#if DEBUG_BLOCK_EDGE_COUNTS
       if (n1->getStartAddress()==debug_edge)
       {
          fprintf(stderr, "Creating new edge of type %d\n", _type);
          raise(SIGINT);
       }
#endif
    }
    
    virtual ~Edge () {
    }

    Node* source() const { return dynamic_cast<Node*>(n1); }
    Node* sink() const   { return dynamic_cast<Node*>(n2); }

    inline bool hasCount() const  { 
       return ((flags&(CFG_COUNTER_EDGE|EDGE_COUNT_COMPUTED|EDGE_COUNT_GUESSED))>0); 
    }
    inline bool hasGuessedCount() const  { 
       return ((flags&EDGE_COUNT_GUESSED)>0); 
    }
    
    friend class CFG;
    
    // imported from the Sparc version of edge
    inline int64_t ExecCount () const 
    { 
#if DEBUG_BLOCK_EDGE_COUNTS
       if (source()->getStartAddress()==debug_edge)
       {
          fprintf(stderr, "Getting edge exec count of %" PRId64 "\n", ecount);
          raise(SIGINT);
       }
#endif
       return ecount; 
    }
    inline void setExecCount (int64_t _count) { 
#if DEBUG_BLOCK_EDGE_COUNTS
       if (source()->getStartAddress()==debug_edge)
       {
          fprintf(stderr, "Setting edge exec count to %" PRId64 "\n", _count);
          raise(SIGINT);
       }
#endif
       ecount = _count; 
    }
    inline int64_t secondExecCount () const 
    { 
#if DEBUG_BLOCK_EDGE_COUNTS
       if (source()->getStartAddress()==debug_edge)
       {
          fprintf(stderr, "Getting edge exec count of %" PRId64 "\n", ecount2);
          raise(SIGINT);
       }
#endif
       return ecount2; 
    }
    inline void setSecondExecCount (int64_t _count) 
    { 
#if DEBUG_BLOCK_EDGE_COUNTS
       if (source()->getStartAddress()==debug_edge)
       {
          fprintf(stderr, "Setting edge second exec count to %" PRId64 "\n", _count);
          raise(SIGINT);
       }
#endif
       ecount2 = _count; 
    }
    inline float Probability () const { return probab; }
    inline void setProbability (float _probab) { probab = _probab; }
  
    
  protected:
    void write_label (std::ostream& os);
    
  private:
    int64_t ecount;
    int64_t ecount2;
    float probab;
  };
  //------------------------------------------------------------------------------------------------------------------
  
  typedef std::list <Node*> NodeList;
  typedef std::list <Edge*> EdgeList;
  typedef std::vector <Node*> NodeArray;
  typedef std::vector <Edge*> EdgeArray;
  typedef std::map <Node*, int> NodeIntMap;
  typedef std::set <Node*> NodeSet;
  typedef std::map <Node*, Node*> NodeNodeMap;
  typedef std::map <unsigned int, NodeSet> Ui2NSMap;
  
  // node and edge iterators
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
  class IncomingEdgesIterator : public DGraph::IncomingEdgesIterator {
  public:
    IncomingEdgesIterator (const Node  *n) : DGraph::IncomingEdgesIterator(n) {}
    virtual ~IncomingEdgesIterator () {}
    Edge* operator-> () const { return dynamic_cast<CFG::Edge*>(*iter); }
    operator Edge* () const { return dynamic_cast<CFG::Edge*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------
  // enumerate outgoing edges
  //------------------------------------
  class OutgoingEdgesIterator : public DGraph::OutgoingEdgesIterator {
  public:
    OutgoingEdgesIterator (const Node  *n) : DGraph::OutgoingEdgesIterator(n) {}
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
    PredecessorNodesIterator (const Node* n) { 
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
    SuccessorNodesIterator (const Node* n) { 
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
  class NodesIterator : public BaseGraph::NodesIterator {
  public:
    NodesIterator (const CFG &g) : BaseGraph::NodesIterator(g) {}
    virtual ~NodesIterator () {}
    Node* operator-> () const { return dynamic_cast<CFG::Node*>(*iter); }
    operator Node* () const  { return dynamic_cast<CFG::Node*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------------------------------------------------------------------------------------
  /** The EdgesIterator is just an extension of BaseGraph::EdgesIterator */
  class EdgesIterator : public BaseGraph::EdgesIterator {
  public:
    EdgesIterator (const CFG& g) : BaseGraph::EdgesIterator(g) {}
    virtual ~EdgesIterator () {}
    operator Edge* () const { return dynamic_cast<Edge*>(*iter); }
    Edge* operator -> () const { return dynamic_cast<Edge*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------
   //------------------------------------------------------------------------------------------------------------------
   class EdgeListCompare
   {
   public:
      bool operator() (const EdgeList* el1, const EdgeList* el2) const;
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
  
   int loadFromFile(FILE *fd, MIAMI::EntryMap& entries, MIAMI::EntryMap& returns, addrtype _reloc);
   
   Node* CfgEntry() const    { return (static_cast<CFG::Node*>(cfg_entry)); }
   Node* CfgExit() const     { return (static_cast<CFG::Node*>(cfg_exit)); }
   
   int computeBBAndEdgeCounts();
   int addBBAndEdgeCounts();
   void deleteUntakenCallSiteReturnEdges();
   
private:
   void draw_dominator_tree (BaseDominator *bdom, const char *prefix);
   void computeNodeDegrees();
   
protected:
};
//--------------------------------------------------------------------------------------------------------------------

// a few EdgeFilterType functions, must be updated for a CFG
extern bool draw_all(CFG::Edge* e);

}  // namespace MIAMI

#endif
