/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder. 
 */
/* 
 * File: CycleSetDAG.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Data structure to hold a DAG of strongly connected components (SCCs).
 */

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

using std::ostream;
using std::endl;
using std::cout;
using std::cerr;

#include "CycleSetDAG.h"
#include "debug_scheduler.h"

using MIAMI::ObjId;

namespace MIAMI_DG
{

ObjId CycleSetDAG::Node::nextId = 1;
ObjId CycleSetDAG::Edge::nextId = 1;
DGraph::Node* defDagNodeP = NULL;

CycleSetDAG::CycleSetDAG ()
{
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (2, 
      fprintf (stderr, "In CycleSetDAG constructor.\n");
   )
#endif
  Node::resetIds();
  Edge::resetIds();
  lastFixId = 0;
  numFixedSets = 0;
}

//--------------------------------------------------------------------------------------------------------------------
CycleSetDAG::~CycleSetDAG()
{
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

   node_set.clear();
   edge_set.clear();
}
//--------------------------------------------------------------------------------------------------------------------

DGraph::Node* 
CycleSetDAG::addCycleSet (unsigned int _setId)
{
   DGraph::Node* &node = dagNodes[_setId];
   if (node == NULL)
   {
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (2, 
         fprintf (stderr, "In CycleSetDAG::addCycleSet for setId %d.\n", _setId);
      )
#endif
      node = new Node (_setId);
      add (node);
   }
   return (node);
}
//--------------------------------------------------------------------------------------------------------------------

CycleSetDAG::Edge* 
CycleSetDAG::addEdge (Node* n1, Node* n2)
{
   CycleSetDAG::Edge* edge = new Edge(n1, n2);
   add (edge);
   return (edge);
}
//--------------------------------------------------------------------------------------------------------------------

CycleSetDAG::Edge*
CycleSetDAG::addUniqueEdge (Node* n1, Node* n2)
{
   OutgoingEdgesIterator oeit(n1);
   while ((bool)oeit)
   {
      if (oeit->sink()==n2)
      {
         return (oeit);
      }
      ++ oeit;
   }
   return (addEdge (n1, n2));
}
//--------------------------------------------------------------------------------------------------------------------

CycleSetDAG::Edge*
CycleSetDAG::addUniqueEdge (unsigned int _s1, unsigned int _s2)
{
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (2,
      cerr << "CycleSetDAG::addUniqueEdge for setIds " << _s1 << " and " 
           << _s2 << endl;
   )
#endif
   Node* n1 = dynamic_cast<Node*>(dagNodes[_s1]);
   assert (n1 != NULL);
   Node* n2 = dynamic_cast<Node*>(dagNodes[_s2]);
   assert (n2 != NULL);
   OutgoingEdgesIterator oeit(n1);
   while ((bool)oeit)
   {
      if (oeit->sink()==n2)
      {
         return (oeit);
      }
      ++ oeit;
   }
   return (addEdge (n1, n2));
}
//--------------------------------------------------------------------------------------------------------------------

CycleSetDAG::Edge*
CycleSetDAG::addEdge (unsigned int _s1, unsigned int _s2)
{
#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (2,
      cerr << "CycleSetDAG::addEdge for setIds " << _s1 << " and " 
           << _s2 << endl;
   )
#endif
   Node* n1 = dynamic_cast<Node*>(dagNodes[_s1]);
   assert (n1 != NULL);
   Node* n2 = dynamic_cast<Node*>(dagNodes[_s2]);
   assert (n2 != NULL);
   Edge* edge = new Edge(n1, n2);
   add(edge);
   return (edge);
}
//--------------------------------------------------------------------------------------------------------------------

void
CycleSetDAG::resetSchedule ()
{
   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *node = nit;
      node->fixed = 0;
      ++ nit;
   }
   lastFixId = 0;
   numFixedSets = 0;
}
//--------------------------------------------------------------------------------------------------------------------

int 
CycleSetDAG::fixSet (unsigned int sId)
{
   int result = 0;
   Node* nn = dynamic_cast<Node*>(dagNodes[sId]);
   assert (nn != NULL);
   assert (nn->fixed == 0);
   nn->fixed = ++ lastFixId;
   ++ result;
   // now do a DFS back and forth and fix all reachable nodes
   result += fixForwardSets (nn);
   result += fixBackwardSets (nn);
   numFixedSets += result;
   return (result);
}
//--------------------------------------------------------------------------------------------------------------------

int
CycleSetDAG::fixForwardSets (Node *node)
{
   int result = 0;
   OutgoingEdgesIterator oit (node);
   while ((bool)oit)
   {
      Node *sinkNode = oit->sink();
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (3,
         cerr << "CycleSetDAG::fixForwardSets folow " << *((Edge*)oit) << endl;
      )
#endif
      if (!sinkNode->fixed)
      {
         sinkNode->fixed = ++ lastFixId;
         ++ result;
         result += fixForwardSets (sinkNode);
      }
      ++ oit;
   }
   return (result);
}
//--------------------------------------------------------------------------------------------------------------------

int 
CycleSetDAG::fixBackwardSets (Node *node)
{
   int result = 0;
   IncomingEdgesIterator iit (node);
   while ((bool)iit)
   {
      Node *srcNode = iit->source();
#if VERBOSE_DEBUG_SCHEDULER
      DEBUG_SCHED (3,
         cerr << "CycleSetDAG::fixBackwardSets folow " << *((Edge*)iit) << endl;
      )
#endif
      if (!srcNode->fixed)
      {
         srcNode->fixed = ++ lastFixId;
         ++ result;
         result += fixBackwardSets (srcNode);
      }
      ++ iit;
   }
   return (result);
}
//--------------------------------------------------------------------------------------------------------------------

bool 
CycleSetDAG::isSetFixed (unsigned int sId)
{
//   fprintf (stderr, "CycleSetDAG::isSetFixed for sId=%u\n", sId);
   Node* n1 = dynamic_cast<Node*>(dagNodes[sId]);
   assert (n1 != NULL);
   return (n1->fixed != 0);
}
//--------------------------------------------------------------------------------------------------------------------

CycleSetDAG::Edge::Edge (Node* n1, Node* n2)
  : DGraph::Edge (n1, n2)
{
  id = nextId++;
}
//--------------------------------------------------------------------------------------------------------------------

void
CycleSetDAG::Edge::PrintObject (ostream& os) const
{
   os << "Edge " << id << " with source {" << *(source()) 
      << "} and sink {" << *(sink()) << "}.";
//   os << endl;
}
//--------------------------------------------------------------------------------------------------------------------

void
CycleSetDAG::Node::PrintObject (ostream& os) const
{
   os << "Node " << id << " for setId " << setId << " fixed=" 
      << fixed << ".";   //endl;
}
//--------------------------------------------------------------------------------------------------------------------

void 
CycleSetDAG::draw_debug_graphics(const char* prefix)
{
   char file_name[64];
   char file_name_ps[64];
   char title_draw[64];
   
   sprintf(file_name, "%s_cyc_dag.dot", prefix);
   sprintf(file_name_ps, "%s_cyc_dag.ps", prefix);
   sprintf(title_draw, "%s_cyc_dag", prefix);

   ofstream fout(file_name, ios::out);
   assert(fout && fout.good());
   draw(fout, title_draw);
   fout.close();
   assert(fout.good());

   stringstream dot_cmd;
   dot_cmd << "dot -Tps2 -o " << file_name_ps << " "
           << file_name << '\0';
//   system(dot_cmd.str());
}
//--------------------------------------------------------------------------------------------------------------------

void
CycleSetDAG::draw (ostream& os, const char* name)
{
   os << "digraph \"" << name << "\"{\n";
   os << "size=\"7.5,10\";\n";
   os << "center=\"true\";\n";
   os << "ratio=\"compress\";\n";
   os << "ranksep=.3;\n";
   os << "node[color=black,fontcolor=black,shape=ellipse,font=Helvetica,fontsize=14,height=.3];\n";
   os << "edge[font=Helvetica,fontsize=14,dir=forward];\n";

//   os << "N0[fontsize=20,width=.5,height=.25,shape=plaintext,"
//      << "label=\"" << name << "\"];\n";

   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      nn->draw(os);
      ++ nit;
   }
   
   EdgesIterator eit(*this);
   while ((bool)eit)
   {
      Edge *ee = eit;
      ee->draw(os);
      ++ eit;
   }
   
   os << "}\n";
}

void
CycleSetDAG::Node::draw(ostream& os, int info)
{
   os << "B" << id << "[";
   os << "label=\"";
   write_label(os);
   os << "\",shape=ellipse];\n";
}

void
CycleSetDAG::Node::write_label(ostream& os, bool html_escape)
{
   os << id << "::SetId " << setId;
}

void
CycleSetDAG::Edge::draw(ostream& os)
{
   os << "B" << source()->getId() << "->B" << sink()->getId() << "[";
   write_label (os);
   write_style (os);
   os << "];\n";
}

void
CycleSetDAG::Edge::write_label (ostream& os)
{
   os << "label=\"E" << id << "\",";
}

void
CycleSetDAG::Edge::write_style (ostream& os)
{
   os << "style=\"setlinewidth(2),solid\"";
}

}  /* namespace MIAMI_DG */
