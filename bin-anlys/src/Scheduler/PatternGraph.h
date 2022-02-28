/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: PatternGraph.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Graph class to hold the left and the right sides of a replacement rule.
 */

#ifndef _PATTERN_GRAPH_H
#define _PATTERN_GRAPH_H

#include <map>
#include "OAUtils/DGraph.h"
#include "uipair.h"
#include "dependency_type.h"
#include "GenericInstruction.h"
#include "charless.h"
#include "position.h"
#include "miami_types.h"
#include "instruction_class.h"

namespace MIAMI
{

#define SOURCE_REG 0x100
#define DESTINATION_REG 0x200
#define TEMPORARY_REG 0x400
#define MULTI_SOURCE_REG 0x900
#define REGTYPE_MASK  0xff

// associate some info with each register name found in the source pattern
class PRegInfo
{
public:
   PRegInfo(DGraph::Node* _node, unsigned int _rType, int _index, const Position& _pos)
   {
      node = _node; regType = _rType; index = _index; pos = _pos;
      count = 1;
   }
   
   DGraph::Node* node;
   unsigned int regType;
   int index;
   int count;
   Position pos;
};

typedef std::map<const char*, PRegInfo, CharLess> PRegMap;


//--------------------------------------------------------------------------------------------------------------------
/** PatternGraph is a DGraph (directed graph) with enhanced nodes and edges.  
    Each node in the PatternGraph represents a machine instruction, and an edge
    represents a dependency between instructions. All nodes in the graph 
    represent an instruction pattern that must be searched in a SchedDG.
    The PatternGraph should be acyclic.
*/
class PatternGraph : public DGraph {
public:
   PatternGraph(GenInstList* instList, const Position& pos, PRegMap& regMap);
   ~PatternGraph();

  class Node;
//  friend class Node;
  class Edge;

  class Node : public DGraph::Node {
  public:
    Node (InstructionClass _iclass) : DGraph::Node() 
    { 
       iclass = _iclass;
       actualN = NULL;
       reqRegCount = 0;
       hasMemory = false;
       exclusiveOutput = false;
       Ctor();
    }
    
    ~Node () {  }
    
    // getId: An id unique within instances of PatternGraph::Node
    virtual ObjId getId() const { return id; }
    // resetIds: reset id numbering
    static void resetIds() { nextId = 1; }

    void visit(unsigned int m) { _visited = m; }
    bool visited(unsigned int m) { return (m==_visited); }
    
//    int getType() const   { return (iclass.type); }
    const InstructionClass& getIClass() const  { return (iclass); }
    
    // have a method to access directly first successor edge
    // for source pattern graphs we should have one successor anyway
    Edge* FirstSuccessor();
    // similar method for the predecessor
    Edge* FirstPredecessor();
    
    void SetActualNode(DGraph::Node* _actualN) { actualN = _actualN; }
    DGraph::Node* ActualNode()  { return (actualN); }
    
    void SetIterationsMoved (unsigned int _iters) { iters_moved = _iters; }
    unsigned int IterationsMoved()  { return (iters_moved); }
    
    // match only patterns where the result of this instruction is used
    // only by instructions matched by the source pattern
    void setExclusiveOutputMatch()     { exclusiveOutput = true; }
    bool ExclusiveOutputMatch() const  { return (exclusiveOutput); }
    bool hasMemoryOperand() const      { return (hasMemory); }
    void setHasMemoryOperand()         { hasMemory = true; }
    
    int requiredRegisterCount() const  { return (reqRegCount); }
    void requireRegisterCount(int cnt) { reqRegCount = cnt; }
    
    friend class PatternGraph;
    
  private:
    void Ctor() { id = nextId++; _visited = 0; }

    static ObjId nextId;

    unsigned int _visited;
    unsigned int id; // 0 is reserved; address of instruction or unique id
    InstructionClass iclass;
    
    unsigned int iters_moved;
    DGraph::Node* actualN;
    
    int reqRegCount;
    
    bool hasMemory;
    bool exclusiveOutput;
  };
  //------------------------------------------------------------------------------------------------------------------
  /* dependencies in a PatternGraph are always intra-iteration (distance is 0) */
  class Edge : public DGraph::Edge {
  public:
    Edge (DGraph::Node* n1, DGraph::Node* n2, int _ddirection, int _dtype );
    ~Edge () {}
    
    int getType() const      { return dtype; }
    int getDirection() const { return ddirection; }
    
    // getId: An id unique within instances of CFG::Edge
    virtual ObjId getId() const { return id; }
    // resetIds: reset id numbering
    static void resetIds() { nextId = 1; }

    Node* source () const { return dynamic_cast<Node*>(n1); }
    Node* sink () const { return dynamic_cast<Node*>(n2); }

    void SetActualEdge(DGraph::Edge* _actualE) { actualE = _actualE; }
    DGraph::Edge* ActualEdge()  { return (actualE); }
    
  private:
    static ObjId nextId;
    
    int ddirection;
    int dtype;
    ObjId id; // 0 is reserved; first instance is 1
    DGraph::Edge* actualE;
  };  
  
  Node* TopNode() { return (cfg_top); }
  virtual ObjId HighWaterMarker() const { return Node::nextId; }
  
private:
  
  Node* cfg_top;   // identify the entry node in the pattern (can have only one)
  
};

}

#endif
