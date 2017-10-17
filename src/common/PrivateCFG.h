/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: PrivateCFG.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Base class for control flow graph support in MIAMI.
 */

#ifndef _MIAMI_PRIVATE_CFG_H
#define _MIAMI_PRIVATE_CFG_H

#include "miami_types.h"

//--------------------------------------------------------------------------------------------------------------------
// standard headers
#ifdef NO_STD_CHEADERS
# include <string.h>
#else
# include <cstring>
#endif

// standard header
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

// STL headers
#include <list>
#include <set>
#include <map>
#include <queue>
#include <functional>
#include <csignal>

// OpenAnalysis headers
#include "OAUtils/DGraph.h"

// Tarjans headers
#include "tarjans/RIFG.h"
#include "tarjans/TarjanIntervals.h"

// MIAMI headers
#include "uipair.h"
#include "printable_class.h"

#include "unionfind.h"
#include "uop_code_cache.h"

// define bit flags for nodes
#define CFG_COUNTER_NODE        0x2
#define NODE_COUNT_COMPUTED     0x4
#define NODE_PARTIAL_COUNT      0x8
// next flag defines that the node was identified as a routine entry point
#define ROUTINE_ENTRY_NODE      0x10
#define NODE_IS_CFG_ENTRY       0x80
#define NODE_IS_CFG_EXIT        0x100
#define NODE_IS_CFG_ENTRY_EXIT  0x180


namespace MIAMI 
{

#define DEBUG_BLOCK_EDGE_COUNTS 0

#if DEBUG_BLOCK_EDGE_COUNTS
const addrtype debug_edge = 0x4116981;
const addrtype debug_block = 0x40e4501;
#endif

/* Define some bit flags for CFG */
// - first flag must be set by the child class that constructs the initial
//   control flow graph
#define CFG_CONSTRUCTED         0x1
// - graph have been modified
#define CFG_GRAPH_IS_MODIFIED   0x2
// - graph top nodes are routine entry points, do not recompute
#define CFG_HAS_ENTRY_POINTS   0x4
// - next flag indicates if graph was freshly pruned
#define CFG_GRAPH_PRUNED        0x200
// - indicates if cycles have been computed
#define CFG_CYCLES_COMPUTED     0x400
// - indicates if graph was updated with machine information
#define CFG_MACHINE_DEFINED     0x800
// - indicates the scheduling has been computed for this graph
#define CFG_SCHEDULE_COMPUTED   0x1000

/* I have to check the graph's state in the various routines that work on 
 * the graph, and to update its state accordingly after each transformation
 */
// Following macros can work with one or multiple flags at a time.
// Multiple flags must be passed with + or | between them.
#define setCfgFlags(x)     cfgFlags |= (x)
#define removeCfgFlags(x)  cfgFlags &= (~(x))
#define HasNoFlags(x)      (! (cfgFlags & (x)) )
#define HasAllFlags(x)     (! ((~cfgFlags) & (x)) )
#define HasSomeFlags(x)    (cfgFlags & (x))

#define  ANY_GRAPH_NODE  ((void*)(-1))

// Forward declarations
class PrivateRoutine;
class MiamiRIFG;

//--------------------------------------------------------------------------------------------------------------------
/** CFG is a DGraph (directed graph) with enhanced nodes and edges.  
    Each node in the CFG represents a block of machine instructions, 
    and an edge represents a control flow between blocks. 
*/
class PrivateCFG : public DGraph {
public:
  class Node;
  friend class Node;
  class Edge;
  friend class Edge;
  
  typedef bool (*EdgeFilterType)(Edge*);

  typedef std::multimap<addrtype, Edge*> EdgeMap;

  PrivateCFG (PrivateRoutine* _r, const std::string& func_name);
  virtual ~PrivateCFG ();
  
  unsigned int new_marker(unsigned int delta=1) { 
     if (delta<1) delta = 1;
     topMarker += delta;
     return (topMarker); 
  }

  // enumerate possible edge types
  typedef enum {
       ANY_EDGE_TYPE = -1
       ,MIAMI_FALLTHRU_EDGE
       ,MIAMI_DIRECT_BRANCH_EDGE
       ,MIAMI_INDIRECT_BRANCH_EDGE
       ,MIAMI_RETURN_EDGE
       ,MIAMI_BYPASS_EDGE  // used for REP blocks to indicate REP with count 0
       ,MIAMI_CFG_EDGE
       ,MIAMI_NUM_EDGE_TYPES
  } EdgeType;

  typedef enum {
       ANY_BLOCK_TYPE = -1
       ,MIAMI_CODE_BLOCK
       ,MIAMI_REP_BLOCK
       ,MIAMI_CALL_SITE
       ,MIAMI_INNER_LOOP
       ,MIAMI_DATA_BLOCK
       ,MIAMI_CFG_BLOCK
       ,MIAMI_NUM_BLOCK_TYPES
  } NodeType;

  //------------------------------------------------------------------------------------------------------------------
  class Node : public DGraph::Node, public PrintableClass {
  public:
    Node (PrivateCFG *_g, addrtype _start_addr, addrtype _end_addr, NodeType _type = MIAMI_CODE_BLOCK) 
         : DGraph::Node()
    { 
       start_addr = _start_addr;
       end_addr = _end_addr;
       _in_cfg = _g;
       type = _type;
       Ctor();
    }
    
    virtual ~Node () {  
       if (inst_len)
          delete[] inst_len;
       inst_len = 0;
       if (ucc)
          delete (ucc);
       ucc = 0;
    }

    // split node at split_addr creating a new block for the second
    // part of the old block, and adjust the original block's end address
    // to be the splitting point.
    // incEdges will store the number of prospective edges found for
    // the newly created block.
    Node* SplitAt(addrtype split_addr, int *incEdges, NodeType _type = MIAMI_CODE_BLOCK);
    
    // getId: An id unique within instances of PrivateCFG::Node
    virtual ObjId getId() const { return id; }
    // resetIds: reset id numbering

    void dump (std::ostream& os) { }
    void longdump (PrivateCFG *, std::ostream& os);
    void longdump (PrivateCFG * _cfg) { longdump(_cfg, std::cout); } 

    void drawNode (std::ostream& os, addrtype offset, bool level_col, int info = 0);
    void drawNode (addrtype offset, bool level_col, int info = 0) 
    { 
       drawNode(std::cout, offset, level_col, info); 
    }
    void PrintObject (ostream& os) const;
    
    void visit(unsigned int m)          { _visited = m; }
    unsigned int getVisitedWith() const { return (_visited); }
    bool visited(unsigned int m) const { return (m==_visited); }
    bool visited(unsigned int mL, unsigned int mU) const 
    {
       return (mL<=_visited && _visited<mU); 
    }
    
    addrtype getStartAddress() const { return (start_addr); }
    addrtype getEndAddress() const   { return (end_addr);   }
    NodeType getType() const   { return (type); }
    int getLevel() const     { return (_level); }
    void setLevel(int lev)   { _level = lev; }
    addrtype Size() const    { return (end_addr - start_addr); }

    int getRank() const     { return (rank); }
    void setRank(int _rank) { rank = _rank; }

    // additional rank values, meaningful only for nodes on a cyclic path
    int getPathRank() const     { return (path_rank); }
    void setPathRank(int _rank) { path_rank = _rank; }

    bool isCfgNode () const  { return (flags & NODE_IS_CFG_ENTRY_EXIT); }
    bool isCfgEntry () const { return (flags & NODE_IS_CFG_ENTRY); }
    bool isCfgExit () const  { return (flags & NODE_IS_CFG_EXIT); }

    bool is_inner_loop () const  { return (type == MIAMI_INNER_LOOP); }
    bool is_delay_block() const  { return (false); }  // no delay slots on x86
    
    bool isCodeNode() const;
    bool isCallSurrogate() const { return (type==MIAMI_CALL_SITE); }
    void setTarget(addrtype _target)  { target = _target; }
    addrtype getTarget() const  { return (target); }

    friend class PrivateCFG;
    
    PrivateCFG* inCfg() const { return (_in_cfg); }
    PrivateRoutine* InRoutine() const { 
       if (_in_cfg) return (_in_cfg->InRoutine());
       else return (0);
    }
    addrtype RelocationOffset() const;
    
    static const char* nodeTypeToString(NodeType tt);
    static const char* nodeTypeColor(NodeType tt);
    
    void setNodeFlags(uint32_t _f) { flags |= _f; }
    uint32_t getNodeFlags() const    { return (flags); }

    inline void setCounterNode() { flags |= CFG_COUNTER_NODE; }
    inline bool isCounterNode() const { return (flags & CFG_COUNTER_NODE); }

    inline void setRoutineEntry() { flags |= ROUTINE_ENTRY_NODE; }
    inline bool isRoutineEntry() const { return (flags & ROUTINE_ENTRY_NODE); }
    
    Edge* firstOutgoing() const
    {
       std::list<DGraph::Edge*>::const_iterator iter = outgoing_edges.begin();
       if (iter==outgoing_edges.end())
          return (NULL);
       else
          return (static_cast<Edge*>(*iter));
    }
    Edge* firstIncoming() const
    {
       std::list<DGraph::Edge*>::const_iterator iter = incoming_edges.begin();
       if (iter==incoming_edges.end())
          return (NULL);
       else
          return (static_cast<Edge*>(*iter));
    }

    Edge* firstOutgoingOfType(EdgeType tt) const
    {
       std::list<DGraph::Edge*>::const_iterator iter = outgoing_edges.begin();
       for ( ; iter!=outgoing_edges.end() ; ++iter)
       {
          Edge *e = static_cast<Edge*>(*iter);
          if (e->getType() == tt)
             return (e);
       }
       return (NULL);
    }
    Edge* firstIncomingOfType(EdgeType tt) const
    {
       std::list<DGraph::Edge*>::const_iterator iter = incoming_edges.begin();
       for ( ; iter!=incoming_edges.end() ; ++iter)
       {
          Edge *e = static_cast<Edge*>(*iter);
          if (e->getType() == tt)
             return (e);
       }
       return (NULL);
    }
    
    inline int numInstructions() const  { return (ninsts); }
    int lengthOfInstruction(int i) const
    {  
       if (inst_len && i>=0 && i<ninsts)
          return (inst_len[i]);
       else
          return (0);
    }
    void computeInstructionLengths();

    inline void setExitlessLoopHead()  { _exitless_loop = true; }
    inline bool isExitlessLoopHead()  { return (_exitless_loop); }

    inline void setLoopEntryBlock ()  { _loop_entry_block = true; }
    inline bool isLoopEntryBlock ()  { return (_loop_entry_block); }

    UopCodeCache* MicroOpCode()  { 
       if (! ucc)
       {
          bool has_call = (num_outgoing()>0 && 
                 firstOutgoing()->sink()->isCallSurrogate());  // succesor is call surrogate
          addrtype reloc = RelocationOffset();
          ucc = new UopCodeCache(start_addr, end_addr, reloc, has_call);
       }
       return (ucc); 
    }
    
    void DeleteMicroOpCode() {
       if (ucc)
          delete ucc;
       ucc = 0;
    }

    inline bool isMarkedWith(unsigned int _mark2) { return (_mark2==_marker2); }
    inline void markWith(unsigned int _mark2) { _marker2 = _mark2; }
    inline unsigned int getMarkedWith() { return (_marker2); }
    
  protected:
    virtual void write_label (std::ostream& os, addrtype offset, int info=0, bool html_escape=false);
    void Ctor() 
    { 
       if (_in_cfg)
          id = _in_cfg->getNextNodeId(); 
       else
          id = 0;
       _visited = 0; tempVal = 0;
       flags = 0; 
       _level = 0; 
       rank = 0;
       path_rank = 0;
       target = MIAMI_NO_ADDRESS;
       ninsts = 0;
       inst_len = 0;
       ucc = 0;
       _exitless_loop = false;
       _loop_entry_block = false;
       _marker2 = 0;
    }

    PrivateCFG* _in_cfg;
    int _level;
    int rank, path_rank;

    unsigned int _visited;
    // a second maker for blocks
    unsigned int _marker2;

    unsigned int tempVal;
    ObjId id; // 0 is reserved; address of instruction or unique id
    addrtype start_addr, end_addr;
    addrtype target;   // right now this is used for call sites only 
    NodeType type;
    uint32_t flags;

    int32_t ninsts;     // number of machine instructions in block
    int32_t *inst_len;  // array to store instruction lengths

    // keep a linear representation of the micro-ops inside a block
    UopCodeCache *ucc;

    // marker that specifies if this is a head block for a loop with no exit
    // edge
    unsigned short _exitless_loop : 1;
    // mark if this is a loop entry block. For irreducible intervals we
    // can have more than one entry
    unsigned short _loop_entry_block : 1;
  };
  //------------------------------------------------------------------------------------------------------------------
  typedef std::map <unsigned int, Edge*> UiEdgeMap;
  typedef std::set <Edge*> EdgeSet;
  
  enum _edge_flags {
//     CFG_BACK_EDGE = 0x1,     /* set by simple DFS analysis during rank computation, Should not be used anymore. */
     CFG_COUNTER_EDGE = 0x2
     ,EDGE_COUNT_COMPUTED = 0x4
     ,CFG_LOOP_BACK_EDGE = 0x8   /* set based on Tarjan analysis. Use this flag for important things */
     ,EDGE_COUNT_GUESSED = 0x10
  };
  
  class Edge : public DGraph::Edge, public PrintableClass {
  public:
    Edge (Node* n1, Node* n2, EdgeType _type = MIAMI_FALLTHRU_EDGE);
    virtual ~Edge () {
    }

    Node *ChangeSource(Node *src);
    Node *ChangeSink(Node *sink);
    inline EdgeType getType() const      { return type; }
    
    // getId: An id unique within instances of PrivateCFG::Edge
    inline virtual ObjId getId() const { return id; }
    // resetIds: reset id numbering
    
    inline Node* source() const { return dynamic_cast<Node*>(n1); }
    inline Node* sink() const   { return dynamic_cast<Node*>(n2); }
    
    void dump (std::ostream& os);
    void dump () { dump(std::cout); }

    void drawEdge (std::ostream& os, EdgeFilterType f, bool level_col);
    void drawEdge (EdgeFilterType f, bool level_col) 
    { 
       drawEdge(std::cout, f, level_col);
    }
    
    void PrintObject (ostream& os) const;

    inline void visit(unsigned int m)         { _visited = m; }
    inline bool visited(unsigned int m) const { return (m==_visited); }
    inline bool visited(unsigned int mL, unsigned int mU) const 
    {
       return (mL<=_visited && _visited<mU); 
    }
    
#if 0
    inline void setBackEdge() { flags |= CFG_BACK_EDGE; }
    inline bool isBackEdge() const  { return (flags & CFG_BACK_EDGE); }
#endif
    inline void setCounterEdge() { flags |= CFG_COUNTER_EDGE; }
    inline bool isCounterEdge() const { return (flags & CFG_COUNTER_EDGE); }
    inline uint64_t* Counter() {
#if DEBUG_BLOCK_EDGE_COUNTS
       if (source()->getStartAddress()==debug_edge)
          raise(SIGINT);
#endif
       return (&counter); 
    }

    static const char* edgeTypeToString(EdgeType);
    static const char* edgeTypeColor(EdgeType);
    static const char* edgeTypeStyle(EdgeType);

    inline void setLoopBackEdge() { flags |= CFG_LOOP_BACK_EDGE; }
    inline bool isLoopBackEdge() const  { return (flags & CFG_LOOP_BACK_EDGE); }

    inline bool isInnerLoopEntry () const  { return (inner_loop_entry); }
    inline void setInnerLoopEntry (bool _flag) { inner_loop_entry = _flag; }
  
    inline Node* getOuterLoopExited () const { return (_outermost_loop_head_exited); }
    inline void setOuterLoopExited (Node *_lhead) { 
       _outermost_loop_head_exited = _lhead; 
    }
    inline void setNumLevelsExited (int _l) { _levels_exited = _l; }
    inline int getNumLevelsExited () const  { return (_levels_exited); }
  
    inline Node* getOuterLoopEntered () const { return (_outermost_loop_head_entered); }
    inline void setOuterLoopEntered (Node *_lhead) { _outermost_loop_head_entered = _lhead; }
    inline void setNumLevelsEntered (int _l) { _levels_entered = _l; }
    inline int getNumLevelsEntered () const  { return (_levels_entered); }
    
    void setProspectiveEdge(bool _flag)   { is_prospective = _flag; }
    bool isProspectiveEdge() const        { return (is_prospective); }
  
    friend class PrivateCFG;
    
  protected:
    virtual void write_label (std::ostream& os);
    void write_style (std::ostream& os, EdgeFilterType f, bool level_col);

    uint64_t counter;
    uint32_t flags;

    unsigned int _visited;
    
    EdgeType type;
    ObjId id; // 0 is reserved; first instance is 1
    
    bool is_prospective;
    bool inner_loop_entry;
    // next flag is set using the tarjan analysis, though function
    // mark_loop_back_edges_for_interval must be called explicitly after
    // tarjan intervals are computed. Sometime an edge exits multiple levels
    // of a loop nest and we want to know which is the outermost loop it exits.
    // Flag is NULL when analysis is not performed, or if an edge is not
    // an exit edge, otherwise it points to the loop head BB.
    Node* _outermost_loop_head_exited;
    Node* _outermost_loop_head_entered;
    int _levels_exited;
    int _levels_entered;
  };
  //------------------------------------------------------------------------------------------------------------------

  virtual Node* MakeNode(addrtype _start_addr, addrtype _end_addr, NodeType _type = MIAMI_CODE_BLOCK) = 0;
  virtual Edge* MakeEdge(PrivateCFG::Node* n1, PrivateCFG::Node* n2, EdgeType _type = MIAMI_FALLTHRU_EDGE) = 0;
  
  typedef std::list <Node*> NodeList;
  typedef std::list <Edge*> EdgeList;
  typedef std::vector <Node*> NodeArray;
  typedef std::map <Node*, int> NodeIntMap;
  typedef std::set <Node*> NodeSet;
  typedef std::map <unsigned int, NodeSet> Ui2NSMap;
  typedef std::multimap <addrtype, Edge*> AddrEdgeMMap;
  //------------------------------------------------------------------------------------------------------------------
  // Implement a forward and backward instruction iterator
  //-----------------------------------------
  // ForwardInstructionIterator - iterates over instructions in a basic block
  // machine independent interface for iterating over variable length instructions
  class ForwardInstructionIterator : public Iterator {
  public:
    ForwardInstructionIterator (Node* n, addrtype from = -1);
    virtual ~ForwardInstructionIterator () {}
    void operator ++ () 
    { 
       if (i < node->numInstructions())
       {
          pc += node->inst_len[i];
          ++i;
       }
    }
    operator bool () const { return (i < node->numInstructions()); }
    operator int () { return (node->lengthOfInstruction(i)); }
    addrtype Address() const  { return (pc); }
  private:
    Node* node;
    addrtype pc;
    int i;  // position in instruction array
  };

  class BackwardInstructionIterator : public Iterator {
  public:
    BackwardInstructionIterator (Node* n, addrtype from = -1);
    virtual ~BackwardInstructionIterator () {}
    void operator ++ () 
    { 
       if (i >= 0)
          --i;
       if (i >= 0)
          pc -= node->inst_len[i];
    }
    operator bool () const { return (i >= 0); }
    operator int () { return (node->lengthOfInstruction(i)); }
    addrtype Address() const  { return (pc); }
  private:
    Node* node;
    addrtype pc;
    int i;  // position in instruction array
  };
  
  //------------------------------------
  // enumerate incoming edges
  //------------------------------------
  class IncomingEdgesIterator : public DGraph::IncomingEdgesIterator {
  public:
    IncomingEdgesIterator (const Node  *n) : DGraph::IncomingEdgesIterator(n) {}
    virtual ~IncomingEdgesIterator () {}
    Edge* operator-> () const { return dynamic_cast<PrivateCFG::Edge*>(*iter); }
    operator Edge* () const { return dynamic_cast<PrivateCFG::Edge*>(*iter); }
    Edge* Object() const { return dynamic_cast<Edge*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------
  // enumerate outgoing edges
  //------------------------------------
  class OutgoingEdgesIterator : public DGraph::OutgoingEdgesIterator {
  public:
    OutgoingEdgesIterator (const Node  *n) : DGraph::OutgoingEdgesIterator(n) {}
    virtual ~OutgoingEdgesIterator () {}
    Edge* operator-> () const { return dynamic_cast<PrivateCFG::Edge*>(*iter); }
    operator Edge* () const { return dynamic_cast<PrivateCFG::Edge*>(*iter); }
    Edge* Object() const { return dynamic_cast<Edge*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------
  // enumerate nodes in no particular order
  //------------------------------------
  class NodesIterator : public BaseGraph::NodesIterator {
  public:
    NodesIterator (const PrivateCFG &g) : BaseGraph::NodesIterator(g) {}
    virtual ~NodesIterator () {}
    Node* operator-> () const { return dynamic_cast<PrivateCFG::Node*>(*iter); }
    operator Node* () const   { return dynamic_cast<PrivateCFG::Node*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------
  //------------------------------------------------------------------------------------------------------------------
  /** The EdgesIterator is just an extension of BaseGraph::EdgesIterator */
  class EdgesIterator : public BaseGraph::EdgesIterator {
  public:
    EdgesIterator (const PrivateCFG& g) : BaseGraph::EdgesIterator(g) {}
    virtual ~EdgesIterator () {}
    operator Edge* () const { return dynamic_cast<Edge*>(*iter); }
    Edge* operator -> () const { return dynamic_cast<Edge*>(*iter); }
    Edge* Object() const { return dynamic_cast<Edge*>(*iter); }
  };
  //------------------------------------------------------------------------------------------------------------------
  /** The SortedEdgesIterator is just an extension of PrivateCFG::EdgesIterator */
  class SortedEdgesIterator : public BaseGraph::BaseEdgesIterator {
     // lt_Edge: function object that compares Edges.  Useful for sorting.
     class lt_Edge {
     public:
       // return true if e1 < e2; false otherwise
       // we sort first by type, then by ID. Hmm...
       bool operator()(const Edge* e1, const Edge* e2) const {
         EdgeType et1 = e1->getType(), et2 = e2->getType();
         if (et1 != et2)
            return (et1 < et2);
         else {
            return (e1->getId() < e2->getId());
         }
       }
     };
     typedef std::multiset<Edge*, lt_Edge> SortedEdgesSet;
  public:
    SortedEdgesIterator (BaseGraph::BaseEdgesIterator* ei) {
       // insert all edges reached by the input iterator into our local
       // set, sorting them in the process. Then, I just iterate over the
       // sorted set.
       while ((bool)(*ei)) {
          sorted_edges.insert(dynamic_cast<Edge*>((*ei).Object()));
          ++ (*ei);
       }
       iter = sorted_edges.begin();
    }
    virtual ~SortedEdgesIterator () {}
    operator Edge* () const { return dynamic_cast<Edge*>(*iter); }
    Edge* operator -> () const { return dynamic_cast<Edge*>(*iter); }
    operator bool () const { return (iter != sorted_edges.end()); }
    void operator++ () { ++iter; }
  private:
    SortedEdgesSet sorted_edges;
    SortedEdgesSet::iterator iter;
  };
  //------------------------------------------------------------------------------------------------------------------
   
   // define a container to search for the node that contains a particular address
   typedef std::multimap<addrtype, Node*> AddrNodeMap;
   
   // use arguments of type DGraph::Edge* and DGraph::Node*, to avoid the
   // warnings
   virtual void add (DGraph::Edge* e);
   virtual void add (DGraph::Node* n);
   virtual void remove (DGraph::Edge* e);
   virtual void remove (DGraph::Node* n);

   // add a Node, but check also prospective edges and return how
   // many prospective edges were found to point to this node.
   int addNode (Node* n);
   
   Edge* addEdge(Node* n1, Node* n2, EdgeType _type);
   Edge* addUniqueEdge(Node* n1, Node* n2, EdgeType _type);
   Edge* addProspectiveEdge(Node* n1, addrtype sink_addr, EdgeType _type);

   const std::string& name() { return routine_name; }
   
   void dump (std::ostream& os);
   void dump () { dump(std::cout); }

   void draw (std::ostream& os, addrtype offset, const char* name, 
              EdgeFilterType f, bool level_col = false,
              int min_rank = 0, int max_rank = 0);
   void draw (addrtype offset, const char* name, EdgeFilterType f, bool level_col = false,
             int min_rank = 0, int max_rank = 0) 
   { 
      draw(std::cout, offset, name, f, level_col, min_rank, max_rank); 
   }
   void draw_legend (std::ostream& os);
   void draw_CFG(const char* prefix, addrtype offset, int parts = 1);

   Node* CfgEntry()    { return (cfg_entry); }
   Node* CfgExit()     { return (cfg_exit); }
   
   MIAMI::PrivateRoutine* InRoutine() const  { return (in_routine); }

   virtual ObjId HighWaterMarker() const { return nextNodeId; }

   virtual ObjId NumNodes() const { return nextNodeId; }
   virtual ObjId NumEdges() const { return nextEdgeId; }

   void mark_loop_back_edges (RIFGNodeId root, TarjanIntervals *tarj, MiamiRIFG *ecfg,
           AddrEdgeMMap *entryEdges, AddrEdgeMMap *callEntryEdges);
   void set_node_levels(RIFGNodeId node, TarjanIntervals *tarj, 
           MiamiRIFG* mCfg, int level);

   void build_loops_scope_tree(TarjanIntervals *tarj, MiamiRIFG *ecfg);
   void build_loops_scope_tree_recursively(RIFGNodeId node, TarjanIntervals *tarj, unsigned int parent);
   
   unsigned int LoopTreeSize() const  { return (treeSize); }
   unsigned int* LoopTree() const     { return (loopTree); }

   Node* findNodeContainingAddress(addrtype pc);
   
   void ComputeNodeRanks ();
   
   /* this is not the best choice of names, because the graph can still
    * be modified. However, this method ensures the graph is in a good
    * state to perform analysis on it. I need a routine that
    * connects the entry and exit nodes to the rest of the graph.
    * computeTopNodes does this, but it is protected. This method will
    * call computeTopNodes only if the graph has been modified since 
    * previous call. 
    */
   void FinalizeGraph();
   
protected:
   int computeTopNodes ();
   void recursiveNodeRanks (Node* node, int lev, unsigned int m);

   void resetIds() { nextNodeId = 0; nextEdgeId = 0; }
   ObjId getNextNodeId()  { return (nextNodeId++); }
   ObjId getNextEdgeId()  { return (nextEdgeId++); }

   void switchNodeIds (Node *n1, Node *n2)
   {
      unsigned int tempid = n1->id;
      n1->id = n2->id;
      n2->id = tempid;
   }

   MIAMI::PrivateRoutine *in_routine;
   std::string routine_name;
   unsigned int topMarker;
   ObjId nextNodeId;
   ObjId nextEdgeId;
   NodeList topNodes;
   NodeList bottomNodes;
   EdgeMap prospectiveEdges;
   
   AddrNodeMap all_nodes;
   bool setup_node_container;
   
   int maximumGraphRank;
   
   uint32_t cfgFlags;
   Node *cfg_entry, *cfg_exit;
   
   // store the tree relationship of the loop structure, based on their Tarjan
   // indecies. Store it as an array, where an entry points to its parent.
   unsigned int *loopTree;
   unsigned int treeSize;  // the size of the loopTree structure
};
//--------------------------------------------------------------------------------------------------------------------

// a few EdgeFilterType functions, must be updated for a CFG
extern bool draw_all(PrivateCFG::Edge* e);

}  // namespace MIAMI

#endif
