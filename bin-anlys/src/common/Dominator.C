/*
 * This file is part of the MIAMI framework. For copyright information, see
 * the LICENSE file in the MIAMI root folder.
 */
/* 
 * File: Dominator.C
 * Author: Gabriel Marin, mgabi99@gmail.com
 *
 * Description: Computes dominator information on a directed graph.
 */

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <fstream>
#include "Dominator.h"

using namespace std;

namespace MIAMI
{

template <class forward_iterator, class backward_iterator>
Dominator<forward_iterator, backward_iterator>::Dominator (DGraph *_g, 
         SNodeList &_root_nodes, bool _incl_struct, const char *prefix)
           : BaseDominator (_g)
{
   int i;
//   fprintf (stderr, "Dominator::Dominator (1): numNodes=%d\n", numNodes);
   rootN = NULL;
   root_nodes = &_root_nodes;
   include_struct = _incl_struct;
   vertex   = new SNode* [numNodes];
   parent   = new int[numNodes];
   idom     = new int[numNodes];
   semidom  = new int[numNodes];
   dfsIndex = new int[numNodes];
   ancestor = new int[numNodes];
   label    = new int[numNodes];
   size     = new int[numNodes];
   child    = new int[numNodes];
   
   for (i=0 ; i<numNodes ; ++i)
   {
      vertex[i] = 0;
      parent[i] = idom[i] = dfsIndex[i] = ancestor[i] = 
           child[i] = 0;
      label[i] = semidom[i] = i;
      size[i] = 1;
   }
   size[0] = 0;
   count = 1;
   SNodeList::iterator nlit = root_nodes->begin ();
   for ( ; nlit!=root_nodes->end() ; ++nlit)
      if (dfsIndex[(*nlit)->getId()] == 0)  // did not visit it yet
         DFS_count (*nlit, 0);
   // I have all nodes DFS numbered at this point
   // Stage 2 & 3, I need to compute semidominators
   // Stage 4: compute immediate dominators
#if DRAW_DFS_TREE
   draw_DFS_tree (prefix);
#endif
   calc_idoms ();
   
   // transform vertex such that vertex[node->id] points to the immediate
   // dominator of that node, or NULL if node is root in dominator tree
   numChildren = new int [count];
   memset (numChildren, 0, count*sizeof(int));
   
   for (i=1 ; i<count ; ++i)
   {
      nodeDom[vertex[i]->getId()] = vertex[idom[i]];
      numChildren[idom[i]] += 1;
   }
   
   // once immediate dominators are computed, we can deallocate all the
   // temporary arrays; keep the minimum necessary
   delete[] parent;
   delete[] semidom;
   delete[] ancestor;
   delete[] label;
   delete[] size;
}

template <class forward_iterator, class backward_iterator>
Dominator<forward_iterator, backward_iterator>::Dominator (DGraph *_g, 
         SNode* _rootN, bool _incl_struct, const char *prefix)
           : BaseDominator (_g)
{
   int i;
//   fprintf (stderr, "Dominator::Dominator (2): numNodes=%d\n", numNodes);
   rootN = _rootN;
   root_nodes = NULL;
   include_struct = _incl_struct;
   vertex   = new SNode* [numNodes];
   parent   = new int[numNodes];
   idom     = new int[numNodes];
   semidom  = new int[numNodes];
   dfsIndex = new int[numNodes];
   ancestor = new int[numNodes];
   label    = new int[numNodes];
   size     = new int[numNodes];
   child    = new int[numNodes];
   
   for (i=0 ; i<numNodes ; ++i)
   {
      vertex[i] = 0;
      parent[i] = idom[i] = dfsIndex[i] = ancestor[i] = 
           child[i] = 0;
      label[i] = semidom[i] = i;
      size[i] = 1;
   }
   size[0] = 0;
   count = 1;
   if (dfsIndex[rootN->getId()] == 0)  // did not visit it yet
      DFS_count (rootN, 0);
   // I have all nodes DFS numbered at this point
   // Stage 2 & 3, I need to compute semidominators
   // Stage 4: compute immediate dominators
#if DRAW_DFS_TREE
   draw_DFS_tree (prefix);
#endif
   calc_idoms ();
   
   // transform vertex such that vertex[node->id] points to the immediate
   // dominator of that node, or NULL if node is root in dominator tree
   numChildren = new int [count];
   memset (numChildren, 0, count*sizeof(int));
   
   for (i=1 ; i<count ; ++i)
   {
      SNode *tmpdom = vertex[idom[i]];
      if (tmpdom)
         nodeDom[vertex[i]->getId()] = tmpdom;
      else
         nodeDom[vertex[i]->getId()] = NULL;
      numChildren[idom[i]] += 1;
   }
   
   // once immediate dominators are computed, we can deallocate all the
   // temporary arrays; keep the minimum necessary
   delete[] parent;
   delete[] semidom;
   delete[] ancestor;
   delete[] label;
   delete[] size;
}


template <class forward_iterator, class backward_iterator> int
Dominator<forward_iterator, backward_iterator>::
    findLCA (SNodeList &nodeset, int nid, DisjointSet *dsuf, DisjointSet *ods,
               SNodeList *otherset, SNode **minLCA)
{
   int perf = 1;  // count how many minLCA operations are executed
   
   // compute the min LCA of the parent of nid and all nodes in nodeset.
   // Once we find the minLCA, check if it dominates only one node. 
   // If it dominates more than one node, then return the found node. 
   // If it dominates only one node, then add this node
   // in otherset and go up in the dominator tree applying the same check step.
   // If we have multiple nodes in nodeset, compute LCA of two nodes at a time.
   // Then compute the LCA of the intermediary LCA and the next node in the 
   // list until only one LCA remaining, or we find there is no LCA (in case of
   // a forest only).
   assert (nid != 0);
   // Get minLCA for nid and compute a new LCA considering also the nodes
   // in nodeset.
   // For nodes in nodeset, I have to find their parent first.
   
   int idx1 = 0;
   int p1 = dsuf->Find (nid);
   SNode *node1 = minLCA[p1];
   if (node1)
      idx1 = dfsIndex [node1->getId()];
   
   SNodeList::iterator slit = nodeset.begin ();
   for ( ; slit!=nodeset.end() ; ++slit)  // find LCA of node1 and LCA(*slit)
   {
      int pathLen = 0;
      SNode *node2 = *slit;
      int id2 = dsuf->Find (node2->getId());
      int idx2 = 0;

      SNode *lca = minLCA[id2];
      perf += 1;
      // I am going to union all nodes involved in computing the LCA of
      // these 2 nodes, only after I find the LCA. Store them in a temp array
      child[pathLen] = node2->getId();
      ++ pathLen;
      
      if (lca)
         idx2 = dfsIndex [lca->getId()];
         
      // find the LCA of idx1 and idx2;
      SNode *aux = NULL;
      bool firstIdx;
      while (idx1 != idx2)
      {
         int newIdx = 0;
         if (idx1 < idx2)
         {
            newIdx = idx2;
            firstIdx = false;
         }
         else
         {
            newIdx = idx1;
            firstIdx = true;
         }
         aux = vertex[newIdx];
         int auxid = aux->getId();
         child[pathLen] = auxid;
         ++ pathLen;

         // add aux to the otherset
         int p2 = dsuf->Find (auxid);
         lca = minLCA[p2];
         perf += 1;
         if (lca)
            newIdx = dfsIndex [lca->getId()];
         else
            newIdx = 0;
         
         if (firstIdx)
            idx1 = newIdx;
         else
            idx2 = newIdx;
         
         
         // I have to add aux to otherset; check if it was added already
         // I do not need to add *slit to otherset, because it must be
         // unioned already (that's why I have to process it).
         int op1 = ods->Find (nid);
         int op2 = ods->Find (auxid);
         if (op1 != op2)
         {
            // do not union here. They are going to be unioned in the other dom
            otherset->push_back (aux);
//            fprintf (stderr, "In Dominator::findLCA union base node B%d with node B%d\n",
//                nid, auxid);
         }
      }
      // union all nodes stored in child
      for (int i=0 ; i<pathLen ; ++i)
      {
         id2 = dsuf->Find (child[i]);
         if (p1 != id2)
         {
            // should I union them here?? Yes
            dsuf->Union (p1, id2);
            p1 = dsuf->Find (nid);
         }
      }
      pathLen = 0;
      
      // I finished finding the LCA of two nodes. Store the intermidiate 
      // result in minLCA.
//      fprintf (stderr, "findLCA check: p1=%d, dsuf->Find(%d)=%d; Found minLCA is B%d\n", p1, nid,
//           dsuf->Find(nid), (vertex[idx1]?vertex[idx1]->getId():0));
      minLCA[p1] = vertex[idx1];
   }
#if 1
   // idx1 is the LCA node. I have only one more check to do.
   // Make sure the found LCA has at least two immediate dominee?
   // What if it has only one? It means this cannot be a real entry/exit.
   while (idx1 && numChildren[idx1]<2)
   {
      // numChildren should be 1; this node must have at least one child
      assert (numChildren[idx1] == 1);
      // find node and add it to the other set
      SNode *aux = vertex[idx1];
      assert (aux);
      int auxid = aux->getId();
      int p2 = dsuf->Find (auxid);
      assert (p1 != p2);  // they couldn't have been unioned in a previous iter
      SNode *lca = minLCA[p2];
      perf += 1;
      if (lca)
         idx1 = dfsIndex [lca->getId()];
      else
         idx1 = 0;
      dsuf->Union (p1, p2);
      p1 = dsuf->Find (nid);
      minLCA[p1] = lca;
//      fprintf (stderr, "findLCA check2: p1=%d, dsuf->Find(%d)=%d; Found minLCA is B%d\n", p1, nid,
//           dsuf->Find(nid), (lca?lca->getId():0));

      // I have to add aux to otherset; check if it was added already
      int op1 = ods->Find (nid);
      int op2 = ods->Find (auxid);
      if (op1 != op2)
      {
         // do not union here. They are going to be unioned in the other dom
         otherset->push_back (aux);
//         fprintf (stderr, "In Dominator::findLCA (one child) union base node B%d with node B%d\n",
//              nid, auxid);
      }
   }
#endif
#if VERBOSE_DEBUG_DOMINATOR
   DEBUG_DOM (6, 
   {
      cerr << "findLCA: starting from node with id " << nid 
           << " we found the final LCA is ";
      if (idx1)
         cerr << *(vertex[idx1]) << endl;
      else
         cerr << "NONE." << endl;
   }
   )
#endif
   return (perf);
}

template <class forward_iterator, class backward_iterator> void
Dominator<forward_iterator, backward_iterator>::
    DFS_count (SNode *node, int pid)
{
   int w = node->getId ();
   vertex [count] = node;
   parent [count] = pid;
   pid = count;
   dfsIndex[w] = count;
   ++ count;
   // Must write a SuccesiveNodeIterator that iterates over the
   // succesor edge and returns the node at the other end.
   forward_iterator nit (node, include_struct);
   while ((bool)nit)
   {
      if (dfsIndex[nit->getId()] == 0)  // did not visit it yet
         DFS_count (nit, pid);
      ++ nit;
   }
}

#if DRAW_DFS_TREE
template <class forward_iterator, class backward_iterator> void
Dominator<forward_iterator, backward_iterator>::
    draw_DFS_tree (const char *prefix)
{
   int i;
   char file_name[64];
   char file_name_ps[64];
   char title_draw[64];

   sprintf(file_name, "%s_dfs_tree.dot", prefix);
   sprintf(file_name_ps, "%s_dfs_tree.ps", prefix);
   sprintf(title_draw, "%s_dfs_tree", prefix);
   ofstream fout(file_name, ios::out);
   assert(fout && fout.good());

   fout << "digraph \"" << title_draw << "\"{\n";
   fout << "size=\"7.5,10\";\n";
   fout << "center=\"true\";\n";
//   fout << "ratio=\"compress\";\n";
   fout << "ranksep=.3;\n";
   fout << "node[color=black,fontcolor=black,shape=ellipse,font=Helvetica,fontsize=14,height=.3];\n";
   fout << "edge[font=Helvetica,fontsize=14,dir=forward,style=\"setlinewidth(2),solid\",color=black,fontcolor=black];\n";
   for (i=1 ; i<count ; ++i)
   {
      SNode *nn = vertex[i];  // it should not be NULL;
      assert (nn!=0);
      nn->draw(fout, i);
      int prnt = parent[i];
      if (prnt)   // if it is not a root node
      {
         SNode *idomN = vertex[prnt];
         assert (idomN);
         fout << "B" << idomN->getId() << "->B" << nn->getId() << ";\n";
      }
   }
   fout << "}\n";

   fout.close();
   assert(fout.good());
}
#endif

template <class forward_iterator, class backward_iterator>
Dominator<forward_iterator, backward_iterator>::~Dominator ()
{
   delete[] child;
   delete[] idom;
   delete[] dfsIndex;
   delete[] vertex;
   delete[] numChildren;
}


/* Calculate the immediate dominators. */
template <class forward_iterator, class backward_iterator> void
Dominator<forward_iterator, backward_iterator>::calc_idoms ()
{
  int v, w, u, prnt;

  int *bucket = new int[numNodes];
  int *next_bucket = new int[numNodes];
  for (u=0 ; u<numNodes ; ++u)
     bucket[u] = next_bucket[u] = 0;
  
  /* Go backwards in DFS order. */
  for (w=count-1 ; w>0 ; --w)
  {
     prnt = parent[w];
     // if parent is zero, then this a top-node; nothing to do
     if (prnt == 0)
        continue;
     SNode *sn = vertex[w];
     u = w;
     backward_iterator bit (sn, include_struct);
     
     /* Search all direct predecessors for the smallest node with a path
        to them.  That way we have the smallest node with also a path to
        us only over nodes behind us.  In effect we search for our
        semidominator.  */
     while ((bool)bit)
     {
        SNode *pn = bit;
//        assert (pn->isInstructionNode() || pn->isCfgNode());
        v = dfsIndex [pn->getId()];
	/* Call eval_forest() only if really needed.  If v < w,
           then we know, that eval(v) == v and semidom[v] == v.
         */
        if (v > w)
           v = semidom [eval_forest (v)];
        if (v < u)
           u = v;
           
        ++ bit;
     }

     semidom[w] = u;
     link_forest (prnt, w);
     next_bucket[w] = bucket[u];
     bucket[u] = w;

     /* Transform semidominators into dominators. */
     for (v = bucket[prnt] ; v ; v=next_bucket[v])
     {
        u = eval_forest (v);
        if (semidom[u] < semidom[v])
           idom[v] = u;
        else
           idom[v] = prnt;
     }
    
     /* no need to cleanup next_bucket[].  */
     bucket[prnt] = 0;
  }
  
  // deallocate temporary array
  delete[] bucket;
  delete[] next_bucket;

  /* Step 4: Explicitely define the dominators.  */
  for (w=1 ; w<count ; ++w)
     if (parent[w] == 0)  // a top node
        idom[w] = 0;
     else
        if (idom[w] != semidom[w])
           idom[w] = idom [idom[w]];
}


/* Compress the path from V to the root of its set and update label at the
   same time.  After compress(v) ancestor[v] is the root of the set v is
   in and label[v] is the node with the smallest semidom[] value on the path
   from v to that root. */

template <class forward_iterator, class backward_iterator> void
Dominator<forward_iterator, backward_iterator>::compress (int v)
{
  int prnt = ancestor [v];
  if (ancestor [prnt])
  {
     compress (prnt);
     if (semidom [label[prnt]] < semidom [label[v]])
        label[v] = label[prnt];
     ancestor[v] = ancestor[prnt];
  }
}

/* Compress the path from v to the set root of V if needed (when the root has
   changed since the last call).  Returns the node with the smallest semidom[]
   value on the path from v to the root.  */

template <class forward_iterator, class backward_iterator> int
Dominator<forward_iterator, backward_iterator>::eval_forest (int v)
{
  int prnt = ancestor[v];

  if (!prnt)         // v is the root
    return (label[v]);

  /* Compress only if necessary.  */
  if (ancestor[prnt])
  {
     compress (v);
     prnt = ancestor[v];
  }

  if (semidom[label[prnt]] >= semidom[label[v]])
    return (label[v]);
  else
    return (label[prnt]);
}

/* This essentially merges the two sets of v and w, giving a single set with
   the new root v.  The internal representation of these disjoint sets is a
   balanced tree.  Currently link_forest(v,w) is only used with v being the 
   parent of w. */

template <class forward_iterator, class backward_iterator> void
Dominator<forward_iterator, backward_iterator>::link_forest (int v, int w)
{
  // this routine assumes that size[0] == label[0] == semidom[0] == 0;
  int s = w;

  /* Rebalance the tree.  */
  while (semidom[label[w]] < semidom[label[child[s]]])
  {
     if (size[s]+size[child[child[s]]] >= 2*size[child[s]])
     {
        ancestor[child[s]] = s;
        child[s] = child[child[s]];
     }
     else
     {
        size[child[s]] = size[s];
        s = ancestor[s] = child[s];
     }
  }

  label[s] = label[w];
  size[v] += size[w];
  if (size[v] < 2*size[w])
  {
     int tmp = s;
     s = child[v]; child[v] = tmp;
  }

  /* Merge all subtrees.  */
  while (s)
  {
     ancestor[s] = v;
     s = child[s];
  }
}

} /* namespace MIAMI */
