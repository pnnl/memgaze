/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: Dominator.h
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Computes dominator information on a directed graph.
 */

#ifndef _MIAMI_DOMINATOR_H
#define _MIAMI_DOMINATOR_H


#include "unionfind.h"
#include "BaseDominator.h"

#pragma interface "Dominator.h"

namespace MIAMI
{
#define DRAW_DFS_TREE 0


template <class forward_iterator, class backward_iterator>
class Dominator : public BaseDominator
{
public:
   Dominator (DGraph *_g, SNodeList &root_nodes, bool _incl_struct=false, 
                  const char *prefix = "");
   Dominator (DGraph *_g, SNode* rootN, bool _incl_struct=false, 
                  const char *prefix = "");
   virtual ~Dominator ();

   // return number of minLCA operations (edge traversals); used for profiling
   int findLCA (SNodeList &nodeset, int nid, DisjointSet *dsuf, DisjointSet *ds,
               SNodeList *otherset, SNode **minLCA);
   
   inline bool Dominates (SNode *n1, SNode *n2) {
      int idx1 = dfsIndex[n1->getId()];
      int idx2 = dfsIndex[n2->getId()];
      while (idx2>idx1)
         idx2 = idom[idx2];
      return (idx2 == idx1);
   }
   
private:
#if DRAW_DFS_TREE
   void draw_DFS_tree (const char *prefix);
#endif
   void DFS_count (SNode *node, int pid);
   void calc_idoms ();
   void compress (int v);
   int eval_forest (int v);
   void link_forest (int v, int w);
   
   SNode **vertex;  // translates from a DFS index to an actual node
   int *parent;     // returns the parent (DFS index) of a node (DFS index)
   int *idom;       // stores the DFS index of the immediate dominator of a node
   int *numChildren;// stores the number of direct dominee of each node, dfs indexed
   int *semidom;    // stores the semidominator
   int *dfsIndex;   // translates from node id to DFS index
   
   int *ancestor;   // used to store the forest; pointer to parent in forest
   int *label;      // used in the link/eval thing. Not clearly explained
   int *size;       // number of nodes in this tree
   int *child;      // used in the link method. Unclear.
                    // Use it also as a temporary storage in findLCA
   
   int count;
   SNodeList *root_nodes;
   SNode *rootN;
   bool include_struct;
};

}  // namespace MIAMI

#endif
