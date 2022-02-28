/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: CycleSetDAG.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data structure to hold a DAG of strongly connected components (SCCs).
 */

#ifndef _CYCLESETDAG_H
#define _CYCLESETDAG_H

//--------------------------------------------------------------------------------------------------------------------
// standard headers
#ifdef NO_STD_CHEADERS
# include <string.h>
#else
# include <cstring>
#endif

// STL headers
#include <list>
#include <set>
#include <map>
#include <queue>
#include <functional>

// OpenAnalysis headers
#include "OAUtils/DGraph.h"
#include "uipair.h"
#include "printable_class.h"
#include "fast_hashmap.h"
#include "miami_types.h"

using MIAMI::ObjId;

namespace MIAMI_DG
{

extern DGraph::Node* defDagNodeP;
typedef MIAMIU::HashMap<ObjId, DGraph::Node*, &defDagNodeP> HashMapDagNodeP;

//--------------------------------------------------------------------------------------------------------------------
/** CycleSetDAG is a DGraph (directed graph) with enhanced nodes and edges.  
    Each node in the CycleSetDAG represents a set of nodes in the scheduling
    graph that are connected by cycle edges, and an edge represents a directed
    connexion between instructions cycle sets. There should not be any cycles
    in this DAG (hence the name), otherwise all nodes that are part of the 
    cycle would be a single node.
*/
class CycleSetDAG : public DGraph {
public:
   class Node;
   friend class Node;
   class Edge;
   friend class Edge;

   CycleSetDAG ();
   virtual ~CycleSetDAG ();

   class Node : public DGraph::Node, public PrintableClass {
   public:
     Node (ObjId _setId) : DGraph::Node()
     { 
        setId = _setId;
        Ctor();
     }
    
     virtual ~Node () {  }
    
     // getId: An id unique within instances of CycleSetDAG::Node
     virtual ObjId getId () const    { return id; }
     // resetIds: reset id numbering
     static void resetIds ()                { nextId = 1; }
     static ObjId HighWaterMarker () { return nextId; }

     void draw (std::ostream& os, int info=0);
     void draw (int info=0) 
     { 
        draw(std::cout, info);
     }
     void PrintObject (ostream& os) const;
    
     void visit (unsigned int m)         { _visited = m; }
     bool visited (unsigned int m) const { return (m==_visited); }
    
     ObjId getCycleSetId ()       { return (setId); }
    
     bool isFixed () const               { return (fixed!=0); }
     int fixOrderId ()                   { return (fixed); }

     friend class CycleSetDAG;

   private:
     void write_label (std::ostream& os, bool html_escape=false);
     void Ctor ()        { id = nextId++; _visited = 0; fixed = 0; }

     static ObjId nextId;
     unsigned int _visited;
     ObjId id; // 0 is reserved; unique id of node
     ObjId setId;
     int fixed;
   };
   //------------------------------------------------------------------------------------------------------------------
   class Edge : public DGraph::Edge, public PrintableClass {
   public:
     Edge (Node* n1, Node* n2);
     ~Edge () {}
    
     // getId: An id unique within instances of CFG::Edge
     virtual ObjId getId () const     { return id; }
     // resetIds: reset id numbering
     static void resetIds ()                 { nextId = 1; }
    
     Node* source () const { return dynamic_cast<Node*> (n1); }
     Node* sink () const   { return dynamic_cast<Node*> (n2); }
    
     void draw (std::ostream& os);
     void draw () 
     { 
        draw(std::cout);
     }
    
     void PrintObject (ostream& os) const;

     friend class CycleSetDAG;
    
   private:
     void write_label (std::ostream& os);
     void write_style (std::ostream& os);

     static ObjId nextId;
     ObjId id; // 0 is reserved; first instance is 1
   };  
   //------------------------------------------------------------------------------------------------------------------
 
   //------------------------------------
   // enumerate incoming edges
   //------------------------------------
   class IncomingEdgesIterator : public DGraph::IncomingEdgesIterator {
   public:
     IncomingEdgesIterator (Node  *n) : DGraph::IncomingEdgesIterator(n) {}
     virtual ~IncomingEdgesIterator () {}
     Edge* operator-> () const { return dynamic_cast<CycleSetDAG::Edge*>(*iter); }
     operator Edge* ()  const { return dynamic_cast<CycleSetDAG::Edge*>(*iter); }
   };
   //------------------------------------------------------------------------------------------------------------------
   //------------------------------------
   // enumerate outgoing edges
   //------------------------------------
   class OutgoingEdgesIterator : public DGraph::OutgoingEdgesIterator {
   public:
     OutgoingEdgesIterator (Node  *n) : DGraph::OutgoingEdgesIterator(n) {}
     virtual ~OutgoingEdgesIterator () {}
     Edge* operator-> () const { return dynamic_cast<CycleSetDAG::Edge*>(*iter); }
     operator Edge* () const  { return dynamic_cast<CycleSetDAG::Edge*>(*iter); }
   };
   //------------------------------------------------------------------------------------------------------------------
   //------------------------------------
   // enumerate nodes in no particular order
   //------------------------------------
   class NodesIterator : public BaseGraph::NodesIterator {
   public:
     NodesIterator (CycleSetDAG &g) : BaseGraph::NodesIterator (g) {}
     virtual ~NodesIterator () {}
     Node* operator-> () const { return dynamic_cast<CycleSetDAG::Node*>(*iter); }
     operator Node* () const  { return dynamic_cast<CycleSetDAG::Node*>(*iter); }
   };
   //------------------------------------------------------------------------------------------------------------------
   /** The EdgesIterator is just an extension of BaseGraph::EdgesIterator */
   class EdgesIterator : public BaseGraph::EdgesIterator {
   public:
     EdgesIterator (CycleSetDAG& g) : BaseGraph::EdgesIterator(g) {}
     virtual ~EdgesIterator () {}
     operator Edge* ()  const  { return dynamic_cast<CycleSetDAG::Edge*>(*iter); }
     Edge* operator -> () const { return dynamic_cast<CycleSetDAG::Edge*>(*iter); }
   };
   //------------------------------------------------------------------------------------------------------------------

   DGraph::Node* addCycleSet (unsigned int _setId);
   Edge* addEdge (Node* n1, Node* n2);
   Edge* addUniqueEdge (Node* n1, Node* n2);
   // I need a set of addEdge methods with unsigned int arguments. Most likely
   // I will pass the setId from the scheduler when creating the edges and I
   // must find the nodes based on the setId.
   Edge* addEdge (unsigned int _s1, unsigned int _s2);
   Edge* addUniqueEdge (unsigned int _s1, unsigned int _s2);
      
   void draw_debug_graphics(const char* prefix);
   void resetSchedule ();   // reset 'fixed' field for all nodes
   int fixSet (unsigned int sId); // return how many sets are fixed
   bool isSetFixed (unsigned int sId);
   int getNumFixedSets () const   { return (numFixedSets); }

   virtual ObjId HighWaterMarker() const { return Node::nextId; }
   
private:
   void draw (std::ostream& os, const char* name);
   void draw (const char* name) 
   { 
      draw(std::cout, name); 
   }

   int fixForwardSets (Node *node);
   int fixBackwardSets (Node *node);

   HashMapDagNodeP dagNodes;
   int lastFixId;
   int numFixedSets;

};

}  /* namespace MIAMI_DG */

#endif
