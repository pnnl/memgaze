/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: PrivateCFG.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Base class for control flow graph support in MIAMI.
 */

#include <math.h>

// standard headers

#ifdef NO_STD_CHEADERS
# include <stdlib.h>
# include <string.h>
# include <assert.h>
# include <stdarg.h>
#else
# include <cstdlib>
# include <cstring>
# include <cassert>
using namespace std; // For compatibility with non-std C headers
#endif

#include <iostream>
#include <fstream>
#include <iomanip>

using std::ostream;
using std::endl;
using std::cout;
using std::cerr;
using std::setw;

#include "PrivateCFG.h"
#include "private_routine.h"
#include "file_utilities.h"
#include "instruction_decoding.h"
#include "tarjans/MiamiRIFG.h"

#define DEBUG_BBLs 0

#if DEBUG_BBLs   
static const char *debugName = "dl_iterate_phdr";
#endif

using namespace MIAMI;

//--------------------------------------------------------------------------------------------------------------------

bool 
MIAMI::draw_all(PrivateCFG::Edge* e)
{
   return (true);
}

//--------------------------------------------------------------------------------------------------------------------
PrivateCFG::PrivateCFG (PrivateRoutine* _r, const std::string& func_name)
{
   in_routine = _r;
   routine_name = func_name;

#if VERBOSE_DEBUG_SCHEDULER
   DEBUG_SCHED (1, 
      fprintf (stderr, "In PrivateCFG constructor for routine %s\n", routine_name.c_str());
   )
#endif
  topMarker = 0;
  nextNodeId = 0;
  nextEdgeId = 0;

  loopTree = 0;
  treeSize = 0;

  cfgFlags = 0;
  maximumGraphRank = 0;
  setup_node_container = false;
}

//--------------------------------------------------------------------------------------------------------------------
PrivateCFG::~PrivateCFG()
{
//   fprintf (stderr, "PrivateCFG::DESTRUCTOR called\n");
      
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
   
   if (loopTree)
      free(loopTree);
   loopTree = 0;
   treeSize = 0;

   cfgFlags = 0;
}

//--------------------------------------------------------------------------------------------------------------------
void 
PrivateCFG::add (DGraph::Edge* e)
{
   setCfgFlags(CFG_GRAPH_IS_MODIFIED);
   DGraph::add(e);
}
//--------------------------------------------------------------------------------------------------------------------

#define MAX_INCOMING_EDGES 32

void 
PrivateCFG::add (DGraph::Node* n)
{
   setCfgFlags(CFG_GRAPH_IS_MODIFIED);
   DGraph::add(n);
}

//--------------------------------------------------------------------------------------------------------------------
void 
PrivateCFG::remove (DGraph::Edge* e)
{
   setCfgFlags(CFG_GRAPH_IS_MODIFIED);
   DGraph::remove(e);
}

//--------------------------------------------------------------------------------------------------------------------
void 
PrivateCFG::remove (DGraph::Node* n)
{
   setCfgFlags(CFG_GRAPH_IS_MODIFIED);
   DGraph::remove(n);
}
//--------------------------------------------------------------------------------------------------------------------

PrivateCFG::Edge* 
PrivateCFG::addEdge(PrivateCFG::Node* n1, PrivateCFG::Node* n2, EdgeType _type)
{
#if DEBUG_CFG_DISCOVERY
   LOG_CFG(3,
      if (n1->getStartAddress()>=0x399fc15095 && n1->getEndAddress()<=0x399fc150aa)
         fprintf(stderr, "AddEdge: from block [0x%" PRIxaddr ",0x%" PRIxaddr "] to target [0x%" PRIxaddr 
                ",0x%" PRIxaddr "] of type=%d\n",
                n1->getStartAddress(), n1->getEndAddress(), 
                n2->getStartAddress(), n2->getEndAddress(), _type);
   )
#endif
   Edge* edge = MakeEdge(static_cast<Node*>(n1), static_cast<Node*>(n2), _type);
   add(edge);
   return (edge);
}
//--------------------------------------------------------------------------------------------------------------------

PrivateCFG::Edge*
PrivateCFG::addUniqueEdge(PrivateCFG::Node* n1, PrivateCFG::Node* n2, EdgeType _type)
{
   OutgoingEdgesIterator oeit(n1);
   while ((bool)oeit)
   {
      if (oeit->sink()==n2 && oeit->getType()==_type)
      {
         return (oeit);
      }
      ++ oeit;
   }
   return (addEdge(n1, n2, _type));
}
//--------------------------------------------------------------------------------------------------------------------

PrivateCFG::Edge*
PrivateCFG::addProspectiveEdge(Node* n1, addrtype sink_addr, EdgeType _type)
{
   // check for duplicates by looking at all prospective edges that want to
   // go to sink_addr. Then check that their source node and _type is the same
   std::pair <EdgeMap::iterator, EdgeMap::iterator> _edges = 
             prospectiveEdges.equal_range(sink_addr);
   EdgeMap::iterator eit;
   // I should avoid adding duplicates, keep track of what edges I add
   for (eit=_edges.first ; eit!=_edges.second ; )
   {
      Edge *e = eit->second;
      if (n1==e->source() && _type==e->type)  // duplicate
      {
         // do not add it again, return what we have
         return (e);  // edge already exists
      }
      ++eit;
   }
   Edge* e2 = addEdge(n1, static_cast<Node*>(cfg_exit), _type);
   e2->setProspectiveEdge(true);
   prospectiveEdges.insert(EdgeMap::value_type(sink_addr, e2));
   return (e2);
}
//--------------------------------------------------------------------------------------------------------------------

PrivateCFG::Node*
PrivateCFG::Node::SplitAt(addrtype split_addr, int *incEdges, NodeType  _type)
{
   // test that address is in bounds first
   if (split_addr<start_addr || split_addr>=end_addr)
   {
      fprintf(stderr, "ERROR: Request to split node [0x%" PRIxaddr ",0x%" PRIxaddr "] at outside address 0x%" PRIxaddr "\n", 
                  start_addr, end_addr, split_addr);
      return (NULL);
   }
   // create a new node from split point to end of current node
   Node *newn = _in_cfg->MakeNode(split_addr, end_addr, _type);
   *incEdges = _in_cfg->addNode(newn);
   end_addr = split_addr;
   // change each outgoing edge of the old node to start from
   // the newly created node
   OutgoingEdgesIterator oeit(this);
   while ((bool)oeit) {
      Edge* e = oeit;
      // increment iterator before we remove the edge
      ++oeit;
      
      // remove this edge from the list of outgoing edges of the source node
      outgoing_edges.remove(e);
      newn->outgoing_edges.push_back(e);
      e->ChangeSource(newn);
  }
  
   _in_cfg->addEdge(this, newn, MIAMI_FALLTHRU_EDGE);
   return (newn);
}
//--------------------------------------------------------------------------------------------------------------------

#define MAX_INCOMING_EDGES 32

/* Return how many prospective edges have been found for this node.
 * If we found prospective edges, we must have came from within the routine.
 * Otherwise, this node is likely a routine entry, so mark it as such.
 */
int 
PrivateCFG::addNode (Node* n)
{
   setCfgFlags(CFG_GRAPH_IS_MODIFIED);
   DGraph::add(n);
   // when we add a node, check if we have any prospective incoming edges for it
   // this makes sense only if this is not a call-surrogate block
   int nedges = 0;
#if DEBUG_BBLs
   string name = InRoutine()->Name();
#endif
   if (n->type!=MIAMI_CALL_SITE && !prospectiveEdges.empty())
   {
      std::pair <EdgeMap::iterator, EdgeMap::iterator> _edges = 
               prospectiveEdges.equal_range(n->start_addr);
      EdgeMap::iterator eit;
      // I should avoid adding duplicates, keep track of what edges I add
      Node* srcs[MAX_INCOMING_EDGES];
      EdgeType etypes[MAX_INCOMING_EDGES];
      for (eit=_edges.first ; eit!=_edges.second ; )
      {
         EdgeMap::iterator tmp = eit;
         Edge *e = tmp->second;
         bool duplicate = false;
         for (int i=0 ; i<nedges ; ++i)
            if (srcs[i]==e->source() && etypes[i]==e->type)  // duplicate
            {
               duplicate = true;
               remove(e);
               delete e;
               break;
            }
         if (!duplicate)
         {
            e->sink()->incoming_edges.remove(e);
            n->incoming_edges.push_back(e);
            e->ChangeSink(n);
            if (nedges<MAX_INCOMING_EDGES) {
               srcs[nedges] = e->source();
               etypes[nedges++] = e->type;
            }
            e->setProspectiveEdge(false);
#if DEBUG_BBLs
            if (name.compare(debugName)==0)
            {
               fprintf(stderr, " - addNode: in routine %s, adding block [0x%" PRIxaddr ",0x%" PRIxaddr
                      "] found prospective edge from block [0x%" PRIxaddr ",0x%" PRIxaddr "] of type %d\n",
                      name.c_str(), n->getStartAddress(), n->getEndAddress(),
                      e->source()->getStartAddress(), e->source()->getEndAddress(),
                      e->type);
            }
#endif
         }
         
         ++eit;
         prospectiveEdges.erase(tmp);
      }
   }
   return (nedges);
}

//--------------------------------------------------------------------------------------------------------------------

PrivateCFG::Edge::Edge (PrivateCFG::Node* n1, PrivateCFG::Node* n2, EdgeType _type)
  : DGraph::Edge(n1, n2), type(_type)
{
  assert (n1);
  PrivateCFG *gg = n1->inCfg();
  if (gg)
     id = gg->getNextEdgeId();
  else
     id = 0;
  flags = 0;
  _visited = 0;
  counter = 0;
  
  is_prospective = false;
  inner_loop_entry = false;
  _outermost_loop_head_exited = 0;
  _outermost_loop_head_entered = 0; 
  _levels_exited = 0;
  _levels_entered = 0;
}
//--------------------------------------------------------------------------------------------------------------------

void PrivateCFG::Edge::dump (ostream& os)
{
  os << "{ " << id << " " 
     << edgeTypeToString(type) << " " << " }";
  os.flush();
}
//--------------------------------------------------------------------------------------------------------------------


void
PrivateCFG::dump (ostream& os)
{
  os << "===== PrivateCFG dump =====" << endl << "    Routine: " << routine_name << endl << "--------------------" << endl;
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

/* this is not the best choice of names, because the graph can still
 * be modified. However, this method ensures the graph is in a good
 * state to perform analysis on it. I need a routine that
 * connects the entry and exit nodes to the rest of the graph.
 * computeTopNodes does this, but it is protected. This method will
 * call computeTopNodes only if the graph has been modified since 
 * previous call. 
 */
void 
PrivateCFG::FinalizeGraph()
{
   if (HasAllFlags (CFG_GRAPH_IS_MODIFIED))
   {
      computeTopNodes();
   }
}
//--------------------------------------------------------------------------------------------------------------------

void 
PrivateCFG::ComputeNodeRanks()
{
   if (HasAllFlags (CFG_GRAPH_IS_MODIFIED))
   {
      computeTopNodes();
   }
   
   // from each top node perform a DFS on loop independent edges and
   // compute a level for each node
   OutgoingEdgesIterator teit(cfg_entry);
   // back edges are not marked, so I need to detect cyles as well
   // use two different markers

   // reserve a second value, mm-1 and mm will be used
   // compute also the maximum graph level
   unsigned int mm = new_marker(2);
   maximumGraphRank = 0;
   while ((bool)teit)
   {
      if (!teit->isLoopBackEdge())
      {
         Node *nn = teit->sink();
         recursiveNodeRanks(nn, 1, mm);
      }
      ++ teit;
   }
   cfg_exit->rank = maximumGraphRank+1;
   
   removeCfgFlags(CFG_GRAPH_IS_MODIFIED);
}
//--------------------------------------------------------------------------------------------------------------------

void
PrivateCFG::recursiveNodeRanks(Node* node, int lev, unsigned int m)
{
   // if this is first time I see this node, assign a level to it
   if (node->visited(m-1))  // I am in the process of processing this node
   {
      // hmm, this is a back edge. I can add the test in the loop iterating over 
      // outgoing edges and mark the egde as a back edge as well
      assert(!"I should not be here");
      return;
   }
   if (! node->visited(m) )
   {
      node->rank = lev;
   } else   // otherwise test if this is a deeper level
   {
      // if we find that the node is on a deeper level than we thought before
      // then continue recursively because its children might be on deeper 
      // levels too.
      // Otherwise stop recursion at this node, because it cannot change
      // anything downstream.
      if (node->rank < lev)
         node->rank = lev;
      else
         return;
   }
   node->visit(m-1);

   // we continue only if lev is the deepest level found for this node,
   // therefore, I must go recursive with lev+1
   int i = 0;
   OutgoingEdgesIterator oeit(node);
   while ((bool)oeit)
   {
      Edge *ee = oeit;
      if (!ee->isLoopBackEdge()) // && ee->sink()!=cfg_exit)
      {
         // test if we visited the sink node before. 
         // It may happen if we did not compute Tarjan intervals and 
         // we did not mark back edges yet.
         if (! ee->sink()->visited(m-1))  // it is a back edge
         {
            ++ i;
            recursiveNodeRanks (ee->sink(), lev+1, m);
         }
      }
      ++ oeit;
   }
   // maximum graph level can belong only to a leaf node; if I am here, 
   // it means that this node's level is lev
   if (node!=cfg_exit && lev>maximumGraphRank)
      maximumGraphRank = lev;
   node->visit(m);
}
//--------------------------------------------------------------------------------------------------------------------

addrtype 
PrivateCFG::Node::RelocationOffset() const
{
   if (_in_cfg)
      return (_in_cfg->InRoutine()->InLoadModule()->RelocationOffset());
   else
      return (0);
}
//--------------------------------------------------------------------------------------------------------------------

PrivateCFG::Node*
PrivateCFG::Edge::ChangeSource(PrivateCFG::Node *src)
{
   Node *oldN = static_cast<Node*>(n1);
   n1 = src;
   return (oldN);
}
//--------------------------------------------------------------------------------------------------------------------

PrivateCFG::Node*
PrivateCFG::Edge::ChangeSink(PrivateCFG::Node *sink)
{
   Node *oldN = static_cast<Node*>(n2);
   n2 = sink;
   return (oldN);
}
//--------------------------------------------------------------------------------------------------------------------

int
PrivateCFG::computeTopNodes()
{
#if 0
   bool keepTopNodes = false;
   // if we have the routine entry points, use those nodes as top nodes
   // do not recompute them based on the incoming edges
   if (HasSomeFlags(CFG_HAS_ENTRY_POINTS))
      keepTopNodes = true;
      
   if (!keepTopNodes)
   {
      // delete the old top_nodes
      OutgoingEdgesIterator oeit(cfg_entry);
      while ((bool)oeit)
      {
         Edge* tempE = oeit;
         ++ oeit;
         remove (tempE);
         delete (tempE);
      }
      topNodes.clear();
   }

   // delete the old bottom_nodes
   IncomingEdgesIterator ieit(cfg_exit);
   while ((bool)ieit)
   {
      Edge* tempE = ieit;
      ++ ieit;
      if (tempE->type!=MIAMI_RETURN_EDGE && tempE->type!=MIAMI_FALLTHRU_EDGE)
      {
         remove (tempE);
         delete (tempE);
      }
   }
   bottomNodes.clear();
#endif
   NodeList newTops, newBottoms;
   addrtype rstart = InRoutine()->Start();  // routine start address
   Node *firstNode = 0;
   
   // identify new top nodes and bottom nodes if the graph has changed
//   unsigned int m = new_marker ();
   NodesIterator nit(*this);
   while ((bool)nit) 
   {
      Node *nn = nit;
      if (nn->isCodeNode())
      {
         bool is_top = true, is_bottom = true;
         if (nn->getStartAddress() == rstart)
            firstNode = nn;
         
         IncomingEdgesIterator lieit(nn);
         while ((bool)lieit)
         {
            if (!lieit->isLoopBackEdge())
            {
               is_top = false;
               break;
            }
            ++ lieit;
         }
         
         OutgoingEdgesIterator loeit(nn);
         while ((bool)loeit)
         {
            if (!loeit->isLoopBackEdge())
            {
               is_bottom = false;
               break;
            }
            ++ loeit;
         }

         if (is_top)
            newTops.push_back(nn);
         if (is_bottom)
            newBottoms.push_back(nn);
      }
      ++ nit;
   }

   // if we have no top nodes, add the first node to the list
   if (newTops.empty() && firstNode)
      newTops.push_back(firstNode);
   // create edges from entry to top nodes and from bottom nodes to exit
   NodeList::iterator lit;
   for (lit=newTops.begin() ; lit!=newTops.end() ; ++lit)
   {
      // create also a special type edge
      addUniqueEdge(cfg_entry, *lit, MIAMI_CFG_EDGE);
      topNodes.push_back(*lit);
   }
   
   for (lit=newBottoms.begin() ; lit!=newBottoms.end() ; ++lit)
   {
      // create a special type edge
      addUniqueEdge(*lit, cfg_exit, MIAMI_CFG_EDGE);
      bottomNodes.push_back(*lit);
   }
   return (topNodes.size());
}
//--------------------------------------------------------------------------------------------------------------------

void
PrivateCFG::Edge::PrintObject (ostream& os) const
{
   os << "Edge " << id << " of type " << edgeTypeToString(type) 
      << " with source {" << *(source()) 
      << "} and sink {" << *(sink()) << "}.";
}
//--------------------------------------------------------------------------------------------------------------------

void
PrivateCFG::Node::PrintObject (ostream& os) const
{
   os << "Node " << id << " of type " << nodeTypeToString(type)
      << ".";   //endl;
}
//--------------------------------------------------------------------------------------------------------------------

void
PrivateCFG::set_node_levels(RIFGNodeId node, TarjanIntervals *tarj, 
             MiamiRIFG* mCfg, int level)
{
   Node* b;
   int kid;
   
   b = static_cast<Node*>(mCfg->GetRIFGNode(node));
   b->setLevel(tarj->GetLevel(node));
      
   for (kid = tarj->TarjInners(node) ; kid != RIFG_NIL ; 
                kid = tarj->TarjNext(kid))
   {
      b = static_cast<Node*>(mCfg->GetRIFGNode(kid));

#if DEBUG_GRAPH_CONSTRUCTION
      DEBUG_GRAPH(3,
         if (tarj->getNodeType(kid) == TarjanIntervals::NODE_IRREDUCIBLE)
         {
            fprintf (stderr, "==> found an IRREDUCIBLE node, block 0x%p\n", 
                       (void*)(b->getStartAddress()));
         }
      )
#endif
      
      if (tarj->NodeIsLoopHeader(kid))
         set_node_levels(kid, tarj, mCfg, level+1);
      else
         b->setLevel(tarj->GetLevel(kid));
   }
}
//--------------------------------------------------------------------------------------------------------------------

void 
PrivateCFG::build_loops_scope_tree(TarjanIntervals *tarj, MiamiRIFG* mCfg)
{
   if (loopTree)
      free(loopTree);
   // I do not know how many loops I have in this routine.
   // I am going to make the array of a fixed size, and dyanmically increase 
   // it as needed. Should I start with a function of the routine size, or 
   // just a fixed constant?
   treeSize = NumNodes()/5;
   if (treeSize<2) treeSize = 2;
   loopTree = (unsigned int*)malloc(treeSize*sizeof(unsigned int));
   for(unsigned int i=0 ; i<treeSize ; ++i)
      loopTree[i] = 0;
   RIFGNodeId root = mCfg->GetRootNode();
   build_loops_scope_tree_recursively(root, tarj, 0);
}
//--------------------------------------------------------------------------------------------------------------------

void 
PrivateCFG::build_loops_scope_tree_recursively(RIFGNodeId node, TarjanIntervals *tarj, 
               unsigned int parent)
{
   int kid;
   
   unsigned int loopIndex = tarj->LoopIndex(node);
   if (loopIndex >= treeSize)  // I need to grow my loop tree array
   {
      assert(loopIndex <= (unsigned int)NumNodes());  // I should have fewer loops than graph nodes
      unsigned int oldSize = treeSize;
      // double, or increase by another integer factor.
      while (treeSize<=loopIndex) treeSize += oldSize;
      loopTree = (unsigned int*)realloc(loopTree, treeSize * sizeof(unsigned int));
      for (unsigned int i=oldSize ; i<treeSize ; ++i)
         loopTree[i] = 0;
   }
   loopTree[loopIndex] = parent;
      
   // Iterate over the nodes and recursively invoke the routine for 
   // interval nodes
   for (kid = tarj->TarjInners(node) ; kid != RIFG_NIL ; 
                kid = tarj->TarjNext(kid))
   {
      if (tarj->NodeIsLoopHeader(kid))
      {
         build_loops_scope_tree_recursively(kid, tarj, loopIndex);
      }
   }
}
//--------------------------------------------------------------------------------------------------------------------

void 
PrivateCFG::draw_CFG(const char* prefix, addrtype offset, int parts)
{
   char file_name[64];
   char title_draw[64];
   
   // I have to compute levels for nodes
   if (HasSomeFlags(CFG_GRAPH_IS_MODIFIED))
      ComputeNodeRanks();

   if (parts < 1) parts = 1;
   int numRanks = maximumGraphRank+2;
   
   // divide the range of ranks into close to equal ranges
   int partSize = (numRanks+parts-1) / parts;
   
   int min_rank = 0, p = 0;
   while (min_rank < numRanks)
   {
      int max_rank = min_rank + partSize;
      
      if (parts == 1)
         sprintf(title_draw, "%.40s_cfg", prefix);
      else
         sprintf(title_draw, "%.40s_p%02d_cfg", prefix, p);
      sprintf(file_name, "%s.dot", title_draw);
      int idx=0;
      // do not overwrite an existing file, create a new version
      while (fileExists(file_name))
         sprintf(file_name, "%s_%d.dot", title_draw, ++idx);
         
      ofstream fout(file_name, ios::out);
      assert(fout && fout.good());
      draw(fout, offset, title_draw, draw_all, false, min_rank, max_rank);
      fout.close();
      assert(fout.good());

      if (parts == 1)
         sprintf(title_draw, "%.40s_tarj", prefix);
      else
         sprintf(title_draw, "%.40s_p%02d_tarj", prefix, p);
      sprintf(file_name, "%s.dot", title_draw);
      idx=0;
      // do not overwrite an existing file, create a new version
      while (fileExists(file_name))
         sprintf(file_name, "%s_%d.dot", title_draw, ++idx);

      ofstream fout2(file_name, ios::out);
      assert(fout2 && fout2.good());
      draw(fout2, offset, title_draw, draw_all, true, min_rank, max_rank);
      fout2.close();
      assert(fout2.good());
      
      ++ p;
      min_rank = max_rank;
   }
/*
   strstream dot_cmd;
   dot_cmd << "dot -Tps2 -o " << file_name_ps << " "
           << file_name << '\0';
   system(dot_cmd.str());
   delete dot_cmd.str();
*/
}

static const char* nodeLevelColor[] = {
  "blue",
  "red",
  "darkgreen",
  "magenta",
  "black",
  "yellow",
  "blue",
  "red",
  "darkgreen",
  "magenta",
  "black",
  "yellow",
  "blue",
  "red",
  "darkgreen",
  "magenta",
  "black",
  "yellow"
};

void
PrivateCFG::draw (ostream& os, addrtype offset, const char* name, EdgeFilterType f, bool level_col,
         int min_rank, int max_rank)
{
   // I have to compute levels for nodes
   if (HasSomeFlags(CFG_GRAPH_IS_MODIFIED))
      ComputeNodeRanks();

   os << "digraph \"" << name << "\"{\n";
   os << "size=\"7.5,10\";\n";
   os << "center=\"true\";\n";
   os << "ratio=\"expand\";\n"; //compress\";\n";
   os << "ranksep=.3;\n";
   
//   draw_legend(os);
   
   os << "node[color=black,fontcolor=black,shape=ellipse,font=Helvetica,fontsize=14,height=.3,penwidth=3];\n";
   os << "edge[font=Helvetica,fontsize=14,dir=forward,penwidth=3];\n";

//   os << "N0[fontsize=20,width=.5,height=.25,shape=plaintext,"
//      << "label=\"" << name << "\"];\n";

   NodesIterator nit(*this);
   while ((bool)nit)
   {
      Node *nn = nit;
      if (nn->isCodeNode() && ((nn->getRank()>=min_rank && nn->getRank()<max_rank) ||
                 min_rank==max_rank))
      {
         nn->drawNode (os, offset, level_col);
      }
      ++ nit;
   }
   
   EdgesIterator egit(*this);
   while ((bool)egit)
   {
      Edge *ee = egit;
      int src_rank = ee->source()->getRank(),
          sink_rank = ee->sink()->getRank();
      // if at least one of the edge's ends is in the correct range
      if ((min_rank<=src_rank && src_rank<max_rank) ||
          (min_rank<=sink_rank && sink_rank<max_rank) ||
          min_rank==max_rank )
         ee->drawEdge(os, f, level_col);
      ++ egit;
   }
   
   os << "}\n";
}
//--------------------------------------------------------------------------------------------------------------------

void
PrivateCFG::Node::drawNode(ostream& os, addrtype offset, bool level_col, int info)
{
   os << "B" << id << "[level=" << _level << ","
      << "color=" << (level_col?nodeLevelColor[_level]:nodeTypeColor(type))
      << "," << "label=\"";
   write_label(os, offset, info);
   os << "\",shape=ellipse];\n";
}

void
PrivateCFG::Node::write_label(ostream& os, addrtype offset, int info, bool html_escape)
{
//   os << hex << address << dec << ":"
   os << "B" << id;
   if (info)
      os << "(" << info << ")";
   if (type==MIAMI_CALL_SITE)
      os << " [0x" << hex << start_addr-offset << ": call 0x" << target << "]" << dec;
   else
      os << " [0x" << hex << start_addr-offset << ",0x" << end_addr-offset << "]" << dec;
}

void
PrivateCFG::Edge::drawEdge(ostream& os, EdgeFilterType f, bool level_col)
{
   if (source()->getRank()<sink()->getRank())
   {
      os << "B" << source()->getId() << "->B" << sink()->getId() << "[";
      if (isCounterEdge())
         os << "dir=both,arrowtail=box,";
   }
   else
   {
      os << "B" << sink()->getId() << "->B" << source()->getId();
      if (isCounterEdge())
         os << "[dir=both,arrowhead=box,";
      else
         os << "[dir=back,";
   }
   write_label (os);
   write_style (os, f, level_col);
   os << "];\n";
}

void
PrivateCFG::Edge::write_label (ostream& os)
{
   os << "label=\"E" << id;
   if (isCounterEdge())
      os << "\\nM=" << counter;
   os << "\",";
}

void
PrivateCFG::Edge::write_style (ostream& os, EdgeFilterType f, bool level_col)
{
   os << "style=";
   if (f(this))  // edge was not pruned
      os << edgeTypeStyle(type);
   else
      os << "invis";
   os << ",color=\"";
   if (isLoopBackEdge()) os << "yellow:";
   os << (level_col?nodeLevelColor[min(sink()->_level,source()->_level)]:edgeTypeColor(type));
//   if (isCounterEdge()) os << ":purple";
   os << "\",fontcolor=" << edgeTypeColor(type);
}


void
PrivateCFG::draw_legend (ostream& os)
{
//  os << "digraph \"Scheduling Graph Legend\"{\n";
  os << "subgraph legend{\n";
  os << "node[width=.5,height=.25,shape=plaintext,style=\"setlinewidth(2)\",fontsize=14,font=Helvetica];\n";
  os << "N0[fontcolor=black,fontsize=20,label=\"Legend\"];\n";
  os << "N1[color=black,fontcolor=black,fontsize=16,label=\"Color Map for Edge Type\"];\n";
  os << "N2[color=black,fontcolor=black,shape=ellipse,label=\"Basic Block\"];\n";
  for (int i=0 ; i<MIAMI_NUM_EDGE_TYPES ; ++i)
  {
     os << "EN2_" << i << "[height=.1,width=.1,style=invis];\n";
  }

  os << "N0->N1->N2->EN2_" << MIAMI_NUM_EDGE_TYPES/2 << "[style=invis];\n";
  os << "edge[style=\"setlinewidth(2)\"];\n";
  for (int i=0 ; i<MIAMI_NUM_EDGE_TYPES ; ++i)
  {
     os << "N2->EN2_" << i << "[style=" << Edge::edgeTypeStyle((EdgeType)i)
         << ",color=" << Edge::edgeTypeColor((EdgeType)i)
         << ",fontcolor=" << Edge::edgeTypeColor((EdgeType)i)
         << ",label=\"" << Edge::edgeTypeToString((EdgeType)i) << "\"];\n";
  }
  os << "};\n";
//  os << "}\n";
}

//--------------------------------------------------------------------------------------------------------------------
void
PrivateCFG::Node::longdump (PrivateCFG *currcfg, ostream& os)
{
  // print the node ID
  os << "NodeId: " << getId() << ", start_addr=0x" << hex << start_addr
     << ", end_addr=0x" << end_addr << dec 
     << ", type=" << type << endl;
     
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
PrivateCFG::Node::isCodeNode() const
{
   switch (type)
   {
      case MIAMI_CODE_BLOCK:
      case MIAMI_CALL_SITE:
      case MIAMI_REP_BLOCK:
         return true;
         
      case MIAMI_DATA_BLOCK:
      case MIAMI_CFG_BLOCK:
         return false;
         
      default:
         return false;
   }
   return (false);
}

const char* 
PrivateCFG::Node::nodeTypeToString(NodeType tt)
{
   switch (tt)
   {
      case ANY_BLOCK_TYPE:
         return "AnyBlockType";
         
      case MIAMI_CODE_BLOCK:
         return "CodeBlock";
         
      case MIAMI_CALL_SITE:
         return "CallSite";
         
      case MIAMI_REP_BLOCK:
         return "RepBlock";
         
      case MIAMI_DATA_BLOCK:
         return "DataBlock";
         
      case MIAMI_CFG_BLOCK:
         return "CfgBlock";
         
      default:
         assert(! "Invalid node type.");
   }
   return (NULL);
}
//--------------------------------------------------------------------------------------------------------------------

const char* 
PrivateCFG::Node::nodeTypeColor(NodeType tt)
{
   switch (tt)
   {
      case MIAMI_CODE_BLOCK:
         return "blue";
         
      case MIAMI_CALL_SITE:
         return "yellow";
         
      case MIAMI_REP_BLOCK:
         return "darkgreen";
         
      default:
         return "black";
   }
   return (NULL);
}
//--------------------------------------------------------------------------------------------------------------------

const char* 
PrivateCFG::Edge::edgeTypeToString(EdgeType tt)
{
   switch (tt)
   {
      case ANY_EDGE_TYPE:
         return "AnyEdgeType";
         
      case MIAMI_FALLTHRU_EDGE:
         return "FallThroughEdge";
         
      case MIAMI_DIRECT_BRANCH_EDGE:
         return "DirectBranchEdge";
         
      case MIAMI_INDIRECT_BRANCH_EDGE:
         return "IndirectBranchEdge";
         
      case MIAMI_BYPASS_EDGE:
         return "BypassEdge";
         
      case MIAMI_RETURN_EDGE:
         return "ReturnEdge";
         
      case MIAMI_CFG_EDGE:
         return "CFGEdge";
         
      default:
         assert(! "Invalid edge type.");
   }
   return (NULL);
}
//--------------------------------------------------------------------------------------------------------------------

const char* 
PrivateCFG::Edge::edgeTypeColor(EdgeType tt)
{
   switch (tt)
   {
      case ANY_EDGE_TYPE:
         return "black";
         
      case MIAMI_FALLTHRU_EDGE:
         return "blue";
         
      case MIAMI_DIRECT_BRANCH_EDGE:
         return "deepskyblue";
         
      case MIAMI_INDIRECT_BRANCH_EDGE:
         return "purple";
         
      case MIAMI_BYPASS_EDGE:
         return "darkgreen";
         
      case MIAMI_RETURN_EDGE:
         return "red";
         
      case MIAMI_CFG_EDGE:
         return "black";
         
      default:
         assert(! "Invalid edge type.");
   }
   return (NULL);
}
//--------------------------------------------------------------------------------------------------------------------

const char* 
PrivateCFG::Edge::edgeTypeStyle(EdgeType tt)
{
   switch (tt)
   {
      case ANY_EDGE_TYPE:
         return "\"setlinewidth(3),dotted\""; break;
         
      case MIAMI_FALLTHRU_EDGE:
      case MIAMI_DIRECT_BRANCH_EDGE:
      case MIAMI_INDIRECT_BRANCH_EDGE:
      case MIAMI_BYPASS_EDGE:
         return "\"setlinewidth(3),solid\""; break;
         
      case MIAMI_RETURN_EDGE:
      case MIAMI_CFG_EDGE:
         return "\"setlinewidth(3),dashed\""; break;
         
      default:
         assert(! "Invalid edge type.");
   }
   return (NULL);
}
//--------------------------------------------------------------------------------------------------------------------

void 
PrivateCFG::Node::computeInstructionLengths()
{
   // I need to iterate over all the instructions in this block
   // Because I do not know apriori the length of the instructions, 
   // I will decode them one at a time, record their length, and
   // advance to the end of the decoded instruction.
   // Use the machine independent interface for getting instruction lengths
   ninsts = 0;
   std::list<int> instLen; // temporary storage for instruction lengths
   int ilen = 0;
   addrtype pc = start_addr;
   addrtype reloc = RelocationOffset();
   std::list<int>::iterator lit;
   for ( ; pc<end_addr ; pc+=ilen)
   {
      if ((ilen=length_instruction_at_pc((void*)(pc+reloc), end_addr-pc))<1)
         break;
      instLen.push_back(ilen);
      ++ ninsts;
   }
   if (pc < end_addr)
   {
      // It is actually possible not to stop at exactly the block's end_address.
      // This case is possible when an instruction has a prefix, but there is
      // a path that jumps to the instruction itself, while another path falls through
      // the prefix.
      // Set an assert to catch if it happens in practice, but once verified, we can
      // remove the assert.
      // Found this case with a lock prefix. Changed condition from != to <
      fprintf(stderr, "Error: While iterating over instructions, did not stop at end of block [0x%lx,0x%lx], iterator pc=0x%lx, ninsts=%d\nInst lengths:",
            start_addr, end_addr, pc, ninsts);
      for (lit=instLen.begin() ; lit!=instLen.end() ; ++lit)
         fprintf(stderr, "  %d", *lit);
      fprintf(stderr, "\n");
      assert(!"bad instruction decoding");
   }
   if (inst_len)
      delete[] inst_len;
   inst_len = new int32_t[ninsts];
   int i=0;
   for (lit=instLen.begin() ; lit!=instLen.end() ; ++lit, ++i)
   {
      assert (i<ninsts);
      inst_len[i] = *lit;
   }
}
//--------------------------------------------------------------------------------------------------------------------


PrivateCFG::ForwardInstructionIterator::ForwardInstructionIterator(Node* n, addrtype from)
{
   node = n; 
   if (node->Size()>0 && node->inst_len==0)
   {
      node->computeInstructionLengths();
      // the number of instructions and their lengths should be computed by now
      assert (node->ninsts>0 && node->inst_len!=0);
   }
   i = 0;
   pc = node->start_addr;
   if (from != (addrtype)-1)   // find instruction at 'from' pc
      while(i<node->numInstructions() && pc<from)
      {
         pc += node->inst_len[i];
         ++i;
      }
}
//--------------------------------------------------------------------------------------------------------------------

PrivateCFG::BackwardInstructionIterator::BackwardInstructionIterator(Node* n, addrtype from)
{
   node = n; 
   if (node->Size()>0 && node->inst_len==0)
   {
      node->computeInstructionLengths();
      // the number of instructions and their lengths should be computed by now
      assert (node->ninsts>0 && node->inst_len!=0);
   }
   i = node->ninsts-1;
   pc = node->end_addr - node->inst_len[i];

   if (from != (addrtype)-1)   // find instruction at 'from' pc
      while(i>=0 && pc>from)
      {
         -- i;
         if (i>=0)
            pc -= node->inst_len[i];
      }
}
//--------------------------------------------------------------------------------------------------------------------

PrivateCFG::Node*
PrivateCFG::findNodeContainingAddress(addrtype pc)
{
   if (!setup_node_container)
   {
      NodesIterator nit(*this);
      while ((bool)nit)
      {
         Node *node = nit;
         if (node->Size()>0)
            all_nodes.insert(AddrNodeMap::value_type(node->getEndAddress(), node));
         ++ nit;
      }
      setup_node_container = true;
   }
   AddrNodeMap::iterator upit = all_nodes.upper_bound(pc);
   if (upit != all_nodes.end() && upit->second->getStartAddress()<=pc)
      return (upit->second);
   else
      return (0);
}
//--------------------------------------------------------------------------------------------------------------------
